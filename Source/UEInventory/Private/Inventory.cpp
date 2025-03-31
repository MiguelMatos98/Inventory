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
    WidgetTree->RootWidget = Canvas;

    BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (!BackgroundBorder)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundBorder is invalid"));
        return;
    }
    BackgroundBorder->SetBrushColor(FLinearColor::Gray);

    UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (!VerticalBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("VerticalBox is invalid"));
        return;
    }

    Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (Title)
    {
        Title->SetText(FText::FromString(TEXT("Inventory")));
        if (UVerticalBoxSlot* TitleVSlot = VerticalBox->AddChildToVerticalBox(Title))
        {
            TitleVSlot->SetHorizontalAlignment(HAlign_Center);
            TitleVSlot->SetPadding(FMargin(0, 20, 0, 10));
        }
    }

    Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    if (Grid)
    {
        Grid->SetSlotPadding(FMargin(10, 10, 10, 10));
        if (UVerticalBoxSlot* GridVSlot = VerticalBox->AddChildToVerticalBox(Grid))
        {
            GridVSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    BackgroundBorder->SetContent(VerticalBox);
    Canvas->AddChild(BackgroundBorder);

    BackgroundBorderSlot = Cast<UCanvasPanelSlot>(BackgroundBorder->Slot);
    if (BackgroundBorderSlot)
    {
        BackgroundBorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        BackgroundBorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        BackgroundBorderSlot->SetOffsets(FMargin(0, 0, 510.0f, 500.0f));
    }

    // Reset render scale to 1:1 to ensure correct position calculations
    SetRenderScale(FVector2D(1.0f, 1.0f));

    // Initialize drag state
    DraggedItemIndex = INDEX_NONE;
    bIsDragging = false;

    Create(MaxRows, MaxColumns);
}

void UInventory::Create(uint64 Rows, uint64 Columns)
{
    if (!Grid)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid is null in Create()"));
        return;
    }
    
    Grid->ClearChildren();
    ForegroundBorders.Empty();

    // Create the grid slots and populate ForegroundBorders.
    for (uint64 CurrentRow = 0; CurrentRow < Rows; ++CurrentRow)
    {
        for (uint64 CurrentColumn = 0; CurrentColumn < Columns; ++CurrentColumn)
        {
            TObjectPtr<UBorder> NewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
            if (NewBorder)
            {
                NewBorder->SetBrushColor(FLinearColor::Black);
                ForegroundBorders.Add(NewBorder);
                
                if (GridSlot = Grid->AddChildToUniformGrid(NewBorder, CurrentRow, CurrentColumn))
                {
                    GridSlot->SetHorizontalAlignment(HAlign_Fill);
                    GridSlot->SetVerticalAlignment(VAlign_Fill);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to create Border at Row %llu, Column %llu"), CurrentRow, CurrentColumn);
            }
        }
    }
    // Preallocate the Items array so that it matches the grid size.
    Items.SetNum(Rows * Columns);
    ItemCounter = 1;
}

FReply UInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        MoveItem(InMouseEvent, true, false);
        if (bIsDragging)
        {
            UE_LOG(LogTemp, Log, TEXT("Started dragging item."));
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsDragging && DraggedItemIndex != INDEX_NONE)
    {
        MoveItem(InMouseEvent, false, false);
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        MoveItem(InMouseEvent, false, true);
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UInventory::AddItem(AActor* ItemActor)
{
    if (!ItemActor)
    {
        return;
    }

    // Find the first empty slot.
    uint64 EmptySlotIndex = FindFirstEmptySlot();

    // If no empty slot is found, the inventory is full.
    if (EmptySlotIndex == INDEX_NONE)
    {
        bIsInventoryFull = true;
        return;
    }

    // Assign the new item to the empty slot.
    Items[EmptySlotIndex].ReferencedActorClass = ItemActor->GetClass();
    Items[EmptySlotIndex].WorldLocation = ItemActor->GetActorLocation();

    // Create the UI icon and counter for this slot.
    CreateItemIcon(EmptySlotIndex);
    CreateIconCounterText(EmptySlotIndex);
    CreateIconCounterText(EmptySlotIndex);

    ItemActor->Destroy();

    // If the last slot was just filled, mark inventory as full.
    if (EmptySlotIndex == Items.Num() - 1)
    {
        bIsInventoryFull = true;
    }
    
    // Update the item counter for display (if used).
    ItemCounter = (ItemCounter % 12) + 1;
}

uint64 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent)
{
    // Force layout on Grid to ensure geometry is updated.
    if (Grid)
    {
        Grid->ForceLayoutPrepass();
    }

    FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
    UE_LOG(LogTemp, Log, TEXT("Screen Mouse Position (Viewport): X=%f, Y=%f"), MousePos.X, MousePos.Y);

    if (!Grid || ForegroundBorders.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid or ForegroundBorders are missing!"));
        return INDEX_NONE;
    }

    FGeometry GridGeometry = Grid->GetCachedGeometry();
    FVector2D GridAbsTopLeft = GridGeometry.LocalToAbsolute(FVector2D::ZeroVector);
    FVector2D GridAbsSize = GridGeometry.GetLocalSize();
    FVector2D GridAbsBottomRight = GridAbsTopLeft + GridAbsSize;
    UE_LOG(LogTemp, Log, TEXT("Grid TopLeft: X=%f, Y=%f, Size: X=%f, Y=%f"),
           GridAbsTopLeft.X, GridAbsTopLeft.Y, GridAbsSize.X, GridAbsSize.Y);

    const float Tolerance = 10.f;
    if (MousePos.X < GridAbsTopLeft.X - Tolerance || MousePos.X > GridAbsBottomRight.X + Tolerance ||
        MousePos.Y < GridAbsTopLeft.Y - Tolerance || MousePos.Y > GridAbsBottomRight.Y + Tolerance)
    {
        UE_LOG(LogTemp, Warning, TEXT("Mouse outside grid boundaries."));
        return INDEX_NONE;
    }

    for (uint64 i = 0; i < ForegroundBorders.Num(); i++)
    {
        if (!ForegroundBorders[i])
            continue;

        FGeometry BorderGeometry = ForegroundBorders[i]->GetCachedGeometry();
        FVector2D BorderAbsTopLeft = BorderGeometry.LocalToAbsolute(FVector2D::ZeroVector);
        FVector2D BorderAbsSize = BorderGeometry.GetLocalSize();
        FVector2D BorderAbsBottomRight = BorderAbsTopLeft + BorderAbsSize;

        UE_LOG(LogTemp, Log, TEXT("Border[%d] - TopLeft: X=%f, Y=%f; BottomRight: X=%f, Y=%f"),
               i, BorderAbsTopLeft.X, BorderAbsTopLeft.Y, BorderAbsBottomRight.X, BorderAbsBottomRight.Y);

        if (MousePos.X >= BorderAbsTopLeft.X - Tolerance && MousePos.X <= BorderAbsBottomRight.X + Tolerance &&
            MousePos.Y >= BorderAbsTopLeft.Y - Tolerance && MousePos.Y <= BorderAbsBottomRight.Y + Tolerance)
        {
            UE_LOG(LogTemp, Log, TEXT("Mouse is over Border[%d]"), i);
            return i;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("No hovered index found."));
    return INDEX_NONE;
}

void UInventory::CreateItemIcon(uint64 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !ForegroundBorders.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid item or border index %d"), SlotIndex);
        return;
    }

     TObjectPtr<UOverlay> IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    if (!IconOverlay) return;

     TObjectPtr<UImage> ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    if (!ItemIcon) return;

    if (Items[SlotIndex].IconTexture.IsValid())
    {
        ItemIcon->SetBrushFromTexture(Items[SlotIndex].IconTexture.Get());
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

    ForegroundBorders[SlotIndex]->SetContent(IconOverlay);
    ForegroundBorders[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
}

void UInventory::CreateIconCounterText(uint64 SlotIndex)
{
    if (!ForegroundBorders.IsValidIndex(SlotIndex) || !ForegroundBorders[SlotIndex]->GetContent())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid border or missing content at index %d"), SlotIndex);
        return;
    }

    TObjectPtr<UOverlay> IconOverlay = Cast<UOverlay>(ForegroundBorders[SlotIndex]->GetContent());
    if (!IconOverlay) return;

    UTextBlock* ItemCounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (!ItemCounterText) return;

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

uint64 UInventory::FindFirstEmptySlot() const
{
    for (uint64 i = 0; i < Items.Num(); i++)
    {
        if (!Items[i].ReferencedActorClass) // Slot is empty
        {
            return i;
        }
    }
    return INDEX_NONE; // No empty slot found
}

TObjectPtr<UOverlay> UInventory::FindDraggedOverlay(uint64 ItemIndex)
{
    if (!Canvas)
    {
        UE_LOG(LogTemp, Warning, TEXT("Canvas is null in FindDraggedOverlay"));
        return nullptr;
    }

    // Loop through all children of the canvas.
    for (uint64 i = 0; i < Canvas->GetChildrenCount(); i++)
    {
        TObjectPtr<UOverlay> ChildOverlay = Cast<UOverlay>(Canvas->GetChildAt(i));
        if (ChildOverlay)
        {
            // Make sure there is at least one child before accessing index 0.
            if (ChildOverlay->GetChildrenCount() > 0)
            {
                TObjectPtr<UImage> IconImage = Cast<UImage>(ChildOverlay->GetChildAt(0));
                if (IconImage)
                {
                    TObjectPtr<UTexture2D> Texture = Cast<UTexture2D>(IconImage->GetBrush().GetResourceObject());
                    if (Texture == Items[ItemIndex].IconTexture.Get())
                    {
                        return ChildOverlay;
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("ChildOverlay[%d] first child is not an image."), i);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("ChildOverlay[%d] has no children."), i);
            }
        }
    }
    return nullptr;
}

void UInventory::MoveItem(const FPointerEvent& InMouseEvent, bool bStartMove, bool bEndMove)
{
    uint64 MovingIndex = DraggedItemIndex;
    FGeometry CanvasGeometry = Canvas->GetCachedGeometry();

    if (bStartMove && MovingIndex == INDEX_NONE)
    {
        uint64 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
        if (HoveredIndex == INDEX_NONE)
        {
            UE_LOG(LogTemp, Warning, TEXT("No hovered index found on drag start."));
            return;
        }

        if (ForegroundBorders.IsValidIndex(HoveredIndex) && ForegroundBorders[HoveredIndex]->GetContent())
        {
            // Deselect all items; then (for this example) select the first item.
            for (FItem& Item : Items)
            {
                Item.bIsSelected = false;
            }
            if (Items.Num() > 0)
            {
                Items[0].bIsSelected = true;
            }
            DraggedItemIndex = HoveredIndex;
            bIsDragging = true;
            
            TObjectPtr<UBorder> Border = ForegroundBorders[HoveredIndex];
            if (Border)
            {
                TObjectPtr<UOverlay> OverlayToDrag = Cast<UOverlay>(Border->GetContent());
                if (OverlayToDrag)
                {
                    Border->SetContent(nullptr);
                    Canvas->AddChild(OverlayToDrag);

                    if (UCanvasPanelSlot* DraggedSlot = Cast<UCanvasPanelSlot>(OverlayToDrag->Slot))
                    {
                        DraggedSlot->SetSize(FVector2D(100.0f, 100.0f));
                        DraggedSlot->SetZOrder(100);

                        FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
                        FVector2D LocalPos = CanvasGeometry.AbsoluteToLocal(ScreenPos) - FVector2D(1.0f, 1.0f);
                        DraggedSlot->SetPosition(LocalPos);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("OverlayToDrag not found at hovered index %d"), HoveredIndex);
                }
            }
            UE_LOG(LogTemp, Log, TEXT("Started moving item from slot %d"), HoveredIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Cannot start drag at index %d: invalid index or no content."), HoveredIndex);
        }
    }
    else if (!bStartMove && !bEndMove && MovingIndex != INDEX_NONE)
    {
         TObjectPtr<UOverlay> OverlayToDrag = FindDraggedOverlay(MovingIndex);
        if (OverlayToDrag)
        {
            FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
            FVector2D LocalPos = CanvasGeometry.AbsoluteToLocal(ScreenPos);
            TObjectPtr<UCanvasPanelSlot> DraggedSlot = Cast<UCanvasPanelSlot>(OverlayToDrag->Slot);
            if (DraggedSlot)
            {
                LocalPos -= FVector2D(1.0f, 1.0f);
                DraggedSlot->SetPosition(LocalPos);
                UE_LOG(LogTemp, Log, TEXT("Dragging item from slot %d to X=%f, Y=%f"), MovingIndex, LocalPos.X, LocalPos.Y);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Dragged overlay not found during drag update for slot %d"), MovingIndex);
        }
    }
    else if (bEndMove && MovingIndex != INDEX_NONE)
    {
        uint64 TargetIndex = FindHoveredItemIndex(InMouseEvent);
        UE_LOG(LogTemp, Log, TEXT("Drop detected. MovingIndex: %d, TargetIndex: %d"), MovingIndex, TargetIndex);

        if (TargetIndex != INDEX_NONE && ForegroundBorders.IsValidIndex(TargetIndex) && TargetIndex != MovingIndex)
        {
            TObjectPtr<UOverlay> DraggedOverlay = FindDraggedOverlay(MovingIndex);
             TObjectPtr<UOverlay> TargetOverlay = FindDraggedOverlay(TargetIndex);

            if (!DraggedOverlay)
            {
                UE_LOG(LogTemp, Warning, TEXT("Dragged overlay not found during drop for slot %d"), MovingIndex);
                bIsDragging = false;
                DraggedItemIndex = INDEX_NONE;
                return;
            }

            // Remove the dragged overlay from canvas if still present.
            if (Canvas->HasChild(DraggedOverlay))
            {
                Canvas->RemoveChild(DraggedOverlay);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Canvas does not contain dragged overlay for slot %d"), MovingIndex);
            }

            if (ForegroundBorders.IsValidIndex(MovingIndex) && ForegroundBorders.IsValidIndex(TargetIndex))
            {
                // Swap UI: if TargetOverlay is null, clear that slot.
                ForegroundBorders[MovingIndex]->SetContent(TargetOverlay ? TargetOverlay : nullptr);
                ForegroundBorders[TargetIndex]->SetContent(DraggedOverlay);
                ForegroundBorders[MovingIndex]->SetVisibility(ESlateVisibility::Visible);
                ForegroundBorders[TargetIndex]->SetVisibility(ESlateVisibility::Visible);

                if (Items.IsValidIndex(MovingIndex) && Items.IsValidIndex(TargetIndex))
                {
                    SortItem(Items[MovingIndex], Items[TargetIndex]);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Invalid inventory indices for swap: %d, %d"), MovingIndex, TargetIndex);
                }
                UE_LOG(LogTemp, Log, TEXT("Swapped item from slot %d to slot %d"), MovingIndex, TargetIndex);
                ForegroundBorders[TargetIndex]->InvalidateLayoutAndVolatility();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Invalid ForegroundBorders indices during drop: %d, %d"), MovingIndex, TargetIndex);
            }
        }
        else
        {
            // If target index is invalid, restore the dragged overlay to its original slot.
            TObjectPtr<UOverlay> OverlayToRestore = FindDraggedOverlay(MovingIndex);
            if (OverlayToRestore && ForegroundBorders.IsValidIndex(MovingIndex))
            {
                if (Canvas->HasChild(OverlayToRestore))
                {
                    Canvas->RemoveChild(OverlayToRestore);
                }
                ForegroundBorders[MovingIndex]->SetContent(OverlayToRestore);
                ForegroundBorders[MovingIndex]->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Unable to restore overlay for slot %d"), MovingIndex);
            }
            UE_LOG(LogTemp, Log, TEXT("Drag canceled or invalid target slot; restored original slot."));
        }
        // Reset drag state.
        bIsDragging = false;
        DraggedItemIndex = INDEX_NONE;
    }
}

void UInventory::RemoveItem()
{
}

EDirection UInventory::GetMoveDirection(uint64 RowA, uint64 ColA, uint64 RowB, uint64 ColB)
{
    if (RowA == RowB)
    {
        if (ColB == ColA + 1) return EDirection::Right;
        if (ColB == ColA - 1) return EDirection::Left;
    }
    else if (ColA == ColB)
    {
        if (RowB == RowA + 1) return EDirection::Down;
        if (RowB == RowA - 1) return EDirection::Up;
    }
    
    return static_cast<EDirection>(255); // Invalid move
}

void UInventory::SortItem(FItem MovedItem, FItem ItemToMove)
{
    int32 IndexA = Items.IndexOfByKey(MovedItem);
    int32 IndexB = Items.IndexOfByKey(ItemToMove);

    if (IndexA == INDEX_NONE || IndexB == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("SortItem failed: Invalid item indices."));
        return;
    }

    // Calculate row and column for each item based on MaxColumns
    int32 RowA = IndexA / MaxColumns;
    int32 ColA = IndexA % MaxColumns;
    int32 RowB = IndexB / MaxColumns;
    int32 ColB = IndexB % MaxColumns;

    EDirection MoveDirection = GetMoveDirection(RowA, ColA, RowB, ColB);

    if (MoveDirection == EDirection::Up || MoveDirection == EDirection::Down || MoveDirection == EDirection::Left || MoveDirection == EDirection::Right)
    {
        // Swap the items in the inventory list only
        Swap(Items[IndexA], Items[IndexB]);

        UE_LOG(LogTemp, Log, TEXT("Sorted item from (%d, %d) to (%d, %d)"), RowA, ColA, RowB, ColB);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid move detected. No sorting performed."));
    }

    UE_LOG(LogTemp, Log, TEXT("Sorted item from (%d, %d) to (%d, %d)"), RowA, ColA, RowB, ColB);
}


void UInventory::Open()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UInventory::Close()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

bool UInventory::GetIsInventoryFull() const
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

