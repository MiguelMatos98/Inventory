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
#include "Components/Border.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AUEInventoryCharacter

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
}

void AUEInventoryCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	PlayerController = Cast<APlayerController>(GetController());

	DisplayInventory();
}

//////////////////////////////////////////////////////////////////////////
// Input
void AUEInventoryCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Add Input Mapping Context
	PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Click
		EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Completed, this, &AUEInventoryCharacter::OnClick);
		if (ClickAction == nullptr)
			UE_LOG(LogTemp, Warning, TEXT("Click action is null"));
		
		// Toggle
		EnhancedInputComponent->BindAction(ToggleAction, ETriggerEvent::Triggered, this, &AUEInventoryCharacter::OnToggle);
		if (ToggleAction == nullptr)
			UE_LOG(LogTemp, Warning, TEXT("Toggle action is null"));
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUEInventoryCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AUEInventoryCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
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
	if (!Inventory)
	{
		// Since inventory isn't initialed through BP
		// We'll set it up here once UEInventoryCharacter gets instantiated
		PlayerController = Cast<APlayerController>(GetController());
		if (PlayerController)
		{
			PlayerController->bShowMouseCursor = true;
			PlayerController->bEnableClickEvents = true;
			PlayerController->bEnableMouseOverEvents = true;
		
			// Create inventory widget
			Inventory = CreateWidget<UInventory>(PlayerController.Get(), UInventory::StaticClass());
			UE_LOG(LogTemp,  Warning, TEXT("Inventory has been created"))

			// Check inventory widget creation was successful
			// in order to add it to the viewport as hidden
			if (Inventory)
			{
				Inventory->AddToViewport();
				Inventory->SetIsEnabled(true);
				Inventory->SetVisibility(ESlateVisibility::Visible);
				UE_LOG(LogTemp,  Warning, TEXT("Inventory has been added to viewport"))
			}
		}
	}
	else
	{
		UE_LOG(LogTemp,  Warning, TEXT("Inventory has been already create"))
	}
}

void AUEInventoryCharacter::OnToggle()
{
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is null"));
		return;
	}
    
	// Toggle visibility
	if (Inventory->GetVisibility() == ESlateVisibility::Visible)
	{
		Inventory->Close();
	}
	else
	{
		Inventory->Open();
	}
}

void AUEInventoryCharacter::OnClick()
{
    if (!PlayerController || !Inventory)
    {
        return;
    }

    FPointerEvent MouseEvent; // (Populate this as needed.)
    int64 HoveredIndex = Inventory->FindHoveredItemIndex(MouseEvent);

    // If an item is being dragged, finalize its position.
    if (HoveredIndex != INDEX_NONE)
    {
        UE_LOG(LogTemp, Log, TEXT("Dragging item at index %lld"), HoveredIndex);
        Inventory->MoveItem(MouseEvent, false, true); // Drop the item
        return;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Item at index not dragged %lld"), HoveredIndex);
    }

    if (!Inventory->GetIsInventoryFull())
    {
        FVector MouseWorldLocation, MouseWorldDirection;
        if (PlayerController->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection))
        {
            FHitResult Hit;
            constexpr float TraceDistance = 1000.0f;
            FVector FinalTracePosition = MouseWorldLocation + (MouseWorldDirection * TraceDistance);

            FCollisionQueryParams CollisionQuery;
            CollisionQuery.AddIgnoredActor(this);

            UE_LOG(LogTemp, Log, TEXT("Mouse Projected to World"));

            if (GetWorld()->LineTraceSingleByChannel(Hit, MouseWorldLocation, FinalTracePosition, ECC_Visibility, CollisionQuery))
            {
                UE_LOG(LogTemp, Log, TEXT("Hit: %s"), *Hit.GetActor()->GetName());
                if (Hit.GetActor())
                {
                    // Project hit location to screen space.
                    FVector2D ScreenPosition;
                    PlayerController->ProjectWorldLocationToScreen(Hit.Location, ScreenPosition);
                    UE_LOG(LogTemp, Log, TEXT("Screen Mouse Position (Viewport Space): %s"), *ScreenPosition.ToString());

                    // Get the grid panel.
                    UUniformGridPanel* GridPanel = Inventory->GetGrid();
                    if (GridPanel)
                    {
                        // Convert the screen position to the grid panel's local space.
                        FVector2D LocalMousePosition = GridPanel->GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
                        UE_LOG(LogTemp, Log, TEXT("Local Mouse Position: %s"), *LocalMousePosition.ToString());

                        // Get the overall grid size.
                        FVector2D GridPanelSize = GridPanel->GetCachedGeometry().GetLocalSize();
                        // Calculate cell size using your predefined MaxColumns and MaxRows.
                        FVector2D CellSize = FVector2D(GridPanelSize.X / UInventory::MaxColumns,
                                                       GridPanelSize.Y / UInventory::MaxRows);
                        UE_LOG(LogTemp, Log, TEXT("Grid Size: %s"), *GridPanelSize.ToString());

                        bool bFoundSlot = false;
                        // Loop through each item.
                        for (int32 i = 0; i < Inventory->GetItems().Num(); ++i)
                        {
                            if (Inventory->GetItems().IsValidIndex(i) &&
                                Inventory->GetForegroundBorders().IsValidIndex(i))
                            {
                                UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(Inventory->GetForegroundBorders()[i]->Slot);
                                if (GridSlot)
                                {
                                    int32 Row = GridSlot->GetRow();
                                    int32 Column = GridSlot->GetColumn();

                                    // Calculate bounds in the grid panel's local space.
                                    FVector2D TopLeft = FVector2D(Column * CellSize.X, Row * CellSize.Y);
                                    FVector2D BottomRight = TopLeft + CellSize;
                                    UE_LOG(LogTemp, Log, TEXT("Border[%d] - TopLeft: %s, BottomRight: %s"),
                                        i, *TopLeft.ToString(), *BottomRight.ToString());

                                    // If the local mouse position falls inside this cell, finalize pickup.
                                    if (LocalMousePosition.X >= TopLeft.X && LocalMousePosition.X <= BottomRight.X &&
                                        LocalMousePosition.Y >= TopLeft.Y && LocalMousePosition.Y <= BottomRight.Y)
                                    {
                                        bFoundSlot = true;
                                        // Finalize move: this updates the icon position.
                                        Inventory->MoveItem(MouseEvent, false, true);
                                        break;
                                    }
                                }
                            }
                        }
                        if (!bFoundSlot)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("No grid slot matched the local mouse position"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("GridPanel not found!"));
                    }
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Inventory is full"));
    }
}





void AUEInventoryCharacter::OnDrag()
{
}