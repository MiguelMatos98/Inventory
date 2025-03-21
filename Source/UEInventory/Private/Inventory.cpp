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
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "UEInventory/Item.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"

#endif // !INVENTORY_HEADERS_H

static constexpr uint64  MaxColumns = 4;
static constexpr uint64  MaxRows = 3;

uint64 UInventory::ItemCounter = 1;

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

	TObjectPtr<UVerticalBox> VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
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
		TObjectPtr<UVerticalBoxSlot> TitleVerticalBoxSlot = VerticalBox->AddChildToVerticalBox(Title);
		if (TitleVerticalBoxSlot)
		{
			UE_LOG(LogTemp, Error, TEXT("TitleVSlot is valid"));
			// Center horizontally, add some top padding
			TitleVerticalBoxSlot->SetHorizontalAlignment(HAlign_Center);
			TitleVerticalBoxSlot->SetPadding(FMargin(0, 20, 0, 10));
		}
	}
	
	// Create instance of UUniformGridPanel at run time
	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	if (Grid)
	{
		UE_LOG(LogTemp, Error, TEXT("Grid is valid"));
		// Reduce padding in the grid itself to avoid extra spacing
		Grid->SetSlotPadding(FMargin(10, 16, 10, 16));  // Reducing slot padding to minimal

		TObjectPtr<UVerticalBoxSlot> GridVerticalBoxSlot = VerticalBox->AddChildToVerticalBox(Grid);
		if (GridVerticalBoxSlot)
		{
			UE_LOG(LogTemp, Error, TEXT("GridVSlot is valid"));
			// Fill remaining space in the VerticalBox
			GridVerticalBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
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
	
	Create(MaxRows, MaxColumns);
}

void UInventory::Create(uint64 Rows, uint64 Columns)
{
	if (!Grid)
	{
		UE_LOG(LogTemp, Warning, TEXT("Create: Grid is null"));
		return;
	}

	// Validate that the requested grid size matches MaxRows and MaxColumns
	if (Rows != MaxRows || Columns != MaxColumns)
	{
		UE_LOG(LogTemp, Warning, TEXT("Create: Requested grid size (%llu x %llu) does not match MaxRows x MaxColumns (%llu x %llu)"), Rows, Columns, MaxRows, MaxColumns);
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

			if (ForegroundBorders.Last())
			{
				ForegroundBorders.Last()->SetBrushColor(FLinearColor::Black); // Example border color

				// Add last slot border to the inventory grid
				GridSlot = Grid->AddChildToUniformGrid(ForegroundBorders.Last(), CurrentRow, CurrentColumn);

				if (GridSlot)
				{
					// Set the slot's alignment
					GridSlot->SetHorizontalAlignment(HAlign_Fill);
					GridSlot->SetVerticalAlignment(VAlign_Fill);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Create: Failed to create border for slot at Row=%llu, Col=%llu"), CurrentRow, CurrentColumn);
			}
		}
	}
	
	ItemCounter = 1;
}

void UInventory::AddItem(AActor* ItemActor)
{
	if (!ItemActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItem: ItemActor is null"));
		return;
	}

	if (bIsInventoryFull)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItem: Inventory is full"));
		return;
	}

	uint64 NewIndex = Items.Add(FItem());
	// Record the actor's class for later respawn
	Items[NewIndex].ReferencedActorClass = ItemActor->GetClass();

	if (Items[NewIndex].ReferencedActorClass)
	{
		UE_LOG(LogTemp, Log, TEXT("AddItem: ReferencedActorClass is valid")); // Changed LogLevel to Log
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItem: ReferencedActorClass is null for ItemActor"));
		Items.RemoveAt(NewIndex); // Clean up the added item
		return;
	}

	// Record its location and log it
	Items[NewIndex].WorldLocation = ItemActor->GetActorLocation();
	UE_LOG(LogTemp, Log, TEXT("AddItem: Storing WorldLocation for item at index %llu: %s"), NewIndex, *Items[NewIndex].WorldLocation.ToString());

	// Set a predefined screen position for the icon (e.g., inventory grid slot)
	Items[NewIndex].IconPosition = FVector2D(700, 400); // Example: Fixed position for simplicity
	SpawnItemIcon(Items[NewIndex].IconPosition); // Spawn the icon at this position

	if (!ItemActor->IsPendingKillPending())
	{
		// Remove the actor from the world
		ItemActor->Destroy();
	}
}

void UInventory::SpawnItemIcon(FVector2D ScreenPosition)
{
	uint64 LastItemIndex = Items.Num() - 1; // Get the index of the last added item
    if (LastItemIndex < 0 || !Items.IsValidIndex(LastItemIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Invalid item index %llu"), LastItemIndex);
        return;
    }

    if (LastItemIndex >= ForegroundBorders.Num() || !ForegroundBorders[LastItemIndex])
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Invalid ForegroundBorder at index %llu"), LastItemIndex);
        return;
    }

    // Create an overlay to stack the image and text
    TObjectPtr<UOverlay> IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    if (!IconOverlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Failed to create overlay"));
        return;
    }

    // Create a UImage widget
    Items[LastItemIndex].Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    if (!Items[LastItemIndex].Icon)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Icon Image is not valid"));
        return;
    }

    // Set the image properties
    Items[LastItemIndex].Icon->SetColorAndOpacity(FLinearColor::Blue);
    Items[LastItemIndex].Icon->SetVisibility(ESlateVisibility::Visible);
    Items[LastItemIndex].Icon->SetRenderScale(FVector2D(0.9f, 0.9f)); // Ensure no scaling happens

    // Add the image to the overlay, filling the space
    TObjectPtr<UOverlaySlot> ImageSlot = IconOverlay->AddChildToOverlay(Items[LastItemIndex].Icon);
    if (ImageSlot)
    {
        ImageSlot->SetHorizontalAlignment(HAlign_Fill);
        ImageSlot->SetVerticalAlignment(VAlign_Fill);
    }

    // Create a UTextBlock widget to display the counter on top of the item
    UTextBlock* ItemCounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (!ItemCounterText)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Counter TextBlock creation failed"));
        return;
    }

    // Set the counter text
    ItemCounterText->SetText(FText::AsNumber(ItemCounter)); // Display the current count
    ItemCounterText->SetColorAndOpacity(FLinearColor::Red); // Set the counter color
    ItemCounterText->SetVisibility(ESlateVisibility::Visible);
    ItemCounterText->SetJustification(ETextJustify::Center);

    TObjectPtr<UObject> RobotoFont = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto"));
    if (RobotoFont)
    {
        UE_LOG(LogTemp, Log, TEXT("SpawnItemIcon: Loaded font successfully")); // Changed LogLevel to Log
        ItemCounterText->SetFont(FSlateFontInfo(RobotoFont, 20.0f));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Failed to load font"));
    }

    // Add the text to the overlay, positioned bottom-right
    TObjectPtr<UOverlaySlot> TextOverlaySlot = IconOverlay->AddChildToOverlay(ItemCounterText);
    if (TextOverlaySlot)
    {
        TextOverlaySlot->SetHorizontalAlignment(HAlign_Right);
        TextOverlaySlot->SetVerticalAlignment(VAlign_Bottom);
        TextOverlaySlot->SetPadding(FMargin(55, 50, 50, 65));
    }

    // Set the overlay as the content of the border
    ForegroundBorders[LastItemIndex]->SetContent(IconOverlay);
    ForegroundBorders[LastItemIndex]->SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Log, TEXT("SpawnItemIcon: Text Translation: %s"), *ItemCounterText->GetRenderTransform().Translation.ToString());

    // Update bIsInventoryFull and ItemCounter
    if (LastItemIndex == MaxRows * MaxColumns - 1)
    {
        bIsInventoryFull = true;
    }

    if (ItemCounter <= 12)
    {
	    ItemCounter++;
    }
    else
    {
    	ItemCounter = 1;
    }
}

FItem UInventory::RemoveItem()
{
	return FItem();
}

void UInventory::MoveItem()
{
	// Convert ItemCounter (1-based) to 0-based index
	uint64 FromIndex = ItemCounter - 1;

	// Ensure FromIndex is valid
	if (!Items.IsValidIndex(FromIndex))
	{
		return;
	}

	// Determine the direction (example: moving right by one slot)
	uint64 ToIndex = FromIndex + 1; // Change this logic to move in other directions

	// Ensure ToIndex is valid
	if (!Items.IsValidIndex(ToIndex))
	{
		return;
	}
	
}

void UInventory::Open()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UInventory::Close()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

TArray<FItem>& UInventory::GetItems()
{
	return Items;
}

bool UInventory::GetIsInventoryFull() 
{
    return bIsInventoryFull;
}


void UInventory::SortItems(FItem MovedItem, FItem ItemToMove, EDirection Direction)
{
}

void UInventory::GetItemIndexAtPosition(FVector2D ScreenPosition, uint64& OutIndex)
{
	for (uint64 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Index >= ForegroundBorders.Num() || !ForegroundBorders[Index]) continue;

		// Get the geometry of the icon slot
		FGeometry IconGeometry = ForegroundBorders[Index]->GetCachedGeometry();
		FVector2D LocalPosition = IconGeometry.AbsoluteToLocal(ScreenPosition);

		// Check if the mouse position is within the bounds of the icon
		FVector2D IconSize = IconGeometry.GetLocalSize();
		if (LocalPosition.X >= 0 && LocalPosition.X <= IconSize.X &&
			LocalPosition.Y >= 0 && LocalPosition.Y <= IconSize.Y)
		{
			OutIndex = Index;
			return; // Exit early once a match is found
		}
	}
	// No valid index found; OutIndex remains unchanged
}


uint64 UInventory::GetMaxRows()
{
	return MaxRows;
}

uint64 UInventory::GetMaxColumns()
{
	return MaxColumns;
}
