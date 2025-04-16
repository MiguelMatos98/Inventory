#include "Inventory.h"

uint64 UInventory::ItemCounter = 0;

UInventory::UInventory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer),
    DraggedItemIndex(INDEX_NONE),
    DraggedOverlay(nullptr),
    bIsDragging(false),
    bIsSliding(false),
    SlideFromIndex(INDEX_NONE),
    SlideToIndex(INDEX_NONE),
    SlideProgress(0.0f),
    SlideDuration(0.0f)
{
    // Initialize arrays
    SlidingOverlays.Empty();
    SlideFromIndices.Empty();
    SlideToIndices.Empty();
    SlidingItems.Empty();
}

void UInventory::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    bIsInventoryFull = false;
    DraggedItemIndex = INDEX_NONE;
    DraggedOverlay = nullptr;
    ItemCounter = 0;

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

    Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (!Title)
    {
        UE_LOG(LogTemp, Warning, TEXT("Title is invalid"));
        return;
    }
    Title->SetText(FText::FromString(TEXT("Inventory")));

    UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (!ContentBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("ContentBox is invalid"));
        return;
    }

    UVerticalBoxSlot* TitleBoxSlot = ContentBox->AddChildToVerticalBox(Title);
    if (TitleBoxSlot)
    {
        TitleBoxSlot->SetHorizontalAlignment(HAlign_Center);
        TitleBoxSlot->SetVerticalAlignment(VAlign_Top);
        TitleBoxSlot->SetPadding(FMargin(0, 10, 0, 10));
    }

    Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    if (!Grid)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid is invalid"));
        return;
    }
    Grid->SetSlotPadding(FMargin(15, 15, 15, 15));

    UVerticalBoxSlot* GridBoxSlot = ContentBox->AddChildToVerticalBox(Grid);
    if (GridBoxSlot)
    {
        GridBoxSlot->SetHorizontalAlignment(HAlign_Fill);
        GridBoxSlot->SetVerticalAlignment(VAlign_Fill);
    }

    BackgroundBorder->SetContent(ContentBox);

    BackgroundBorderSlot = Canvas->AddChildToCanvas(BackgroundBorder);
    if (BackgroundBorderSlot)
    {
        BackgroundBorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        BackgroundBorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        BackgroundBorderSlot->SetOffsets(FMargin(0, -100, 510.0f, 500.0f));
    }

    SetRenderScale(FVector2D(1.0f, 1.0f));
    Create();
}

void UInventory::NativeConstruct()
{
    Super::NativeConstruct();

    BackgroundBorder->ForceLayoutPrepass();
    BackgroundBorder->InvalidateLayoutAndVolatility();
    Grid->ForceLayoutPrepass();
    Grid->InvalidateLayoutAndVolatility();
    Canvas->ForceLayoutPrepass();
    Canvas->InvalidateLayoutAndVolatility();

    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        if (Border)
        {
            Border->ForceLayoutPrepass();
            Border->InvalidateLayoutAndVolatility();
        }
    }
}

void UInventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsSliding && SlidingOverlays.Num() > 0)
    {
        // Force layout update on the first tick to ensure geometry is ready
        if (SlideProgress == 0.0f)
        {
            Canvas->ForceLayoutPrepass();
            Canvas->InvalidateLayoutAndVolatility();
            Grid->ForceLayoutPrepass();
            Grid->InvalidateLayoutAndVolatility();
            for (TObjectPtr<UBorder> Border : ForegroundBorders)
            {
                if (Border)
                {
                    Border->ForceLayoutPrepass();
                    Border->InvalidateLayoutAndVolatility();
                }
            }
        }

        SlideProgress += InDeltaTime / SlideDuration;
        if (SlideProgress >= 1.0f)
        {
            SlideProgress = 1.0f;
            bIsSliding = false;

            uint64 MinIndex = FMath::Min(SlideFromIndex, SlideToIndex);
            uint64 MaxIndex = FMath::Max(SlideFromIndex, SlideToIndex);
            for (uint64 i = MinIndex; i <= MaxIndex; ++i)
            {
                UpdateSlotUI(i);
            }

            // Clean up all overlays
            for (TObjectPtr<UOverlay> Overlay : SlidingOverlays)
            {
                if (Overlay)
                    Canvas->RemoveChild(Overlay);
            }
            SlidingOverlays.Empty();
            SlideFromIndices.Empty();
            SlideToIndices.Empty();
            SlidingItems.Empty();

            SlideFromIndex = INDEX_NONE;
            SlideToIndex = INDEX_NONE;
            SlidingItem = FItem();

            UE_LOG(LogTemp, Log, TEXT("Sliding animation completed"));
        }
        else
        {
            for (int32 i = 0; i < SlidingOverlays.Num(); ++i)
            {
                FVector2D StartPos = GetSlotPosition(SlideFromIndices[i]);
                FVector2D EndPos = GetSlotPosition(SlideToIndices[i]);

                // If either position is invalid, cancel the move and restore all items
                if (FMath::IsNearlyZero(StartPos.X) && FMath::IsNearlyZero(StartPos.Y))
                {
                    UE_LOG(LogTemp, Warning, TEXT("NativeTick: Invalid start position for index %d, cancelling move"), SlideFromIndices[i]);

                    // Restore all items to their original slots
                    for (int32 j = 0; j < SlideFromIndices.Num(); ++j)
                    {
                        Items[SlideFromIndices[j]] = SlidingItems[j];
                        UpdateSlotUI(SlideFromIndices[j]);
                    }

                    // Clean up all overlays
                    for (TObjectPtr<UOverlay> Overlay : SlidingOverlays)
                    {
                        if (Overlay)
                            Canvas->RemoveChild(Overlay);
                    }
                    SlidingOverlays.Empty();
                    SlideFromIndices.Empty();
                    SlideToIndices.Empty();
                    SlidingItems.Empty();

                    // Reset sliding state
                    bIsSliding = false;
                    SlideFromIndex = INDEX_NONE;
                    SlideToIndex = INDEX_NONE;
                    SlideProgress = 0.0f;
                    SlidingItem = FItem();

                    return;
                }

                if (FMath::IsNearlyZero(EndPos.X) && FMath::IsNearlyZero(EndPos.Y))
                {
                    UE_LOG(LogTemp, Warning, TEXT("NativeTick: Invalid end position for index %d, cancelling move"), SlideToIndices[i]);

                    // Restore all items to their original slots
                    for (int32 j = 0; j < SlideFromIndices.Num(); ++j)
                    {
                        Items[SlideFromIndices[j]] = SlidingItems[j];
                        UpdateSlotUI(SlideFromIndices[j]);
                    }

                    // Clean up all overlays
                    for (TObjectPtr<UOverlay> Overlay : SlidingOverlays)
                    {
                        if (Overlay)
                            Canvas->RemoveChild(Overlay);
                    }
                    SlidingOverlays.Empty();
                    SlideFromIndices.Empty();
                    SlideToIndices.Empty();
                    SlidingItems.Empty();

                    // Reset sliding state
                    bIsSliding = false;
                    SlideFromIndex = INDEX_NONE;
                    SlideToIndex = INDEX_NONE;
                    SlideProgress = 0.0f;
                    SlidingItem = FItem();

                    return;
                }

                float EasedProgress = CustomEaseInOut(SlideProgress);
                FVector2D CurrentPos = FMath::Lerp(StartPos, EndPos, EasedProgress);

                UCanvasPanelSlot* OverlaySlot = Cast<UCanvasPanelSlot>(SlidingOverlays[i]->Slot);
                if (OverlaySlot)
                {
                    OverlaySlot->SetPosition(CurrentPos - FVector2D(50.0f, 50.0f));
                }
            }
        }
    }
}

FReply UInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !bIsSliding)
    {
        MoveItem(InMouseEvent, true, false);
        if (DraggedItemIndex != INDEX_NONE)
        {
            UE_LOG(LogTemp, Log, TEXT("Started dragging item."));
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging && DraggedItemIndex != INDEX_NONE)
    {
        uint64 TargetIndex = FindHoveredItemIndex(InMouseEvent);
        if (TargetIndex != INDEX_NONE && TargetIndex != DraggedItemIndex)
        {
            uint64 RowA = DraggedItemIndex / MaxColumns;
            uint64 ColA = DraggedItemIndex % MaxColumns;
            uint64 RowB = TargetIndex / MaxColumns;
            uint64 ColB = TargetIndex % MaxColumns;
            EDirection Direction = GetMoveDirection(RowA, ColA, RowB, ColB);

            if (Direction != EDirection::None)
            {
                StartSlideAnimation(DraggedItemIndex, TargetIndex, Direction);
                UE_LOG(LogTemp, Log, TEXT("Started sliding animation from slot %d to slot %d in direction %d"),
                    DraggedItemIndex, TargetIndex, static_cast<int32>(Direction));
            }
        }

        for (uint64 i = 0; i < ForegroundBorders.Num(); i++)
        {
            if (ForegroundBorders[i])
            {
                ForegroundBorders[i]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
            }
        }

        DraggedItemIndex = INDEX_NONE;
        DraggedItem = FItem();
        bIsDragging = false;

        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && DraggedItemIndex != INDEX_NONE)
    {
        MoveItem(InMouseEvent, false, false);
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape && (bIsDragging || bIsSliding))
    {
        if (bIsSliding)
        {
            Items[SlideFromIndex] = SlidingItem;
            UpdateSlotUI(SlideFromIndex);

            if (DraggedOverlay)
            {
                Canvas->RemoveChild(DraggedOverlay);
                DraggedOverlay = nullptr;
            }

            bIsSliding = false;
            SlideFromIndex = INDEX_NONE;
            SlideToIndex = INDEX_NONE;
            SlideProgress = 0.0f;
            SlidingItem = FItem();
        }

        if (bIsDragging)
        {
            DraggedItemIndex = INDEX_NONE;
            DraggedItem = FItem();
            bIsDragging = false;
        }

        for (uint64 i = 0; i < ForegroundBorders.Num(); i++)
        {
            if (ForegroundBorders[i])
            {
                ForegroundBorders[i]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
            }
        }

        UE_LOG(LogTemp, Log, TEXT("Cancelled drag/slide operation with Escape key"));
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UInventory::AddItem(AActor* ItemActor)
{
    if (!ItemActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddItem: ItemActor is null."));
        return;
    }

    uint64 EmptySlotIndex = FindFirstEmptySlot();
    if (EmptySlotIndex == INDEX_NONE)
    {
        bIsInventoryFull = true;
        UE_LOG(LogTemp, Warning, TEXT("AddItem: Inventory is full."));
        return;
    }

    Items[EmptySlotIndex] = FItem();
    Items[EmptySlotIndex].ReferencedActorClass = ItemActor->GetClass();
    Items[EmptySlotIndex].WorldLocation = ItemActor->GetActorLocation();
    Items[EmptySlotIndex].Index = ItemCounter;

    UE_LOG(LogTemp, Log, TEXT("AddItem: Added item to slot %d with Index %d, ActorClass %s"),
        EmptySlotIndex, Items[EmptySlotIndex].Index,
        *Items[EmptySlotIndex].ReferencedActorClass->GetName());

    UpdateSlotUI(EmptySlotIndex);
    ItemActor->Destroy();

    ItemCounter = (ItemCounter + 1) % 13;
    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);
}

uint64 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent)
{
    if (!Grid || ForegroundBorders.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid or ForegroundBorders are missing!"));
        return INDEX_NONE;
    }

    Canvas->ForceLayoutPrepass();
    Canvas->InvalidateLayoutAndVolatility();
    Grid->ForceLayoutPrepass();
    Grid->InvalidateLayoutAndVolatility();
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        if (Border)
        {
            Border->ForceLayoutPrepass();
            Border->InvalidateLayoutAndVolatility();
        }
    }

    FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
    uint64 ClosestIndex = INDEX_NONE;
    float MinDistance = FLT_MAX;

    for (uint64 Row = 0; Row < MaxRows; Row++)
    {
        for (uint64 Col = 0; Col < MaxColumns; Col++)
        {
            uint64 Index = Row * MaxColumns + Col;
            if (!ForegroundBorders.IsValidIndex(Index) || !ForegroundBorders[Index])
                continue;

            FGeometry SlotGeometry = ForegroundBorders[Index]->GetCachedGeometry();
            FVector2D SlotAbsTopLeft = SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
            FVector2D SlotAbsSize = SlotGeometry.GetLocalSize();
            FVector2D SlotAbsBottomRight = SlotAbsTopLeft + SlotAbsSize;

            if (MousePos.X >= SlotAbsTopLeft.X && MousePos.X <= SlotAbsBottomRight.X &&
                MousePos.Y >= SlotAbsTopLeft.Y && MousePos.Y <= SlotAbsBottomRight.Y)
            {
                FVector2D SlotCenter = SlotAbsTopLeft + (SlotAbsSize / 2.0f);
                float Distance = FVector2D::Distance(MousePos, SlotCenter);
                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    ClosestIndex = Index;
                }
            }
        }
    }

    if (ClosestIndex != INDEX_NONE)
    {
        UE_LOG(LogTemp, Log, TEXT("Picked Slot[%d] at MousePos: X=%f, Y=%f"), ClosestIndex, MousePos.X, MousePos.Y);
        return ClosestIndex;
    }

    UE_LOG(LogTemp, Log, TEXT("No slot found at MousePos: X=%f, Y=%f"), MousePos.X, MousePos.Y);
    return INDEX_NONE;
}

void UInventory::CreateItemIcon(uint64 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !ForegroundBorders.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid item or border index %d"), SlotIndex);
        return;
    }

    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("USizeBox not found for slot %d in CreateItemIcon"), SlotIndex);
        return;
    }

    TObjectPtr<UOverlay> IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    if (!IconOverlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create UOverlay for slot %d"), SlotIndex);
        return;
    }
    IconOverlay->SetVisibility(ESlateVisibility::Visible);
    SizeBox->SetContent(IconOverlay);

    UImage* ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    if (!ItemIcon)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create UImage for slot %d"), SlotIndex);
        return;
    }
    ItemIcon->SetVisibility(ESlateVisibility::Visible);
    TObjectPtr<UOverlaySlot> ImageSlot = IconOverlay->AddChildToOverlay(ItemIcon);
    if (ImageSlot)
    {
        ImageSlot->SetHorizontalAlignment(HAlign_Fill);
        ImageSlot->SetVerticalAlignment(VAlign_Fill);
    }

    if (Items[SlotIndex].IconTexture.IsValid())
    {
        ItemIcon->SetBrushFromTexture(Items[SlotIndex].IconTexture.Get());
    }
    else
    {
        ItemIcon->SetColorAndOpacity(FLinearColor::Blue);
    }
}

void UInventory::CreateIconCounterText(uint64 SlotIndex)
{
    if (!ForegroundBorders.IsValidIndex(SlotIndex) || !Items.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid border or item index %d in CreateIconCounterText"), SlotIndex);
        return;
    }

    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("USizeBox not found for slot %d in CreateIconCounterText"), SlotIndex);
        return;
    }

    TObjectPtr<UOverlay> IconOverlay = Cast<UOverlay>(SizeBox->GetContent());
    if (!IconOverlay)
    {
        IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
        if (!IconOverlay)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create UOverlay for slot %d in CreateIconCounterText"), SlotIndex);
            return;
        }
        IconOverlay->SetVisibility(ESlateVisibility::Visible);
        SizeBox->SetContent(IconOverlay);
    }

    UTextBlock* ItemCounterText = nullptr;
    for (int32 i = 0; i < IconOverlay->GetChildrenCount(); ++i)
    {
        if (UTextBlock* Text = Cast<UTextBlock>(IconOverlay->GetChildAt(i)))
        {
            ItemCounterText = Text;
            break;
        }
    }

    if (!ItemCounterText)
    {
        ItemCounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!ItemCounterText)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create UTextBlock for slot %d"), SlotIndex);
            return;
        }
        ItemCounterText->SetVisibility(ESlateVisibility::Visible);
        TObjectPtr<UOverlaySlot> TextOverlaySlot = IconOverlay->AddChildToOverlay(ItemCounterText);
        if (TextOverlaySlot)
        {
            TextOverlaySlot->SetHorizontalAlignment(HAlign_Center);
            TextOverlaySlot->SetVerticalAlignment(VAlign_Center);
            TextOverlaySlot->SetPadding(FMargin(0, 0, 0, 0));
        }
    }

    ItemCounterText->SetText(FText::AsNumber(Items[SlotIndex].Index));
    ItemCounterText->SetColorAndOpacity(FLinearColor::Red);
    ItemCounterText->SetJustification(ETextJustify::Center);
    ItemCounterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
}

uint64 UInventory::FindFirstEmptySlot() const
{
    for (uint64 i = 0; i < static_cast<uint64>(Items.Num()); i++)
    {
        if (!Items[i].ReferencedActorClass)
        {
            UE_LOG(LogTemp, Log, TEXT("FindFirstEmptySlot: Found empty slot at index %d"), i);
            return i;
        }
    }
    UE_LOG(LogTemp, Log, TEXT("FindFirstEmptySlot: No empty slot found."));
    return INDEX_NONE;
}

UOverlay* UInventory::FindDraggedOverlay(uint64 ItemIndex)
{
    return nullptr; // No longer used for dragging
}

UOverlay* UInventory::CreateTemporaryDragOverlay(const FItem& Item)
{
    UOverlay* TempOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    if (!TempOverlay)
    {
        return nullptr;
    }
    TempOverlay->SetVisibility(ESlateVisibility::Visible);

    UImage* ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    if (ItemIcon)
    {
        ItemIcon->SetVisibility(ESlateVisibility::Visible);
        if (Item.IconTexture.IsValid())
        {
            ItemIcon->SetBrushFromTexture(Item.IconTexture.Get());
        }
        else
        {
            ItemIcon->SetColorAndOpacity(FLinearColor::Blue);
        }
        UOverlaySlot* ImageSlot = TempOverlay->AddChildToOverlay(ItemIcon);
        if (ImageSlot)
        {
            ImageSlot->SetHorizontalAlignment(HAlign_Fill);
            ImageSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }

    UTextBlock* ItemCounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (ItemCounterText)
    {
        ItemCounterText->SetVisibility(ESlateVisibility::Visible);
        ItemCounterText->SetText(FText::AsNumber(Item.Index));
        ItemCounterText->SetColorAndOpacity(FLinearColor::Red);
        ItemCounterText->SetJustification(ETextJustify::Center);
        ItemCounterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
        UOverlaySlot* TextOverlaySlot = TempOverlay->AddChildToOverlay(ItemCounterText);
        if (TextOverlaySlot)
        {
            TextOverlaySlot->SetHorizontalAlignment(HAlign_Center);
            TextOverlaySlot->SetVerticalAlignment(VAlign_Center);
            TextOverlaySlot->SetPadding(FMargin(0, 0, 0, 0));
        }
    }

    return TempOverlay;
}

void UInventory::MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished)
{
    Canvas->ForceLayoutPrepass();
    Canvas->InvalidateLayoutAndVolatility();
    FGeometry CanvasGeometry = Canvas->GetCachedGeometry();

    if (bItemMovementStarted && DraggedItemIndex == INDEX_NONE && !bIsDragging && !bIsSliding)
    {
        uint64 HoveredIndex = FindHoveredItemIndex(MouseEvent);
        if (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex) && Items[HoveredIndex].ReferencedActorClass)
        {
            DraggedItemIndex = HoveredIndex;
            DraggedItem = Items[HoveredIndex];
            bIsDragging = true;

            if (ForegroundBorders.IsValidIndex(DraggedItemIndex))
            {
                ForegroundBorders[DraggedItemIndex]->SetBrushColor(FLinearColor::Yellow);
            }

            UE_LOG(LogTemp, Log, TEXT("Started dragging item %d from slot %d"), DraggedItem.Index, HoveredIndex);
        }
    }
    else if (!bItemMovementStarted && !bItemMovementFinished && bIsDragging)
    {
        uint64 HoveredIndex = FindHoveredItemIndex(MouseEvent);
        for (uint64 i = 0; i < ForegroundBorders.Num(); i++)
        {
            if (ForegroundBorders[i])
            {
                if (i == DraggedItemIndex)
                    ForegroundBorders[i]->SetBrushColor(FLinearColor::Yellow);
                else if (i == HoveredIndex && i != DraggedItemIndex)
                    ForegroundBorders[i]->SetBrushColor(FLinearColor::Green);
                else
                    ForegroundBorders[i]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
            }
        }
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
    return EDirection::None;
}

EDirection UInventory::SortItem(FItem& MovedItem, FItem& ItemToMove)
{
    int32 IndexA = FindItemIndex(MovedItem);
    int32 IndexB = FindItemIndex(ItemToMove);

    if (IndexA == INDEX_NONE || IndexB == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("SortItem: Invalid indices - IndexA=%d, IndexB=%d"), IndexA, IndexB);
        return EDirection::None;
    }

    EDirection Direction = GetMoveDirection(IndexA / MaxColumns, IndexA % MaxColumns, IndexB / MaxColumns, IndexB % MaxColumns);
    if (Direction != EDirection::None)
    {
        Items.Swap(IndexA, IndexB);
        UpdateSlotUI(IndexA);
        UpdateSlotUI(IndexB);

        UE_LOG(LogTemp, Log, TEXT("SortItem: Swapped item %d at Index %d with item %d at Index %d"),
            Items[IndexB].Index, IndexB, Items[IndexA].Index, IndexA);
    }

    return Direction;
}

int32 UInventory::FindItemIndex(const FItem& TargetItem) const
{
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i].Index == TargetItem.Index)
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

TArray<UBorder*> UInventory::GetForegroundBorders()
{
    TArray<UBorder*> RawBorders;
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        RawBorders.Add(Border.Get());
    }
    return RawBorders;
}

UUniformGridPanel* UInventory::GetGrid()
{
    return Grid.Get();
}

void UInventory::UpdateSlotUI(uint64 SlotIndex)
{
    if (Items.IsValidIndex(SlotIndex) && Items[SlotIndex].ReferencedActorClass)
    {
        CreateItemIcon(SlotIndex);
        CreateIconCounterText(SlotIndex);
    }
    else
    {
        RemoveItemIcon(SlotIndex);
    }

    if (ForegroundBorders.IsValidIndex(SlotIndex))
    {
        ForegroundBorders[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
    }
}

void UInventory::RemoveItemIcon(uint64 SlotIndex)
{
    if (ForegroundBorders.IsValidIndex(SlotIndex))
    {
        if (TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent()))
        {
            SizeBox->SetContent(nullptr);
        }
    }
}

void UInventory::ShiftItems(uint64 StartIndex, uint64 EndIndex, EDirection Direction, bool bUpdateUI)
{
    if (!Items.IsValidIndex(StartIndex) || !Items.IsValidIndex(EndIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShiftItems: Invalid indices - Start=%d, End=%d"), StartIndex, EndIndex);
        return;
    }

    int32 Step = 0;
    uint64 MinIndex = FMath::Min(StartIndex, EndIndex);
    uint64 MaxIndex = FMath::Max(StartIndex, EndIndex);

    if (Direction == EDirection::Left || Direction == EDirection::Right)
    {
        Step = (Direction == EDirection::Right) ? 1 : -1;
        if (EndIndex / MaxColumns != StartIndex / MaxColumns)
            return;
    }
    else if (Direction == EDirection::Up || Direction == EDirection::Down)
    {
        Step = (Direction == EDirection::Down) ? static_cast<int32>(MaxColumns) : -static_cast<int32>(MaxColumns);
        if (EndIndex % MaxColumns != StartIndex % MaxColumns)
            return;
    }
    else
    {
        return;
    }

    TArray<FItem> TempItems;
    TArray<uint64> Indices;
    uint64 CurrentIndex = MinIndex;
    while (CurrentIndex <= MaxIndex)
    {
        TempItems.Add(Items[CurrentIndex]);
        Indices.Add(CurrentIndex);
        CurrentIndex = static_cast<uint64>(static_cast<int64>(CurrentIndex) + Step);
        if (CurrentIndex > MaxIndex)
            break;
    }

    if (StartIndex < EndIndex)
    {
        for (int32 i = 1; i < TempItems.Num(); ++i)
        {
            Items[Indices[i - 1]] = TempItems[i];
            if (bUpdateUI)
                UpdateSlotUI(Indices[i - 1]);
        }
        Items[EndIndex] = TempItems[0];
        if (bUpdateUI)
            UpdateSlotUI(EndIndex);
    }
    else
    {
        for (int32 i = 0; i < TempItems.Num() - 1; ++i)
        {
            Items[Indices[i + 1]] = TempItems[i];
            if (bUpdateUI)
                UpdateSlotUI(Indices[i + 1]);
        }
        Items[StartIndex] = TempItems[TempItems.Num() - 1];
        if (bUpdateUI)
            UpdateSlotUI(StartIndex);
    }
}

void UInventory::StartSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction)
{
    if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("StartSlideAnimation: Invalid indices - From=%d, To=%d"), FromIndex, ToIndex);
        return;
    }

    bIsSliding = true;
    SlideFromIndex = FromIndex;
    SlideToIndex = ToIndex;
    SlideProgress = 0.0f;
    SlidingItem = Items[FromIndex];

    // Clear previous sliding data
    SlidingOverlays.Empty();
    SlideFromIndices.Empty();
    SlideToIndices.Empty();
    SlidingItems.Empty();

    // Force layout update for all relevant widgets to ensure geometry is ready
    Canvas->ForceLayoutPrepass();
    Canvas->InvalidateLayoutAndVolatility();
    Grid->ForceLayoutPrepass();
    Grid->InvalidateLayoutAndVolatility();
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        if (Border)
        {
            Border->ForceLayoutPrepass();
            Border->InvalidateLayoutAndVolatility();
        }
    }

    // Determine the path and collect all items that will move
    int32 Step = 0;
    if (Direction == EDirection::Left || Direction == EDirection::Right)
    {
        Step = (Direction == EDirection::Right) ? 1 : -1;
        if (ToIndex / MaxColumns != FromIndex / MaxColumns)
            return;
    }
    else if (Direction == EDirection::Up || Direction == EDirection::Down)
    {
        Step = (Direction == EDirection::Down) ? static_cast<int32>(MaxColumns) : -static_cast<int32>(MaxColumns);
        if (ToIndex % MaxColumns != FromIndex % MaxColumns)
            return;
    }
    else
    {
        return;
    }

    uint64 MinIndex = FMath::Min(FromIndex, ToIndex);
    uint64 MaxIndex = FMath::Max(FromIndex, ToIndex);
    TArray<uint64> Indices;
    uint64 CurrentIndex = MinIndex;
    while (CurrentIndex <= MaxIndex)
    {
        Indices.Add(CurrentIndex);
        CurrentIndex = static_cast<uint64>(static_cast<int64>(CurrentIndex) + Step);
        if (CurrentIndex > MaxIndex)
            break;
    }

    // Clear the slots that will be animated
    for (uint64 Index : Indices)
    {
        if (ForegroundBorders.IsValidIndex(Index))
        {
            if (TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[Index]->GetContent()))
            {
                SizeBox->ClearChildren();
            }
        }
    }

    // Create drag overlays for all moving items
    for (int32 i = 0; i < Indices.Num(); ++i)
    {
        uint64 CurrentSlot = Indices[i];
        uint64 NextSlot = (FromIndex < ToIndex) ? Indices[(i + 1) % Indices.Num()] : Indices[i == 0 ? Indices.Num() - 1 : i - 1];

        SlidingItems.Add(Items[CurrentSlot]);
        SlideFromIndices.Add(CurrentSlot);
        SlideToIndices.Add(NextSlot);

        UOverlay* Overlay = CreateTemporaryDragOverlay(Items[CurrentSlot]);
        if (Overlay)
        {
            Canvas->AddChild(Overlay);
            UCanvasPanelSlot* OverlaySlot = Cast<UCanvasPanelSlot>(Overlay->Slot);
            if (OverlaySlot)
            {
                FVector2D ItemSize(100.0f, 100.0f);
                OverlaySlot->SetSize(ItemSize);
                OverlaySlot->SetZOrder(100);

                FVector2D FromPos = GetSlotPosition(CurrentSlot);
                if (FMath::IsNearlyZero(FromPos.X) && FMath::IsNearlyZero(FromPos.Y))
                {
                    UE_LOG(LogTemp, Warning, TEXT("StartSlideAnimation: Invalid start position for index %d, cancelling move"), CurrentSlot);

                    // Restore all items to their original slots
                    for (int32 j = 0; j < SlideFromIndices.Num(); ++j)
                    {
                        Items[SlideFromIndices[j]] = SlidingItems[j];
                        UpdateSlotUI(SlideFromIndices[j]);
                    }

                    // Clean up any overlays created so far
                    for (TObjectPtr<UOverlay> ExistingOverlay : SlidingOverlays)
                    {
                        if (ExistingOverlay)
                            Canvas->RemoveChild(ExistingOverlay);
                    }
                    SlidingOverlays.Empty();
                    SlideFromIndices.Empty();
                    SlideToIndices.Empty();
                    SlidingItems.Empty();

                    // Reset sliding state
                    bIsSliding = false;
                    SlideFromIndex = INDEX_NONE;
                    SlideToIndex = INDEX_NONE;
                    SlideProgress = 0.0f;
                    SlidingItem = FItem();

                    return;
                }

                OverlaySlot->SetPosition(FromPos - FVector2D(50.0f, 50.0f));
            }
            SlidingOverlays.Add(Overlay);
        }
    }

    // Perform the shift without updating the UI (since we're animating)
    ShiftItems(FromIndex, ToIndex, Direction, false);
}

FVector2D UInventory::GetSlotPosition(uint64 SlotIndex) const
{
    if (!ForegroundBorders.IsValidIndex(SlotIndex) || !ForegroundBorders[SlotIndex])
    {
        UE_LOG(LogTemp, Warning, TEXT("GetSlotPosition: Invalid slot index %d"), SlotIndex);
        return FVector2D::ZeroVector;
    }

    FGeometry SlotGeometry = ForegroundBorders[SlotIndex]->GetCachedGeometry();
    FVector2D SlotAbsTopLeft = SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
    FVector2D Result = SlotAbsTopLeft + FVector2D(50.0f, 50.0f);
    UE_LOG(LogTemp, Log, TEXT("GetSlotPosition: Index %d, Position (X=%f, Y=%f)"), SlotIndex, Result.X, Result.Y);
    return Result;
}

float UInventory::CustomEaseInOut(float T)
{
    T = FMath::Clamp(T, 0.0f, 1.0f);
    return T * T * (3.0f - 2.0f * T);
}

void UInventory::Create()
{
    if (!Grid || !WidgetTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid or WidgetTree is invalid in Create."));
        return;
    }

    Grid->ClearChildren();
    ForegroundBorders.Empty();
    IconSlots.Empty();
    Items.Empty();

    ForegroundBorders.SetNum(MaxRows * MaxColumns);
    Items.SetNum(MaxRows * MaxColumns);
    bCounterTextUpdated.Init(false, MaxRows * MaxColumns);

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
            SlotBorder->SetVisibility(ESlateVisibility::Visible);

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
