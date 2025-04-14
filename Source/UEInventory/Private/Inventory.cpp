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
#include "Components/SizeBox.h"
#include "Blueprint/UserWidget.h"

#endif // !INVENTORY_HEADERS_H

uint64 UInventory::ItemCounter = 0;

void UInventory::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Initialize inventory state
    bIsInventoryFull = false;
    DraggedItemIndex = INDEX_NONE;
    DraggedOverlay = nullptr;

    // Validate WidgetTree
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetTree is invalid"));
        return;
    }

    // Create and set up the Canvas as the root widget
    Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    if (!Canvas)
    {
        UE_LOG(LogTemp, Warning, TEXT("Canvas is invalid"));
        return;
    }
    WidgetTree->RootWidget = Canvas;

    // Create and set up the BackgroundBorder
    BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (!BackgroundBorder)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundBorder is invalid"));
        return;
    }
    BackgroundBorder->SetBrushColor(FLinearColor::Gray);

    // Create the Title
    Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (!Title)
    {
        UE_LOG(LogTemp, Warning, TEXT("Title is invalid"));
        return;
    }
    Title->SetText(FText::FromString(TEXT("Inventory")));

    // Create a vertical box to hold the Title and Grid inside the BackgroundBorder
    UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (!ContentBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("ContentBox is invalid"));
        return;
    }

    // Add Title to the ContentBox
    UVerticalBoxSlot* TitleBoxSlot = ContentBox->AddChildToVerticalBox(Title);
    if (TitleBoxSlot)
    {
        TitleBoxSlot->SetHorizontalAlignment(HAlign_Center);
        TitleBoxSlot->SetVerticalAlignment(VAlign_Top);
        TitleBoxSlot->SetPadding(FMargin(0, 10, 0, 10)); // Add some padding to position the title nicely
    }

    // Create the Grid
    Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    if (!Grid)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid is invalid"));
        return;
    }
    Grid->SetSlotPadding(FMargin(15, 15, 15, 15));

    // Add Grid to the ContentBox
    UVerticalBoxSlot* GridBoxSlot = ContentBox->AddChildToVerticalBox(Grid);
    if (GridBoxSlot)
    {
        GridBoxSlot->SetHorizontalAlignment(HAlign_Fill);
        GridBoxSlot->SetVerticalAlignment(VAlign_Fill);
    }

    // Add ContentBox to BackgroundBorder as its content
    BackgroundBorder->SetContent(ContentBox);

    // Add BackgroundBorder to Canvas and configure its slot
    BackgroundBorderSlot = Canvas->AddChildToCanvas(BackgroundBorder);
    if (BackgroundBorderSlot)
    {
        BackgroundBorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        BackgroundBorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        BackgroundBorderSlot->SetOffsets(FMargin(0, -100, 510.0f, 500.0f)); // Move the grid up by 100 pixels
    }

    // Set render scale and initialize the inventory grid
    SetRenderScale(FVector2D(1.0f, 1.0f));
    Create();
}

void UInventory::NativeConstruct()
{
    Super::NativeConstruct();

    // Force layout updates to ensure geometry is calculated
    BackgroundBorder->ForceLayoutPrepass();
    BackgroundBorder->InvalidateLayoutAndVolatility();
    Grid->ForceLayoutPrepass();
    Grid->InvalidateLayoutAndVolatility();
    Canvas->ForceLayoutPrepass();
    Canvas->InvalidateLayoutAndVolatility();

    // Force layout for each foreground border
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        if (Border)
        {
            Border->ForceLayoutPrepass();
            Border->InvalidateLayoutAndVolatility();
        }
    }

    // Log the geometry of Title, BackgroundBorder, and Grid for debugging
    if (Title)
    {
        FGeometry TitleGeometry = Title->GetCachedGeometry();
        FVector2D TitleAbsTopLeft = TitleGeometry.LocalToAbsolute(FVector2D::ZeroVector);
        FVector2D TitleAbsSize = TitleGeometry.GetLocalSize();
        UE_LOG(LogTemp, Log, TEXT("Title Position: TopLeft: X=%f, Y=%f, Size: X=%f, Y=%f"),
            TitleAbsTopLeft.X, TitleAbsTopLeft.Y, TitleAbsSize.X, TitleAbsSize.Y);
    }
    if (BackgroundBorder)
    {
        FGeometry BorderGeometry = BackgroundBorder->GetCachedGeometry();
        FVector2D BorderAbsTopLeft = BorderGeometry.LocalToAbsolute(FVector2D::ZeroVector);
        FVector2D BorderAbsSize = BorderGeometry.GetLocalSize();
        FVector2D BorderAbsBottomRight = BorderAbsTopLeft + BorderAbsSize;
        UE_LOG(LogTemp, Log, TEXT("BackgroundBorder TopLeft: X=%f, Y=%f, BottomRight: X=%f, Y=%f"),
            BorderAbsTopLeft.X, BorderAbsTopLeft.Y, BorderAbsBottomRight.X, BorderAbsBottomRight.Y);
    }
    if (Grid)
    {
        FGeometry GridGeometry = Grid->GetCachedGeometry();
        FVector2D GridAbsTopLeft = GridGeometry.LocalToAbsolute(FVector2D::ZeroVector);
        FVector2D GridAbsSize = GridGeometry.GetLocalSize();
        FVector2D GridAbsBottomRight = GridAbsTopLeft + GridAbsSize;
        UE_LOG(LogTemp, Log, TEXT("Grid TopLeft: X=%f, Y=%f, BottomRight: X=%f, Y=%f"),
            GridAbsTopLeft.X, GridAbsTopLeft.Y, GridAbsBottomRight.X, GridAbsBottomRight.Y);
    }

    // Log geometry for each slot
    for (uint64 i = 0; i < ForegroundBorders.Num(); i++)
    {
        if (ForegroundBorders[i])
        {
            FGeometry BorderGeometry = ForegroundBorders[i]->GetCachedGeometry();
            FVector2D SlotAbsTopLeft = BorderGeometry.LocalToAbsolute(FVector2D::ZeroVector);
            FVector2D SlotAbsSize = BorderGeometry.GetLocalSize();
            FVector2D SlotAbsBottomRight = SlotAbsTopLeft + SlotAbsSize;
            UE_LOG(LogTemp, Log, TEXT("Slot[%d] - TopLeft: X=%f, Y=%f, BottomRight: X=%f, Y=%f, Size: X=%f, Y=%f"),
                i, SlotAbsTopLeft.X, SlotAbsTopLeft.Y, SlotAbsBottomRight.X, SlotAbsBottomRight.Y,
                SlotAbsSize.X, SlotAbsSize.Y);
        }
    }
}

void UInventory::Create()
{
    if (!Grid || !WidgetTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid or WidgetTree is invalid in Create."));
        return;
    }

    // Clear existing slots
    Grid->ClearChildren();
    ForegroundBorders.Empty();
    IconSlots.Empty();
    Items.Empty();

    // Resize arrays
    ForegroundBorders.SetNum(MaxRows * MaxColumns);
    Items.SetNum(MaxRows * MaxColumns);
    bCounterTextUpdated.Init(false, MaxRows * MaxColumns);

    // Create slots
    for (uint64 Rows = 0; Rows < MaxRows; Rows++)
    {
        for (uint64 Columns = 0; Columns < MaxColumns; Columns++)
        {
            uint64 Index = Rows * MaxColumns + Columns;

            TObjectPtr<UBorder> SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
            if (!SlotBorder)
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to create SlotBorder at Index=%d"), Index);
                continue;
            }
            SlotBorder->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));

            TObjectPtr<USizeBox> SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            if (!SizeBox)
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to create SizeBox at Index=%d"), Index);
                continue;
            }
            SizeBox->SetWidthOverride(100.0f);
            SizeBox->SetHeightOverride(100.0f);

            SlotBorder->SetContent(SizeBox);

            GridSlot = Grid->AddChildToUniformGrid(SlotBorder, Rows, Columns);
            if (GridSlot)
            {
                GridSlot->SetHorizontalAlignment(HAlign_Center);
                GridSlot->SetVerticalAlignment(VAlign_Center);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to create GridSlot at Index=%d"), Index);
                continue;
            }

            ForegroundBorders[Index] = SlotBorder;

            SlotBorder->ForceLayoutPrepass();
            SlotBorder->InvalidateLayoutAndVolatility();
            SizeBox->ForceLayoutPrepass();
            SizeBox->InvalidateLayoutAndVolatility();
        }
    }

    Grid->ForceLayoutPrepass();
    Grid->InvalidateLayoutAndVolatility();
}

FReply UInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        MoveItem(InMouseEvent, true, false);
        if (DraggedItemIndex != INDEX_NONE) // Use DraggedItemIndex to indicate drag state
        {
            UE_LOG(LogTemp, Log, TEXT("Started dragging item."));
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (DraggedItemIndex != INDEX_NONE) // Use DraggedItemIndex instead of bIsDragging
    {
        MoveItem(InMouseEvent, false, false);
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (DraggedItemIndex != INDEX_NONE && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
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
    Items[EmptySlotIndex].Index = ItemCounter; // Use a persistent counter
    // Increment the counter for the next item
    ItemCounter++;

    // Create the UI icon and counter for this slot.
    CreateItemIcon(EmptySlotIndex);
    CreateIconCounterText(EmptySlotIndex);

    ItemActor->Destroy();

    // If the last slot was just filled, mark inventory as full.
    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);

    
    // Update the item counter for display (if used).
    ItemCounter = (ItemCounter % 12) + 1;
}

uint64 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent)
{
    if (!Grid || ForegroundBorders.Num() == 0) return INDEX_NONE;

    // Force layout updates
    Canvas->ForceLayoutPrepass();
    Canvas->InvalidateLayoutAndVolatility();
    Grid->ForceLayoutPrepass();
    Grid->InvalidateLayoutAndVolatility();

    FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
    FGeometry GridGeometry = Grid->GetCachedGeometry();
    FVector2D GridAbsTopLeft = GridGeometry.LocalToAbsolute(FVector2D::ZeroVector);
    FVector2D GridAbsBottomRight = GridAbsTopLeft + GridGeometry.GetLocalSize();

    const float Tolerance = 20.0f;
    if (MousePos.X < GridAbsTopLeft.X - Tolerance || MousePos.X > GridAbsBottomRight.X + Tolerance ||
        MousePos.Y < GridAbsTopLeft.Y - Tolerance || MousePos.Y > GridAbsBottomRight.Y + Tolerance)
    {
        return INDEX_NONE;
    }

    FVector2D LocalMousePos = GridGeometry.AbsoluteToLocal(MousePos);
    const float SlotWidth = 100.0f, SlotHeight = 100.0f, Padding1 = 15.0f;
    const float CellWidth = SlotWidth + Padding1, CellHeight = SlotHeight + Padding1;
    const float SlotTolerance = 5.0f;

    for (uint64 Row = 0; Row < MaxRows; Row++)
    {
        for (uint64 Col = 0; Col < MaxColumns; Col++)
        {
            uint64 Index = Row * MaxColumns + Col;
            FVector2D SlotLocalTopLeft = FVector2D(Col * CellWidth + Padding1 / 2, Row * CellHeight + Padding1 / 2);
            FVector2D SlotLocalBottomRight = SlotLocalTopLeft + FVector2D(SlotWidth, SlotHeight);
            UE_LOG(LogTemp, Log, TEXT("Checking Slot[%d]: X=[%f, %f], Y=[%f, %f] vs Mouse: X=%f, Y=%f"),
              Index, SlotLocalTopLeft.X, SlotLocalBottomRight.X, SlotLocalTopLeft.Y, SlotLocalBottomRight.Y,
                 LocalMousePos.X, LocalMousePos.Y);
            if (LocalMousePos.X >= SlotLocalTopLeft.X - SlotTolerance && LocalMousePos.X <= SlotLocalBottomRight.X + SlotTolerance &&
                LocalMousePos.Y >= SlotLocalTopLeft.Y - SlotTolerance && LocalMousePos.Y <= SlotLocalBottomRight.Y + SlotTolerance)
            {
                UE_LOG(LogTemp, Log, TEXT("Picked Slot[%d] at LocalMousePos: X=%f, Y=%f"), Index, LocalMousePos.X, LocalMousePos.Y);
                return Index;
            }
        }
    }
    return INDEX_NONE;
}

void UInventory::CreateItemIcon(uint64 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !ForegroundBorders.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid item or border index %d"), SlotIndex);
        return;
    }

    // Get the USizeBox from the border
    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("USizeBox not found for slot %d in CreateItemIcon"), SlotIndex);
        return;
    }

    // Check if the USizeBox already has an overlay; if not, create one
    TObjectPtr<UOverlay> IconOverlay = Cast<UOverlay>(SizeBox->GetContent());
    if (!IconOverlay)
    {
        IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
        if (!IconOverlay)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create UOverlay for slot %d"), SlotIndex);
            return;
        }
        SizeBox->SetContent(IconOverlay); // Set the overlay as the content of the USizeBox
    }

    // Look for an existing UImage in the overlay
    UImage* ItemIcon = nullptr;
    for (int32 i = 0; i < IconOverlay->GetChildrenCount(); ++i)
    {
        if (UImage* Image = Cast<UImage>(IconOverlay->GetChildAt(i)))
        {
            ItemIcon = Image;
            break;
        }
    }

    // If no image exists, create one
    if (!ItemIcon)
    {
        ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        if (!ItemIcon)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create UImage for slot %d"), SlotIndex);
            return;
        }
        TObjectPtr<UOverlaySlot> ImageSlot = IconOverlay->AddChildToOverlay(ItemIcon);
        if (ImageSlot)
        {
            ImageSlot->SetHorizontalAlignment(HAlign_Fill);
            ImageSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }

    // Update the image
    if (Items[SlotIndex].IconTexture.IsValid())
    {
        ItemIcon->SetBrushFromTexture(Items[SlotIndex].IconTexture.Get());
    }
    else
    {
        ItemIcon->SetColorAndOpacity(FLinearColor::Blue); // Placeholder
    }

    // Only set visibility if the slot was not already visible
    if (ForegroundBorders[SlotIndex]->GetVisibility() != ESlateVisibility::Visible)
    {
        ForegroundBorders[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
    }
}

void UInventory::CreateIconCounterText(uint64 SlotIndex)
{
    if (!ForegroundBorders.IsValidIndex(SlotIndex) || !Items.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid border or item index %d in CreateIconCounterText"), SlotIndex);
        return;
    }

    // Get the USizeBox from the border
    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("USizeBox not found for slot %d in CreateIconCounterText"), SlotIndex);
        return;
    }

    // Get or create the overlay inside the USizeBox
    TObjectPtr<UOverlay> IconOverlay = Cast<UOverlay>(SizeBox->GetContent());
    if (!IconOverlay)
    {
        IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
        if (!IconOverlay)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create UOverlay for slot %d in CreateIconCounterText"), SlotIndex);
            return;
        }
        SizeBox->SetContent(IconOverlay);
    }

    // Look for an existing UTextBlock in the overlay
    UTextBlock* ItemCounterText = nullptr;
    for (int32 i = 0; i < IconOverlay->GetChildrenCount(); ++i)
    {
        if (UTextBlock* Text = Cast<UTextBlock>(IconOverlay->GetChildAt(i)))
        {
            ItemCounterText = Text;
            break;
        }
    }

    // If no text block exists, create one
    if (!ItemCounterText)
    {
        ItemCounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!ItemCounterText)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create UTextBlock for slot %d"), SlotIndex);
            return;
        }

        TObjectPtr<UOverlaySlot> TextOverlaySlot = IconOverlay->AddChildToOverlay(ItemCounterText);
        if (TextOverlaySlot)
        {
            TextOverlaySlot->SetHorizontalAlignment(HAlign_Right);
            TextOverlaySlot->SetVerticalAlignment(VAlign_Bottom);
            TextOverlaySlot->SetPadding(FMargin(0, 0, 5, 5)); // 5-pixel padding from bottom-right corner
        }
    }

    // Log the index being set
    UE_LOG(LogTemp, Log, TEXT("Setting counter text for Slot %d to Index %d"), SlotIndex, Items[SlotIndex].Index);

    // Set the text to the item's index
    ItemCounterText->SetText(FText::AsNumber(Items[SlotIndex].Index));
    ItemCounterText->SetColorAndOpacity(FLinearColor::Red);
    ItemCounterText->SetJustification(ETextJustify::Center);
    ItemCounterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));

    // Only set visibility if the slot was not already visible
    if (ForegroundBorders[SlotIndex]->GetVisibility() != ESlateVisibility::Visible)
    {
        ForegroundBorders[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
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

    // Ensure the ItemIndex matches the currently dragged item
    if (ItemIndex != DraggedItemIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemIndex %d does not match DraggedItemIndex %d in FindDraggedOverlay"), ItemIndex, DraggedItemIndex);
        return nullptr;
    }

    if (DraggedOverlay)
    {
        UE_LOG(LogTemp, Log, TEXT("Found dragged overlay for ItemIndex %d"), ItemIndex);
        return DraggedOverlay;
    }

    UE_LOG(LogTemp, Warning, TEXT("No dragged overlay found for ItemIndex %d"), ItemIndex);
    return nullptr;
}

void UInventory::MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished)
{
    uint64 MovingIndex = DraggedItemIndex;
    FGeometry CanvasGeometry = Canvas->GetCachedGeometry();

    if (bItemMovementStarted && MovingIndex == INDEX_NONE)
    {
        uint64 HoveredIndex = FindHoveredItemIndex(MouseEvent);
        if (HoveredIndex == INDEX_NONE)
        {
            // Retry on next tick to allow layout stabilization
            GetWorld()->GetTimerManager().SetTimerForNextTick([this, MouseEvent]()
                {
                    MoveItem(MouseEvent, true, false);
                });
            UE_LOG(LogTemp, Warning, TEXT("No hovered index found; retrying on next tick."));
            return;
        }

        if (!Items.IsValidIndex(HoveredIndex) || !Items[HoveredIndex].ReferencedActorClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("No item at hovered index %d."), HoveredIndex);
            return;
        }

        if (ForegroundBorders.IsValidIndex(HoveredIndex))
        {
            TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[HoveredIndex]->GetContent());
            if (SizeBox && SizeBox->GetContent())
            {
                for (FItem& Item : Items)
                {
                    Item.bIsSelected = false;
                }
                Items[HoveredIndex].bIsSelected = true;
                DraggedItemIndex = HoveredIndex;

                TObjectPtr<UBorder> Border = ForegroundBorders[HoveredIndex];
                if (Border)
                {
                    DraggedOverlay = Cast<UOverlay>(SizeBox->GetContent());
                    if (DraggedOverlay)
                    {
                        SizeBox->SetContent(nullptr);
                        Canvas->AddChild(DraggedOverlay);

                        if (UCanvasPanelSlot* DraggedSlot = Cast<UCanvasPanelSlot>(DraggedOverlay->Slot))
                        {
                            DraggedSlot->SetSize(FVector2D(100.0f, 100.0f));
                            DraggedSlot->SetZOrder(100);

                            FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
                            FVector2D LocalPos = CanvasGeometry.AbsoluteToLocal(ScreenPos) - FVector2D(1.0f, 1.0f);
                            FVector2D WidgetSize = DraggedOverlay->GetDesiredSize();
                            FVector2D CenterOffset = FVector2D(WidgetSize.X * 0.4f, WidgetSize.Y * 0.3f);
                            FVector2D CenteredPos = LocalPos - CenterOffset;

                            DraggedSlot->SetPosition(CenteredPos);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("OverlayToDrag not found at hovered index %d"), HoveredIndex);
                    }
                }
                UE_LOG(LogTemp, Log, TEXT("Started moving item from slot %d with item index %d"), HoveredIndex, Items[HoveredIndex].Index);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Cannot start drag at index %d: invalid index or no content."), HoveredIndex);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Cannot start drag at index %d: invalid index."), HoveredIndex);
        }
    }
    else if (!bItemMovementStarted && !bItemMovementFinished && MovingIndex != INDEX_NONE)
    {
        TObjectPtr<UOverlay> OverlayToDrag = FindDraggedOverlay(MovingIndex);
        if (OverlayToDrag)
        {
            FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
            FVector2D LocalPos = CanvasGeometry.AbsoluteToLocal(ScreenPos);
            TObjectPtr<UCanvasPanelSlot> DraggedSlot = Cast<UCanvasPanelSlot>(OverlayToDrag->Slot);
            if (DraggedSlot)
            {
                FVector2D WidgetSize = OverlayToDrag->GetDesiredSize();
                FVector2D CenterOffset = FVector2D(WidgetSize.X * 0.4f, WidgetSize.Y * 0.3f);
                FVector2D CenteredPos = LocalPos - CenterOffset;
                DraggedSlot->SetPosition(CenteredPos);
                UE_LOG(LogTemp, Log, TEXT("Dragging item from slot %d to X=%f, Y=%f"), MovingIndex, LocalPos.X, LocalPos.Y);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Dragged overlay not found during drag update for slot %d"), MovingIndex);
        }
    }
    else if (bItemMovementFinished && MovingIndex != INDEX_NONE)
    {
        uint64 TargetIndex = FindHoveredItemIndex(MouseEvent);
        UE_LOG(LogTemp, Log, TEXT("Drop detected. MovingIndex: %d, TargetIndex: %d"), MovingIndex, TargetIndex);

        if (TargetIndex != INDEX_NONE && ForegroundBorders.IsValidIndex(TargetIndex) && TargetIndex != MovingIndex)
        {
            TObjectPtr<UOverlay> DraggedOverlayLocal = FindDraggedOverlay(MovingIndex);
            if (!DraggedOverlayLocal)
            {
                UE_LOG(LogTemp, Warning, TEXT("Dragged overlay not found during drop for slot %d"), MovingIndex);
                DraggedItemIndex = INDEX_NONE;
                DraggedOverlay = nullptr;
                return;
            }

            if (Canvas->HasChild(DraggedOverlayLocal))
            {
                Canvas->RemoveChild(DraggedOverlayLocal);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Canvas does not contain dragged overlay for slot %d"), MovingIndex);
            }

            if (ForegroundBorders.IsValidIndex(MovingIndex) && ForegroundBorders.IsValidIndex(TargetIndex))
            {
                TObjectPtr<USizeBox> TargetSizeBox = Cast<USizeBox>(ForegroundBorders[TargetIndex]->GetContent());
                TObjectPtr<USizeBox> MovingSizeBox = Cast<USizeBox>(ForegroundBorders[MovingIndex]->GetContent());

                if (!TargetSizeBox || !MovingSizeBox)
                {
                    UE_LOG(LogTemp, Warning, TEXT("USizeBox not found in ForegroundBorders at indices %d or %d during drop"), MovingIndex, TargetIndex);
                    DraggedItemIndex = INDEX_NONE;
                    DraggedOverlay = nullptr;
                    return;
                }

                TObjectPtr<UOverlay> TargetOverlay = Cast<UOverlay>(TargetSizeBox->GetContent());

                if (Items.IsValidIndex(MovingIndex) && Items.IsValidIndex(TargetIndex))
                {
                    SortItem(Items[MovingIndex], Items[TargetIndex]);
                    UE_LOG(LogTemp, Log, TEXT("Item sorted"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Invalid inventory indices for swap: %d, %d"), MovingIndex, TargetIndex);
                }

                TargetSizeBox->SetContent(DraggedOverlayLocal);
                if (ForegroundBorders[TargetIndex]->GetVisibility() != ESlateVisibility::Visible)
                {
                    ForegroundBorders[TargetIndex]->SetVisibility(ESlateVisibility::Visible);
                }

                if (TargetOverlay)
                {
                    MovingSizeBox->SetContent(TargetOverlay);
                }
                else
                {
                    MovingSizeBox->SetContent(nullptr);
                }

                if (ForegroundBorders[MovingIndex]->GetVisibility() != ESlateVisibility::Visible)
                {
                    ForegroundBorders[MovingIndex]->SetVisibility(ESlateVisibility::Visible);
                }
                if (ForegroundBorders[TargetIndex]->GetVisibility() != ESlateVisibility::Visible)
                {
                    ForegroundBorders[TargetIndex]->SetVisibility(ESlateVisibility::Visible);
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
            TObjectPtr<UOverlay> OverlayToRestore = FindDraggedOverlay(MovingIndex);
            if (OverlayToRestore && ForegroundBorders.IsValidIndex(MovingIndex))
            {
                if (Canvas->HasChild(OverlayToRestore))
                {
                    Canvas->RemoveChild(OverlayToRestore);
                }
                TObjectPtr<USizeBox> MovingSizeBox = Cast<USizeBox>(ForegroundBorders[MovingIndex]->GetContent());
                if (MovingSizeBox)
                {
                    // Check if dropped outside grid bounds
                    FVector2D MousePos = MouseEvent.GetScreenSpacePosition();
                    FGeometry GridGeometry = Grid->GetCachedGeometry();
                    FVector2D GridAbsTopLeft = GridGeometry.LocalToAbsolute(FVector2D::ZeroVector);
                    FVector2D GridAbsBottomRight = GridAbsTopLeft + GridGeometry.GetLocalSize();
                    if (MousePos.X < GridAbsTopLeft.X || MousePos.X > GridAbsBottomRight.X ||
                        MousePos.Y < GridAbsTopLeft.Y || MousePos.Y > GridAbsBottomRight.Y)
                    {
                        // Handle drop outside grid (e.g., remove item)
                        Items[MovingIndex].ReferencedActorClass = nullptr;
                        MovingSizeBox->SetContent(nullptr);
                        UE_LOG(LogTemp, Log, TEXT("Item dropped outside grid from slot %d; removed."), MovingIndex);
                    }
                    else
                    {
                        MovingSizeBox->SetContent(OverlayToRestore);
                        UE_LOG(LogTemp, Log, TEXT("Restored item to original slot %d"), MovingIndex);
                    }
                }
            }
        }
        DraggedItemIndex = INDEX_NONE;
        DraggedOverlay = nullptr;
    }
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

EDirection UInventory::SortItem(FItem& MovedItem, FItem& ItemToMove)
{
    // Reset counter text update flags for the affected slots only
    int32 IndexA = FindItemIndex(MovedItem);
    int32 IndexB = FindItemIndex(ItemToMove);
    if (bCounterTextUpdated.IsValidIndex(IndexA))
    {
        bCounterTextUpdated[IndexA] = false;
    }
    if (bCounterTextUpdated.IsValidIndex(IndexB))
    {
        bCounterTextUpdated[IndexB] = false;
    }

    // Validate the indices
    if (!Items.IsValidIndex(IndexA) || !Items.IsValidIndex(IndexB))
    {
        UE_LOG(LogTemp, Warning, TEXT("SortItem failed: Invalid item indices - IndexA=%d, IndexB=%d"), IndexA, IndexB);
        return static_cast<EDirection>(255);
    }

    // Calculate row and column for each item
    int32 RowA = IndexA / MaxColumns;
    int32 ColA = IndexA % MaxColumns;
    int32 RowB = IndexB / MaxColumns;
    int32 ColB = IndexB % MaxColumns;

    EDirection MoveDirection = GetMoveDirection(RowA, ColA, RowB, ColB);
    if (MoveDirection == static_cast<EDirection>(255))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid move direction from (%d,%d) to (%d,%d)"), RowA, ColA, RowB, ColB);
        return MoveDirection;
    }

    TObjectPtr<UOverlay> OverlayA = FindDraggedOverlay(IndexA);
    TObjectPtr<UOverlay> OverlayB = nullptr;
    if (ForegroundBorders.IsValidIndex(IndexB))
    {
        TObjectPtr<USizeBox> SizeBoxB = Cast<USizeBox>(ForegroundBorders[IndexB]->GetContent());
        if (SizeBoxB)
        {
            OverlayB = Cast<UOverlay>(SizeBoxB->GetContent());
        }
    }

    if (!OverlayA || !OverlayB)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid overlays in SortItem: OverlayA=%s, OverlayB=%s"),
            OverlayA ? TEXT("Valid") : TEXT("Null"),
            OverlayB ? TEXT("Valid") : TEXT("Null"));
        return MoveDirection;
    }

    if (ForegroundBorders.IsValidIndex(IndexB))
    {
        TObjectPtr<USizeBox> SizeBoxB = Cast<USizeBox>(ForegroundBorders[IndexB]->GetContent());
        if (SizeBoxB)
        {
            SizeBoxB->SetContent(nullptr);
        }
    }

    const float SlotWidth = 100.0f;
    const float SlotHeight = 100.0f;
    const float Padding2 = 15.0f;
    float CellWidth = SlotWidth + Padding2;
    float CellHeight = SlotHeight + Padding2;

    FVector2D StartPosA = FVector2D(ColA * CellWidth + Padding2, RowA * CellHeight + Padding2);
    FVector2D StartPosB = FVector2D(ColB * CellWidth + Padding2, RowB * CellHeight + Padding2);
    FVector2D EndPosA = StartPosB;
    FVector2D EndPosB = StartPosA;

    UCanvasPanelSlot* SlotA = nullptr;
    if (Canvas->HasChild(OverlayA))
    {
        SlotA = Cast<UCanvasPanelSlot>(OverlayA->Slot);
        if (!SlotA)
        {
            UE_LOG(LogTemp, Warning, TEXT("OverlayA Slot is invalid; re-adding to Canvas"));
            Canvas->RemoveChild(OverlayA);
            SlotA = Canvas->AddChildToCanvas(OverlayA);
        }
    }
    else
    {
        SlotA = Canvas->AddChildToCanvas(OverlayA);
    }

    UCanvasPanelSlot* SlotB = Canvas->AddChildToCanvas(OverlayB);

    if (!SlotA || !SlotB)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create Canvas slots: SlotA=%s, SlotB=%s"),
            SlotA ? TEXT("Valid") : TEXT("Null"),
            SlotB ? TEXT("Valid") : TEXT("Null"));
        if (OverlayA && !Canvas->HasChild(OverlayA)) Canvas->AddChild(OverlayA);
        if (OverlayB) Canvas->RemoveChild(OverlayB);
        if (ForegroundBorders.IsValidIndex(IndexA))
        {
            TObjectPtr<USizeBox> SizeBoxA = Cast<USizeBox>(ForegroundBorders[IndexA]->GetContent());
            if (SizeBoxA) SizeBoxA->SetContent(OverlayA);
        }
        if (ForegroundBorders.IsValidIndex(IndexB))
        {
            TObjectPtr<USizeBox> SizeBoxB = Cast<USizeBox>(ForegroundBorders[IndexB]->GetContent());
            if (SizeBoxB) SizeBoxB->SetContent(OverlayB);
        }
        return MoveDirection;
    }

    SlotA->SetPosition(StartPosA);
    SlotA->SetZOrder(10);
    SlotB->SetPosition(StartPosB);
    SlotB->SetZOrder(10);

    CurrentSlideData.WidgetA = OverlayA;
    CurrentSlideData.WidgetB = OverlayB;
    CurrentSlideData.StartPosA = StartPosA;
    CurrentSlideData.StartPosB = StartPosB;
    CurrentSlideData.EndPosA = EndPosA;
    CurrentSlideData.EndPosB = EndPosB;
    CurrentSlideData.IndexA = IndexA;
    CurrentSlideData.IndexB = IndexB;
    CurrentSlideData.ElapsedTime = 0.0f;

    Items.Swap(IndexA, IndexB);

    if (SlideSwap)
    {
        PlayAnimation(SlideSwap);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SlideSwap animation is null; falling back to manual animation"));
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UInventory::UpdateSlideTick);
    }

    return MoveDirection;
}

int32 UInventory::FindItemIndex(const FItem& TargetItem) const
{
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i].ReferencedActorClass == TargetItem.ReferencedActorClass &&
            Items[i].WorldLocation == TargetItem.WorldLocation)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

void UInventory::RemoveItem()
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

void UInventory::UpdateSlideTick()
{
    if (!CurrentSlideData.WidgetA || !CurrentSlideData.WidgetB)
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateSlideTick: Invalid widgets."));
        CurrentSlideData.WidgetA = nullptr;
        CurrentSlideData.WidgetB = nullptr;
        CurrentSlideData.StartPosA = FVector2D::ZeroVector;
        CurrentSlideData.StartPosB = FVector2D::ZeroVector;
        CurrentSlideData.EndPosA = FVector2D::ZeroVector;
        CurrentSlideData.EndPosB = FVector2D::ZeroVector;
        CurrentSlideData.IndexA = 0;
        CurrentSlideData.IndexB = 0;
        CurrentSlideData.ElapsedTime = 0.0f;
        return;
    }

    const float AnimationDuration = 0.3f;
    CurrentSlideData.ElapsedTime += GetWorld()->GetDeltaSeconds();
    float Alpha = FMath::Clamp(CurrentSlideData.ElapsedTime / AnimationDuration, 0.0f, 1.0f);

    FVector2D CurrentPosA = FMath::Lerp(CurrentSlideData.StartPosA, CurrentSlideData.EndPosA, Alpha);
    FVector2D CurrentPosB = FMath::Lerp(CurrentSlideData.StartPosB, CurrentSlideData.EndPosB, Alpha);

    if (UCanvasPanelSlot* SlotA = Cast<UCanvasPanelSlot>(CurrentSlideData.WidgetA->Slot))
    {
        SlotA->SetPosition(CurrentPosA);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SlotA is null in UpdateSlideTick."));
    }
    if (UCanvasPanelSlot* SlotB = Cast<UCanvasPanelSlot>(CurrentSlideData.WidgetB->Slot))
    {
        SlotB->SetPosition(CurrentPosB);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SlotB is null in UpdateSlideTick."));
    }

    UE_LOG(LogTemp, Log, TEXT("Sliding: WidgetA Pos=(%f, %f), WidgetB Pos=(%f, %f), Alpha=%f"),
        CurrentPosA.X, CurrentPosA.Y, CurrentPosB.X, CurrentPosB.Y, Alpha);

    if (Alpha >= 1.0f)
    {
        if (ForegroundBorders.IsValidIndex(CurrentSlideData.IndexA) && ForegroundBorders.IsValidIndex(CurrentSlideData.IndexB))
        {
            Canvas->RemoveChild(CurrentSlideData.WidgetA);
            Canvas->RemoveChild(CurrentSlideData.WidgetB);

            TObjectPtr<USizeBox> SizeBoxA = Cast<USizeBox>(ForegroundBorders[CurrentSlideData.IndexA]->GetContent());
            TObjectPtr<USizeBox> SizeBoxB = Cast<USizeBox>(ForegroundBorders[CurrentSlideData.IndexB]->GetContent());

            if (SizeBoxA && SizeBoxB)
            {
                SizeBoxA->SetContent(CurrentSlideData.WidgetB);
                SizeBoxB->SetContent(CurrentSlideData.WidgetA);
                if (ForegroundBorders[CurrentSlideData.IndexA]->GetVisibility() != ESlateVisibility::Visible)
                {
                    ForegroundBorders[CurrentSlideData.IndexA]->SetVisibility(ESlateVisibility::Visible);
                }
                if (ForegroundBorders[CurrentSlideData.IndexB]->GetVisibility() != ESlateVisibility::Visible)
                {
                    ForegroundBorders[CurrentSlideData.IndexB]->SetVisibility(ESlateVisibility::Visible);
                }

                // Refresh the counter text without changing the Index
                if (!bCounterTextUpdated[CurrentSlideData.IndexA])
                {
                    CreateIconCounterText(CurrentSlideData.IndexA);
                    bCounterTextUpdated[CurrentSlideData.IndexA] = true;
                }
                if (!bCounterTextUpdated[CurrentSlideData.IndexB])
                {
                    CreateIconCounterText(CurrentSlideData.IndexB);
                    bCounterTextUpdated[CurrentSlideData.IndexB] = true;
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("USizeBox not found in ForegroundBorders at indices %d or %d"),
                    CurrentSlideData.IndexA, CurrentSlideData.IndexB);
            }

            Grid->ForceLayoutPrepass();
            Grid->InvalidateLayoutAndVolatility();
            UE_LOG(LogTemp, Log, TEXT("Reattached widgets to slots %d and %d"), CurrentSlideData.IndexA, CurrentSlideData.IndexB);

            for (uint64 i = 0; i < ForegroundBorders.Num(); i++)
            {
                if (ForegroundBorders[i])
                {
                    FGeometry BorderGeometry = ForegroundBorders[i]->GetCachedGeometry();
                    FVector2D BorderAbsTopLeft = BorderGeometry.LocalToAbsolute(FVector2D::ZeroVector);
                    FVector2D BorderAbsSize = BorderGeometry.GetLocalSize();
                    FVector2D BorderAbsBottomRight = BorderAbsTopLeft + BorderAbsSize;
                    UE_LOG(LogTemp, Log, TEXT("Post-Slide Slot[%d] - TopLeft: X=%f, Y=%f; BottomRight: X=%f, Y=%f"),
                        i, BorderAbsTopLeft.X, BorderAbsTopLeft.Y, BorderAbsBottomRight.X, BorderAbsBottomRight.Y);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid ForegroundBorders indices in UpdateSlideTick."));
        }

        CurrentSlideData.WidgetA = nullptr;
        CurrentSlideData.WidgetB = nullptr;
        CurrentSlideData.StartPosA = FVector2D::ZeroVector;
        CurrentSlideData.StartPosB = FVector2D::ZeroVector;
        CurrentSlideData.EndPosA = FVector2D::ZeroVector;
        CurrentSlideData.EndPosB = FVector2D::ZeroVector;
        CurrentSlideData.IndexA = 0;
        CurrentSlideData.IndexB = 0;
        CurrentSlideData.ElapsedTime = 0.0f;
    }
    else
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UInventory::UpdateSlideTick);
    }
}

