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
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
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

	bIsInventoryFull = false;

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
	
	Create(MaxRows, MaxColumns);

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

	ItemCounter = 1;
}

void UInventory::AddItem(AActor* ItemActor)
{
	if (!ItemActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemActor is null"));
		return;
	}

	uint64 NewIndex = Items.Add(FItem());
	// Record the actor's class for later respawn
	Items[NewIndex].ReferencedActorClass = ItemActor->GetClass();

	if (Items[NewIndex].ReferencedActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReferencedActorClass is valid"));	
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItem: ReferencedActorClass is null for ItemActor"));
		Items.RemoveAt(NewIndex); // Clean up the added item
		return;
		UE_LOG(LogTemp, Warning, TEXT("ReferencedActorClass is valid"));	
	}
	
	// Record its location
	Items[NewIndex].WorldLocation = ItemActor->GetActorLocation();

	// Set a predefined screen position for the icon (e.g., inventory grid slot)
	Items[NewIndex].IconPosition = FVector2D(700, 400); // Example: Fixed position for simplicity
	SpawnItemIcon(Items[NewIndex].IconPosition); // Spawn the icon at this position

	if (!ItemActor->IsPendingKillPending())
	{
		// Remove the actor from the world
		ItemActor->Destroy();
	}

	if (Items.Num() >= 12)
	{
		bIsInventoryFull = true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Item stored at: %s"), *Items[NewIndex].WorldLocation.ToString());
}

void UInventory::SpawnItemIcon(FVector2D ScreenPosition)
{
	const int32 LastItemIndex = Items.Num() - 1;
    if (!Items.IsValidIndex(LastItemIndex) || !ForegroundBorders.IsValidIndex(LastItemIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Invalid item or border index %d"), LastItemIndex);
        return;
    }

    TObjectPtr<UOverlay> IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    if (!IconOverlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Failed to create overlay"));
        return;
    }

    TObjectPtr<UImage> ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    if (!ItemIcon)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnItemIcon: Icon Image is not valid"));
        return;
    }

    // Assign texture if available
    if (Items[LastItemIndex].IconTexture.IsValid())
    {
        ItemIcon->SetBrushFromTexture(Items[LastItemIndex].IconTexture.Get());
    }
    else
    {
        ItemIcon->SetColorAndOpacity(FLinearColor::Blue); // Placeholder color
    }

    TObjectPtr<UOverlaySlot> ImageSlot = IconOverlay->AddChildToOverlay(ItemIcon);
    if (ImageSlot)
    {
        ImageSlot->SetHorizontalAlignment(HAlign_Fill);
        ImageSlot->SetVerticalAlignment(VAlign_Fill);
    }

    UTextBlock* ItemCounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (ItemCounterText)
    {
        ItemCounterText->SetText(FText::AsNumber(ItemCounter));
        ItemCounterText->SetColorAndOpacity(FLinearColor::Red);
        ItemCounterText->SetJustification(ETextJustify::Center);
        ItemCounterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));

        TObjectPtr<UOverlaySlot> TextOverlaySlot = IconOverlay->AddChildToOverlay(ItemCounterText);
        if (TextOverlaySlot)
        {
            TextOverlaySlot->SetHorizontalAlignment(HAlign_Right);
            TextOverlaySlot->SetVerticalAlignment(VAlign_Bottom);
            TextOverlaySlot->SetPadding(FMargin(55, 50, 50, 65));
        }
    }

    ForegroundBorders[LastItemIndex]->SetContent(IconOverlay);
    ForegroundBorders[LastItemIndex]->SetVisibility(ESlateVisibility::Visible);

    if (LastItemIndex == MaxRows * MaxColumns - 1)
    {
        bIsInventoryFull = true;
    }

    ItemCounter = (ItemCounter % 12) + 1;
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

bool UInventory::GetIsInventoryFull()
{
	return bIsInventoryFull;
}

TArray<FItem>& UInventory::GetItems()
{
	return Items;
}

