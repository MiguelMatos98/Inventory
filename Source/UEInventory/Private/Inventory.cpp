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
#include "Blueprint/UserWidget.h"

#endif // !INVENTORY_HEADERS_H

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

int64 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent)
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController) return INDEX_NONE;

	FVector2D ScreenMousePosition;
	if (PlayerController->GetMousePosition(ScreenMousePosition.X, ScreenMousePosition.Y))
	{
		UE_LOG(LogTemp, Log, TEXT("Screen Mouse Position (Viewport Space): X=%f Y=%f"), ScreenMousePosition.X, ScreenMousePosition.Y);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get mouse position!"));
		return INDEX_NONE;
	}

	// Adjust screen position by applying the correct scaling (if needed)
	UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
	if (ViewportClient)
	{
		FIntPoint MouseIntPoint;
		ViewportClient->Viewport->GetMousePos(MouseIntPoint);
		ScreenMousePosition = FVector2D(MouseIntPoint);
	}

	for (int32 Index = 0; Index < ForegroundBorders.Num(); ++Index)
	{
		if (ForegroundBorders[Index])
		{
			FGeometry BorderGeometry = ForegroundBorders[Index]->GetCachedGeometry();
			FVector2D BorderTopLeft = BorderGeometry.GetAbsolutePosition();
			FVector2D BorderSize = BorderGeometry.GetLocalSize();
			FVector2D BorderBottomRight = BorderTopLeft + BorderSize;

			UE_LOG(LogTemp, Log, TEXT("Border[%d] - TopLeft: X=%f Y=%f, BottomRight: X=%f Y=%f"), Index, BorderTopLeft.X, BorderTopLeft.Y, BorderBottomRight.X, BorderBottomRight.Y);

			// Check if the mouse is within the bounds of the current border (slot)
			if (ScreenMousePosition.X >= BorderTopLeft.X && ScreenMousePosition.X <= BorderBottomRight.X &&
				ScreenMousePosition.Y >= BorderTopLeft.Y && ScreenMousePosition.Y <= BorderBottomRight.Y)
			{
				UE_LOG(LogTemp, Log, TEXT("Hovered Index Found: %d"), Index);
				return Index;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("No hovered index found!"));
	return INDEX_NONE;
}


void UInventory::MoveItem(const FPointerEvent& InMouseEvent, bool bStartMove, bool bEndMove)
{
    int32 MovingIndex = INDEX_NONE;

    // Find the currently selected item
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i].bIsSelected)
        {
            MovingIndex = i;
            break;
        }
    }

    if (bStartMove && MovingIndex == INDEX_NONE) // Start moving an item
    {
        int32 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
        if (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex))
        {
            // Deselect all items first
            for (FItem& Item : Items)
            {
                Item.bIsSelected = false;
            }

            Items[HoveredIndex].bIsSelected = true;
            UE_LOG(LogTemp, Warning, TEXT("Started moving item %d"), HoveredIndex);
            return;
        }
    }
    else if (!bStartMove && !bEndMove && MovingIndex != INDEX_NONE) // Update position while moving
    {
        if (ForegroundBorders.IsValidIndex(MovingIndex) && IconSlots.IsValidIndex(MovingIndex))
        {
            FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
            FVector2D LocalPosition = GetCachedGeometry().AbsoluteToLocal(ScreenPosition);

            Items[MovingIndex].IconPosition = LocalPosition;

            UCanvasPanelSlot* TargetSlot = IconSlots[MovingIndex];
            if (TargetSlot)
            {
                TargetSlot->SetPosition(LocalPosition);
            }
        }
    }
    else if (bEndMove && MovingIndex != INDEX_NONE) // End movement
    {
        int32 TargetIndex = FindHoveredItemIndex(InMouseEvent);
        if (TargetIndex != INDEX_NONE && Items.IsValidIndex(TargetIndex) && TargetIndex != MovingIndex)
        {
            SortItem(Items[MovingIndex], Items[TargetIndex]);

            // Swap ForegroundBorders positions
            if (ForegroundBorders.IsValidIndex(MovingIndex) && ForegroundBorders.IsValidIndex(TargetIndex))
            {
                UBorder* BorderA = ForegroundBorders[MovingIndex];
                UBorder* BorderB = ForegroundBorders[TargetIndex];

                if (BorderA && BorderB)
                {
                    UCanvasPanelSlot* SlotA = Cast<UCanvasPanelSlot>(BorderA->Slot);
                    UCanvasPanelSlot* SlotB = Cast<UCanvasPanelSlot>(BorderB->Slot);

                    if (SlotA && SlotB)
                    {
                        FVector2D TempPos = SlotA->GetPosition();
                        SlotA->SetPosition(SlotB->GetPosition());
                        SlotB->SetPosition(TempPos);
                    }
                }
            }

            UE_LOG(LogTemp, Warning, TEXT("Moved item from %d to %d"), MovingIndex, TargetIndex);
        }

        Items[MovingIndex].bIsSelected = false;
    }
}

void UInventory::RemoveItem()
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

TArray<TObjectPtr<UBorder>> UInventory::GetForegroundBorders()
{
	return ForegroundBorders;
}

TObjectPtr<UUniformGridPanel> UInventory::GetGrid()
{
	return Grid;
}

