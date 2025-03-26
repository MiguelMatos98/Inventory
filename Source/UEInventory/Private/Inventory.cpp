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
        Grid->SetSlotPadding(FMargin(10, 16, 10, 16));
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
    if (!Grid) return;

    Grid->ClearChildren();
    ForegroundBorders.Empty();

    for (uint64 CurrentRow = 0; CurrentRow < Rows; ++CurrentRow)
    {
        for (uint64 CurrentColumn = 0; CurrentColumn < Columns; ++CurrentColumn)
        {
            ForegroundBorders.Add(WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()));
            ForegroundBorders.Last()->SetBrushColor(FLinearColor::Black);

            GridSlot = Grid->AddChildToUniformGrid(ForegroundBorders.Last(), CurrentRow, CurrentColumn);
            if (GridSlot)
            {
                GridSlot->SetHorizontalAlignment(HAlign_Fill);
                GridSlot->SetVerticalAlignment(VAlign_Fill);
            }
        }
    }
    ItemCounter = 1;
}

FReply UInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        MoveItem(InMouseEvent, true, false);
        if (bIsDragging)
        {
            UE_LOG(LogTemp, Log, TEXT("Dragging is true on mouse button down method"));
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    UE_LOG(LogTemp, Log, TEXT("Mouse Button Down Called"));
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsDragging && DraggedItemIndex != INDEX_NONE)
    {
        MoveItem(InMouseEvent, false, false); // Update dragged item position
        // Add logging and hovered slot detection
        FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
        UE_LOG(LogTemp, Log, TEXT("Dragging - Screen Mouse Position: X=%f Y=%f"), MousePos.X, MousePos.Y);
        int32 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
        if (HoveredIndex != INDEX_NONE)
        {
            UE_LOG(LogTemp, Log, TEXT("Hovered slot index: %d"), HoveredIndex);
        }
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
	if (!ItemActor || Items.Num() >= MaxRows * MaxColumns) return;

	uint64 NewIndex = Items.Add(FItem());
	Items[NewIndex].ReferencedActorClass = ItemActor->GetClass();

	if (Items[NewIndex].ReferencedActorClass)
	{
		Items[NewIndex].WorldLocation = ItemActor->GetActorLocation();
		Items[NewIndex].IconPosition = FVector2D(700, 400); // Example: Fixed position for simplicity
		SpawnItemIcon(Items[NewIndex].IconPosition);

		ItemActor->Destroy();
	}

	if (Items.Num() >= MaxRows * MaxColumns)
	{
		bIsInventoryFull = true;
	}
}

void UInventory::SpawnItemIcon(FVector2D ScreenPosition)
{
  const int32 LastItemIndex = Items.Num() - 1;
    if (!Items.IsValidIndex(LastItemIndex) || !ForegroundBorders.IsValidIndex(LastItemIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid item or border index %d"), LastItemIndex);
        return;
    }

    TObjectPtr<UOverlay> IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    TObjectPtr<UImage> ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    
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

    if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(ForegroundBorders[LastItemIndex]->Slot))
    {
        IconSlots.Add(PanelSlot);
    }

    if (LastItemIndex == MaxRows * MaxColumns - 1)
    {
        bIsInventoryFull = true;
    }

    ItemCounter = (ItemCounter % 12) + 1;
}

int64 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent)
{
    if (Grid)
    {
        Grid->ForceLayoutPrepass();
    }

    FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
    UE_LOG(LogTemp, Log, TEXT("Screen Mouse Position (Viewport Space): X=%f Y=%f"), MousePos.X, MousePos.Y);

    if (!Grid || ForegroundBorders.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid or ForegroundBorders are missing!"));
        return INDEX_NONE;
    }

    // Log viewport size
    FVector2D ViewportSize = FVector2D::ZeroVector;
    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        UE_LOG(LogTemp, Log, TEXT("Viewport Size: X=%f Y=%f"), ViewportSize.X, ViewportSize.Y);
    }

    // Get widget geometry
    FGeometry WidgetGeometry = GetCachedGeometry();
    FVector2D WidgetSize = WidgetGeometry.GetLocalSize();
    UE_LOG(LogTemp, Log, TEXT("Widget Render Transform Scale: X=%f Y=%f"), WidgetSize.X, WidgetSize.Y);
    if (WidgetSize == FVector2D::ZeroVector)
    {
        UE_LOG(LogTemp, Warning, TEXT("Widget geometry has zero size!"));
        return INDEX_NONE;
    }
    FVector2D WidgetTopLeft = WidgetGeometry.LocalToAbsolute(FVector2D(0, 0));
    FVector2D WidgetBottomRight = WidgetTopLeft + WidgetSize;
    WidgetBottomRight.X = FMath::Min(WidgetBottomRight.X, WidgetTopLeft.X + ViewportSize.X);
    WidgetBottomRight.Y = FMath::Min(WidgetBottomRight.Y, WidgetTopLeft.Y + ViewportSize.Y);
    UE_LOG(LogTemp, Log, TEXT("Widget Absolute TopLeft: X=%f Y=%f, Size: X=%f Y=%f"), 
        WidgetTopLeft.X, WidgetTopLeft.Y, WidgetSize.X, WidgetSize.Y);

    // Check widget bounds
    if (MousePos.X < WidgetTopLeft.X || MousePos.X > WidgetBottomRight.X ||
        MousePos.Y < WidgetTopLeft.Y || MousePos.Y > WidgetBottomRight.Y)
    {
        UE_LOG(LogTemp, Warning, TEXT("Mouse position is outside widget boundaries!"));
        return INDEX_NONE;
    }

    // Get grid geometry
    FGeometry GridGeometry = Grid->GetCachedGeometry();
    FVector2D GridSize = GridGeometry.GetLocalSize();
    if (GridSize == FVector2D::ZeroVector)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid geometry has zero size!"));
        return INDEX_NONE;
    }
    FVector2D GridTopLeft = GridGeometry.LocalToAbsolute(FVector2D(0, 0));
    FVector2D GridBottomRight = GridTopLeft + GridSize;
    UE_LOG(LogTemp, Log, TEXT("Grid Absolute TopLeft: X=%f Y=%f, Size: X=%f Y=%f"), 
        GridTopLeft.X, GridTopLeft.Y, GridSize.X, GridSize.Y);

    // Check grid bounds
    if (MousePos.X < GridTopLeft.X || MousePos.X > GridBottomRight.X ||
        MousePos.Y < GridTopLeft.Y || MousePos.Y > GridBottomRight.Y)
    {
        UE_LOG(LogTemp, Log, TEXT("Mouse position is outside grid boundaries!"));
        return INDEX_NONE;
    }

    // Check each slot (border)
    for (int32 i = 0; i < ForegroundBorders.Num(); i++)
    {
        if (!ForegroundBorders[i]) continue;

        FGeometry BorderGeometry = ForegroundBorders[i]->GetCachedGeometry();
        FVector2D BorderSize = BorderGeometry.GetLocalSize();
        if (BorderSize == FVector2D::ZeroVector)
        {
            UE_LOG(LogTemp, Warning, TEXT("Border[%d] geometry has zero size!"), i);
            continue;
        }
        FVector2D BorderTopLeft = BorderGeometry.LocalToAbsolute(FVector2D(0, 0));
        FVector2D BorderBottomRight = BorderTopLeft + BorderSize;

        UE_LOG(LogTemp, Log, TEXT("Border[%d] - Absolute TopLeft: X=%f Y=%f, BottomRight: X=%f Y=%f"), 
            i, BorderTopLeft.X, BorderTopLeft.Y, BorderBottomRight.X, BorderBottomRight.Y);

        if (MousePos.X >= BorderTopLeft.X && MousePos.X <= BorderBottomRight.X &&
            MousePos.Y >= BorderTopLeft.Y && MousePos.Y <= BorderBottomRight.Y)
        {
            UE_LOG(LogTemp, Log, TEXT("Mouse is over Border[%d]!"), i);
            return i;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("No hovered index found!"));
    return INDEX_NONE;
}

void UInventory::MoveItem(const FPointerEvent& InMouseEvent, bool bStartMove, bool bEndMove)
{
  int32 MovingIndex = DraggedItemIndex;

    if (bStartMove && MovingIndex == INDEX_NONE)
    {
        int32 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
        if (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex))
        {
            for (FItem& Item : Items)
            {
                Item.bIsSelected = false;
            }
            Items[HoveredIndex].bIsSelected = true;
            DraggedItemIndex = HoveredIndex;
            bIsDragging = true;

            // Reparent the existing overlay to the root Canvas
            if (ForegroundBorders.IsValidIndex(HoveredIndex))
            {
                UBorder* Border = ForegroundBorders[HoveredIndex];
                if (Border)
                {
                    UOverlay* OverlayToDrag = Cast<UOverlay>(Border->GetContent());
                    if (OverlayToDrag)
                    {
                        // Remove the overlay from the border
                        Border->SetContent(nullptr);
                        // Add it to the root Canvas
                        Canvas->AddChild(OverlayToDrag);
                        if (UCanvasPanelSlot* DraggedSlot = Cast<UCanvasPanelSlot>(OverlayToDrag->Slot))
                        {
                            DraggedSlot->SetSize(FVector2D(50.0f, 50.0f)); // Adjust size as needed
                            DraggedSlot->SetZOrder(100); // Ensure it’s on top

                            // Set initial position
                            FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
                            FVector2D LocalPosition = GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
                            LocalPosition -= FVector2D(25.0f, 25.0f); // Center the icon under the cursor
                            DraggedSlot->SetPosition(LocalPosition);
                        }
                    }
                }
            }

            UE_LOG(LogTemp, Log, TEXT("Started moving item %d"), HoveredIndex);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Cannot start drag at index %d"), HoveredIndex);
        }
    }
    else if (!bStartMove && !bEndMove && MovingIndex != INDEX_NONE)
    {
        // Update the overlay position to follow the mouse
        if (ForegroundBorders.IsValidIndex(MovingIndex))
        {
            // The overlay is currently parented to the Canvas
            UOverlay* OverlayToDrag = nullptr;
            for (int32 i = 0; i < Canvas->GetChildrenCount(); i++)
            {
                UOverlay* ChildOverlay = Cast<UOverlay>(Canvas->GetChildAt(i));
                if (ChildOverlay)
                {
                    // Check if this overlay belongs to the dragged item by matching its content
                    if (UImage* IconImage = Cast<UImage>(ChildOverlay->GetChildAt(0)))
                    {
                        UTexture2D* Texture = Cast<UTexture2D>(IconImage->GetBrush().GetResourceObject());
                        if (Texture == Items[MovingIndex].IconTexture.Get())
                        {
                            OverlayToDrag = ChildOverlay;
                            break;
                        }
                    }
                }
            }

            if (OverlayToDrag)
            {
                FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
                FVector2D LocalPosition = GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
                if (UCanvasPanelSlot* DraggedSlot = Cast<UCanvasPanelSlot>(OverlayToDrag->Slot))
                {
                    LocalPosition -= FVector2D(25.0f, 25.0f); // Center the icon under the cursor
                    DraggedSlot->SetPosition(LocalPosition);
                    UE_LOG(LogTemp, Log, TEXT("Dragging item %d to X=%f, Y=%f"), MovingIndex, LocalPosition.X, LocalPosition.Y);
                }
            }
        }
    }
    else if (bEndMove && MovingIndex != INDEX_NONE)
    {
        int32 TargetIndex = FindHoveredItemIndex(InMouseEvent);
        if (TargetIndex != INDEX_NONE && Items.IsValidIndex(TargetIndex) && TargetIndex != MovingIndex)
        {
            // Swap items
            FItem TempItem = Items[MovingIndex];
            Items[MovingIndex] = Items[TargetIndex];
            Items[TargetIndex] = TempItem;

            // Swap the content of the borders
            if (ForegroundBorders.IsValidIndex(MovingIndex) && ForegroundBorders.IsValidIndex(TargetIndex))
            {
                UOverlay* DraggedOverlay = nullptr;
                for (int32 i = 0; i < Canvas->GetChildrenCount(); i++)
                {
                    UOverlay* ChildOverlay = Cast<UOverlay>(Canvas->GetChildAt(i));
                    if (ChildOverlay)
                    {
                        if (UImage* IconImage = Cast<UImage>(ChildOverlay->GetChildAt(0)))
                        {
                            UTexture2D* Texture = Cast<UTexture2D>(IconImage->GetBrush().GetResourceObject());
                            if (Texture == TempItem.IconTexture.Get())
                            {
                                DraggedOverlay = ChildOverlay;
                                break;
                            }
                        }
                    }
                }

                UOverlay* TargetOverlay = Cast<UOverlay>(ForegroundBorders[TargetIndex]->GetContent());

                // Reparent the dragged overlay to the target slot
                if (DraggedOverlay)
                {
                    Canvas->RemoveChild(DraggedOverlay);
                    ForegroundBorders[TargetIndex]->SetContent(DraggedOverlay);
                    ForegroundBorders[TargetIndex]->SetVisibility(ESlateVisibility::Visible);
                }

                // Reparent the target overlay (if any) to the original slot
                if (TargetOverlay)
                {
                    ForegroundBorders[TargetIndex]->SetContent(nullptr);
                    ForegroundBorders[MovingIndex]->SetContent(TargetOverlay);
                    ForegroundBorders[MovingIndex]->SetVisibility(ESlateVisibility::Visible);
                }
            }

            UE_LOG(LogTemp, Log, TEXT("Moved item from %d to %d"), MovingIndex, TargetIndex);
        }
        else
        {
            // Restore the overlay to its original slot
            if (ForegroundBorders.IsValidIndex(MovingIndex))
            {
                UOverlay* OverlayToRestore = nullptr;
                for (int32 i = 0; i < Canvas->GetChildrenCount(); i++)
                {
                    UOverlay* ChildOverlay = Cast<UOverlay>(Canvas->GetChildAt(i));
                    if (ChildOverlay)
                    {
                        if (UImage* IconImage = Cast<UImage>(ChildOverlay->GetChildAt(0)))
                        {
                            UTexture2D* Texture = Cast<UTexture2D>(IconImage->GetBrush().GetResourceObject());
                            if (Texture == Items[MovingIndex].IconTexture.Get())
                            {
                                OverlayToRestore = ChildOverlay;
                                break;
                            }
                        }
                    }
                }

                if (OverlayToRestore)
                {
                    Canvas->RemoveChild(OverlayToRestore);
                    ForegroundBorders[MovingIndex]->SetContent(OverlayToRestore);
                    ForegroundBorders[MovingIndex]->SetVisibility(ESlateVisibility::Visible);
                }
            }
            UE_LOG(LogTemp, Log, TEXT("Drag canceled, no target slot found"));
        }

        // Reset dragging state
        Items[MovingIndex].bIsSelected = false;
        DraggedItemIndex = INDEX_NONE;
        bIsDragging = false;
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

