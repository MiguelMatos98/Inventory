// Copyright Epic Games, Inc. All Rights Reserved.

#include "UEInventoryCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Inventory.h"
#include "Components/UniformGridPanel.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AUEInventoryCharacter::AUEInventoryCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++

	InventoryTagsToIgnore = { "Floor", "Wall" };
}

void AUEInventoryCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	PlayerController = Cast<APlayerController>(GetController());

	DisplayInventory();
}

void AUEInventoryCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Add Input Mapping Context
    PlayerController = Cast<APlayerController>(GetController());
    if (PlayerController)
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

	// Bind Input actions (Such as the clicking and toggling)
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (ClickAction)
        {
            EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Started, this, &AUEInventoryCharacter::OnClick);
        }

        if (ToggleAction)
        {
            EnhancedInputComponent->BindAction(ToggleAction, ETriggerEvent::Triggered, this, &AUEInventoryCharacter::OnToggle);
        }

        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }

        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUEInventoryCharacter::Move);
        }

        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AUEInventoryCharacter::Look);
        }
    }
}

void AUEInventoryCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AUEInventoryCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AUEInventoryCharacter::DisplayInventory()
{
	if (Inventory == nullptr)
	{
		PlayerController = Cast<APlayerController>(GetController());
		if (PlayerController)
		{
			PlayerController->bShowMouseCursor = true;
			PlayerController->bEnableClickEvents = true;
			PlayerController->bEnableMouseOverEvents = true;

			Inventory = CreateWidget<UInventory>(PlayerController.Get(), UInventory::StaticClass());
			if (Inventory)
			{
				Inventory->AddToViewport();
				Inventory->SetIsEnabled(true);
				Inventory->SetVisibility(ESlateVisibility::Visible);

				#if	WITH_EDITOR
				UE_LOG(LogTemp, Log, TEXT("Inventory Has Been Created Succesfullly"))
				#endif
			}
			
		}
		else
		{
			#if	WITH_EDITOR
			UE_LOG(LogTemp, Error, TEXT("PLayerController Cast Has Failed In DisplayInventory()"));
			#else
			UE_LOG(LogTemp, Fatal, TEXT("PLayerController Cast Has Failed In DisplayInventory()"));
			#endif
			
		}
	}
}

void AUEInventoryCharacter::OnToggle()
{
	if (PlayerController == nullptr)
	{
		#if	WITH_EDITOR
		UE_LOG(LogTemp, Error, TEXT("PlayerController Is Null In OnToggle()"));
		#else
		UE_LOG(LogTemp, Fatal, TEXT("PlayerController Is Null In OnToggle()"));
		#endif
		
		return;
	}
    
	Inventory->GetVisibility() == ESlateVisibility::Visible ? Inventory->Close() : Inventory->Open();
}

void AUEInventoryCharacter::OnClick(const FInputActionValue& Value)
{
    if (PlayerController == nullptr)
    {
		#if	WITH_EDITOR
    	UE_LOG(LogTemp, Error, TEXT("PlayerController Is Null In OnClick()"));
		#else
    	UE_LOG(LogTemp, Fatal, TEXT("PlayerController Is Null In OnClick()"));
		#endif
    	
    	return;
    }

	if (Inventory == nullptr)
	{
		#if	WITH_EDITOR
		UE_LOG(LogTemp, Error, TEXT("Inventory Is Null In OnClick()"));
		#else
		UE_LOG(LogTemp, Fatal, TEXT("PlayerController Is Null In OnClick()"));
		#endif
		
		return;
	}
	
    const bool bIsMuseCLickDown = (Value.GetMagnitude() > 0.0f);
	
    if (!bIsMuseCLickDown)
    {
        return;
    }

	
	// No more items can be added when invenotory is full
	if (Inventory->IsInventoryFull())
	{
		#if	WITH_EDITOR
		UE_LOG(LogTemp, Error, TEXT("Inventory Is Full In OnClick()"));
		#endif
		
		return;
	}
	
	FVector2D MousePosition;
	if (!PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y))
	{
		#if	WITH_EDITOR
		UE_LOG(LogTemp, Error, TEXT("PlayerController Failed To Get Mouse Position In OnClick()"));
		#else
		UE_LOG(LogTemp, Fatal, TEXT("PlayerController Failed To Get Mouse Position In OnClick()"));
		#endif
		
		return;
	}
	
	// Convert mouse coordinates to world space
	FVector MouseWorldOrigin, MouseWorldDirection;
	if (!PlayerController->DeprojectMousePositionToWorld(MouseWorldOrigin, MouseWorldDirection))
	{
		#if WITH_EDITOR
		UE_LOG(LogTemp, Error, TEXT("DeprojectMousePositionToWorld failed in OnClick()"));
		#else
		UE_LOG(LogTemp, Fatal, TEXT("DeprojectMousePositionToWorld failed in OnClick()"));
		#endif
		
		return;
	}

	
    if (bIsMuseCLickDown)
    {
        FHitResult Hit;

        FCollisionQueryParams CollisionQuery;

		constexpr float TraceDistance = 1000.0f;

		// Calculate vector for line trace distance and ignore self
        FVector FinalTracePosition = MouseWorldOrigin + (MouseWorldDirection * TraceDistance);

        CollisionQuery.AddIgnoredActor(this);

        if (GetWorld()->LineTraceSingleByChannel(Hit, MouseWorldOrigin, FinalTracePosition, ECC_Visibility, CollisionQuery))
        {
		    for (const FName& IgnoreTag : InventoryTagsToIgnore)
			{
				 // Return when player tries to add a floor or wall objects to the inventory to keep level intact
				if (Hit.GetActor()->ActorHasTag(IgnoreTag)) return;	
			}
           	
           	if (Hit.GetActor())
           	{
           		Inventory->AddItem(Hit.GetActor());
           	}
        }
    }
}