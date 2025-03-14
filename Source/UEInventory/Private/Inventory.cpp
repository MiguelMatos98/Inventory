// Fill out your copyright notice in the Description page of Project Settings.

// New File

#include "Inventory.h"

#ifndef INVENTORY_HEADERS_H
#define INVENTORY_HEADERS_H

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "UEInventory/Item.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Blueprint/WidgetTree.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"

#endif // !INVENTORY_HEADERS_H

void UInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("WidgetTree is invalid"));
		return;
	}
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	if (!Canvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("Canvas is invalid"));
		return;
	}
		// Add canvas to the root widget
		WidgetTree->RootWidget = Canvas;

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	if (!BackgroundBorder)
	{
		UE_LOG(LogTemp, Warning, TEXT("BackgroundBorder is invalid"));
		return;
	}
		BackgroundBorder->SetBrushColor(FLinearColor::Gray);  // Example border color

	UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (!VerticalBox) {
		UE_LOG(LogTemp, Warning, TEXT("VerticalBox is invalid"));
		return;
	}
	
	Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (Title)
	{
		UE_LOG(LogTemp, Error, TEXT("Title is valid"));
		Title->SetText(FText::FromString(TEXT("Inventory")));
	
		//	Grab inventory's name slot and offset it 
		if (UVerticalBoxSlot* TitleVSlot = VerticalBox->AddChildToVerticalBox(Title))
		{
			UE_LOG(LogTemp, Error, TEXT("TitleVSlot is valid"));
			// Center horizontally, add some top padding
			TitleVSlot->SetHorizontalAlignment(HAlign_Center);
			TitleVSlot->SetPadding(FMargin(0, 20, 0, 10));
		}
	}
	
	// Create instance of UUniformGridPanel at run time
	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	if (Grid)
	{
		UE_LOG(LogTemp, Error, TEXT("Grid is valid"));
		// Reduce padding in the grid itself to avoid extra spacing
		Grid->SetSlotPadding(FMargin(10, 16, 10, 16));  // Reducing slot padding to minimal

		if (UVerticalBoxSlot* GridVSlot = VerticalBox->AddChildToVerticalBox(Grid))
		{
			UE_LOG(LogTemp, Error, TEXT("GridVSlot is valid"));
			// Fill remaining space in the VerticalBox
			GridVSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}
	
	BackgroundBorder->SetContent(VerticalBox);
	
	// Add inventory grid to the canvas panel
	Canvas->AddChild(BackgroundBorder);
	
	BackgroundBorderSlot = Cast<UCanvasPanelSlot>(BackgroundBorder->Slot);
	if (BackgroundBorderSlot)
	{
		BackgroundBorderSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		BackgroundBorderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		BackgroundBorderSlot->SetOffsets(FMargin(-50.0f, 50.0f, 510.0f, 500.0f));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BackgroundBorderSlot is invalid"));
	}
	
	Create(3,4);
}

void UInventory::UpdateItemDisplay(int32 SlotIndex, UTexture2D* NewTexture)
{
	if (!NewTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateItemDisplay: NewTexture is null."));
		return;
	}

	if (ForegroundBorders.IsValidIndex(SlotIndex) && ForegroundBorders[SlotIndex])
	{
		// Set the brush of the border to display the new texture.
		ForegroundBorders[SlotIndex]->SetBrushFromTexture(NewTexture);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateItemDisplay: Invalid SlotIndex %d."), SlotIndex);
	}
}

void UInventory::Create(uint64 Rows, uint64 Columns)
{
	if (!Grid)
	{
		return;
	}

	// Clear all the slots before creating the inventory grid
	Grid->ClearChildren();
	ForegroundBorders.Empty();

	for (uint64 CurrentRow = 0; CurrentRow < Rows; ++CurrentRow)
	{
		for (uint64 CurrentColumn = 0; CurrentColumn < Columns; ++CurrentColumn)
		{
			// Add a border to the slot array
			ForegroundBorders.Add(WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()));

			ForegroundBorders.Last()->SetBrushColor(FLinearColor::Black);  // Example border color
			
			// Add last slot border to the inventory grid
			GridSlot = Grid->AddChildToUniformGrid(ForegroundBorders.Last(), CurrentRow, CurrentColumn);

			if(GridSlot)
			{
				// Set the slot's alignment
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}
}

void UInventory::AddItem(AActor* ItemActor)
{
    if (!ItemActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemActor is null"));
        return;
    }

    FVector ActorWorldLocation = ItemActor->GetActorLocation();
    TObjectPtr<APlayerController> PlayerController = GetWorld()->GetFirstPlayerController();

    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player Controller doesn't exist"));
        return;
    }

    FVector2D ScreenLocation;
    bool bProjected = PlayerController->ProjectWorldLocationToScreen(ActorWorldLocation, ScreenLocation);

    if (bProjected)
    {
        // Successfully projected to screen
        UE_LOG(LogTemp, Log, TEXT("Screen X: %f, Screen Y: %f"), ScreenLocation.X, ScreenLocation.Y);

    	// Capture the 3D object as a 2D texture
    	SceneCaptureComponent = NewObject<USceneCaptureComponent2D>(ItemActor);
    	SceneCaptureComponent->RegisterComponent();
        SceneCaptureComponent->AttachToComponent(ItemActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        SceneCaptureComponent->ShowOnlyActorComponents(ItemActor);

        UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
        RenderTarget->ClearColor = FLinearColor::White;
        RenderTarget->InitCustomFormat(512, 512, PF_B8G8R8A8, true);
        SceneCaptureComponent->TextureTarget = RenderTarget;

        if (SceneCaptureComponent)
        {
            // Capture the scene
            SceneCaptureComponent->CaptureScene();

        	UWorld* world = ItemActor->GetWorld();
        	if (!world)
			{
				UE_LOG(LogTemp, Warning, TEXT("World is null"));
				return;
			}
            // Convert render target to Texture2D
            UTexture2D* CapturedTexture = ConvertRenderTargetToTexture(world, SceneCaptureComponent->TextureTarget);

            int32 SlotIndex = 0;
            if (CapturedTexture)
            {
                // Display the captured texture in the UI
                UE_LOG(LogTemp, Warning, TEXT("Successful conversion render target to texture"));
                Items.Add(FItem(CapturedTexture, ActorWorldLocation));
                UpdateItemDisplay(SlotIndex, Items[SlotIndex].Texture.Get());

            	// Save the captured texture to PNG for debugging or persistence
            	FString SavePath = FPaths::ProjectDir() / TEXT("Saved/MyTexture.png");
            	bool bSuccess = SaveTexture2DToPNG(CapturedTexture, SavePath);
            	UE_LOG(LogTemp, Log, TEXT("Texture save to %s was %s"), *SavePath, bSuccess ? TEXT("successful") : TEXT("unsuccessful"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to convert render target to texture"));
            }
        }
    }
    else
    {
        // Projection failed
        UE_LOG(LogTemp, Warning, TEXT("Failed to project world location to screen"));
    }
}



void UInventory::RemoveItem()
{
}

void UInventory::MoveItem()
{
}

void UInventory::SortItem(FItem MovedItem, FItem ItemToMove)
{
}

void UInventory::Open()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UInventory::Close()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

bool UInventory::SaveTexture2DToPNG(UTexture2D* Texture, const FString& FilePath)
{
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid texture or missing mips."));
		return false;
	}

	// Access the first mip map.
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	int32 Width = Mip.SizeX;
	int32 Height = Mip.SizeY;

	// Lock the bulk data to read the pixels.
	void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);
	if (!Data)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to lock texture data."));
		return false;
	}

	// Copy the pixel data into an array of FColor.
	TArray<FColor> ColorData;
	ColorData.AddUninitialized(Width * Height);
	FMemory::Memcpy(ColorData.GetData(), Data, Width * Height * sizeof(FColor));

	// Unlock the bulk data.
	Mip.BulkData.Unlock();

	// Compress the pixel data to PNG format.
	TArray64<uint8> PNGData;
	FImageUtils::PNGCompressImageArray(Width, Height, ColorData, PNGData);

	// Save the PNG data to disk.
	if (!FFileHelper::SaveArrayToFile(PNGData, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to save PNG file to: %s"), *FilePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Successfully saved texture as PNG: %s"), *FilePath);
	return true;
}

UTexture2D* UInventory::ConvertRenderTargetToTexture(UWorld* World, UTextureRenderTarget2D* RenderTarget)
{
	if (!RenderTarget) return nullptr;

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	FReadSurfaceDataFlags ReadPixelFlags;
	TArray<FColor> OutBMP;

	// Read the surface data
	RenderTargetResource->ReadPixels(OutBMP, ReadPixelFlags);

	// Create the texture
	UTexture2D* NewTexture = UTexture2D::CreateTransient(RenderTarget->SizeX, RenderTarget->SizeY);
    
	// Lock the bulk data
	FTexturePlatformData* PlatformData = NewTexture->GetPlatformData();
	FTexture2DMipMap& Mip = PlatformData->Mips[0];
	void* LockedData = Mip.BulkData.Lock(LOCK_READ_WRITE);
    
	// Copy pixel data into the new texture
	FMemory::Memcpy(LockedData, OutBMP.GetData(), OutBMP.Num() * sizeof(FColor));
    
	// Unlock the bulk data and update the texture resource
	Mip.BulkData.Unlock();
	NewTexture->UpdateResource(); // Use UpdateResource() instead of UpdateResourceData()

	return NewTexture;
}

TArray<FItem>& UInventory::GetItems()
{
	return Items;
}

