#include "Inventory.h"

uint64 UInventory::ItemCounter = 0;

UInventory::UInventory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer),
    DraggedItemIndex(INDEX_NONE),
    OriginalSlotIndex(INDEX_NONE),
    PreviousSlotIndex(INDEX_NONE),
    bIsDragging(false),
    bIsSliding(false),
    SlideFromIndex(INDEX_NONE),
    SlideToIndex(INDEX_NONE),
    SlideProgress(0.0f),
    SlideDuration(0.2f),
    bAnimationScheduled(false),
    ScheduledFromIndex(INDEX_NONE),
    ScheduledToIndex(INDEX_NONE),
    ScheduledDirection(EDirection::None),
    MoveCount(0)
{
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
    OriginalSlotIndex = INDEX_NONE;
    PreviousSlotIndex = INDEX_NONE;
    ItemCounter = 0;
    MoveCount = 0;

    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetTree is invalid"));
        return;
    }

    Canvas = NewObject<UCanvasPanel>(this);
    WidgetTree->RootWidget = Canvas;

    BackgroundBorder = NewObject<UBorder>(this);
    BackgroundBorder->SetBrushColor(FLinearColor::Gray);

    Title = NewObject<UTextBlock>(this);
    Title->SetText(FText::FromString(TEXT("Inventory")));

    UVerticalBox* ContentBox = NewObject<UVerticalBox>(this);
    UVerticalBoxSlot* TitleBoxSlot = ContentBox->AddChildToVerticalBox(Title);
    TitleBoxSlot->SetHorizontalAlignment(HAlign_Center);
    TitleBoxSlot->SetVerticalAlignment(VAlign_Top);
    TitleBoxSlot->SetPadding(FMargin(0, 10, 0, 10));

    Grid = NewObject<UUniformGridPanel>(this);
    Grid->SetSlotPadding(FMargin(15, 15, 15, 15));
    UVerticalBoxSlot* GridBoxSlot = ContentBox->AddChildToVerticalBox(Grid);
    GridBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    GridBoxSlot->SetVerticalAlignment(VAlign_Fill);

    BackgroundBorder->SetContent(ContentBox);
    BackgroundBorderSlot = Canvas->AddChildToCanvas(BackgroundBorder);
    BackgroundBorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
    BackgroundBorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    BackgroundBorderSlot->SetOffsets(FMargin(0, -100, 510.0f, 500.0f));

    SetRenderScale(FVector2D(1.0f, 1.0f));
    Create();
}

void UInventory::NativeConstruct()
{
    Super::NativeConstruct();

    if (BackgroundBorder) BackgroundBorder->ForceLayoutPrepass();
    if (Grid) Grid->ForceLayoutPrepass();
    if (Canvas) Canvas->ForceLayoutPrepass();
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        if (Border) Border->ForceLayoutPrepass();
    }
}

void UInventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bAnimationScheduled)
    {
        bool bGeometryReady = true;
        for (uint64 i = 0; i < ForegroundBorders.Num(); ++i)
        {
            FVector2D Position = GetSlotPosition(i);
            if (FMath::IsNearlyZero(Position.X) && FMath::IsNearlyZero(Position.Y))
            {
                bGeometryReady = false;
                break;
            }
        }

        if (bGeometryReady)
        {
            StartSlideAnimation(ScheduledFromIndex, ScheduledToIndex, ScheduledDirection);
            bAnimationScheduled = false;
            ScheduledFromIndex = INDEX_NONE;
            ScheduledToIndex = INDEX_NONE;
            ScheduledDirection = EDirection::None;
        }
        else
        {
            if (Canvas) Canvas->ForceLayoutPrepass();
            if (Grid) Grid->ForceLayoutPrepass();
            for (TObjectPtr<UBorder> Border : ForegroundBorders)
            {
                if (Border) Border->ForceLayoutPrepass();
            }
        }
    }

    if (bIsSliding)
    {
        SlideProgress += InDeltaTime / SlideDuration;
        if (SlideProgress >= 1.0f)
        {
            bIsSliding = false;
            SlideProgress = 0.0f;
            SlideFromIndex = INDEX_NONE;
            SlideToIndex = INDEX_NONE;
            SlidingItem = FItem();
            SlidingOverlays.Empty();
            SlideFromIndices.Empty();
            SlideToIndices.Empty();
            SlidingItems.Empty();

            if (Grid) Grid->ForceLayoutPrepass();
            if (Canvas) Canvas->ForceLayoutPrepass();
        }
    }
}

FReply UInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    {
        uint64 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
        if (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex) && Items[HoveredIndex].ReferencedActorClass)
        {
            DraggedItemIndex = HoveredIndex;
            OriginalSlotIndex = HoveredIndex;
            DraggedItem = Items[HoveredIndex];
            bIsDragging = true;

            // Clear the original slot to avoid duplication
            Items[OriginalSlotIndex] = FItem();
            UpdateSlotUI(OriginalSlotIndex);

            if (ForegroundBorders.IsValidIndex(DraggedItemIndex))
            {
                ForegroundBorders[DraggedItemIndex]->SetBrushColor(FLinearColor(0.9f, 0.0f, 0.9f, 1.0f));
            }

            UE_LOG(LogTemp, Log, TEXT("Started dragging item %d from slot %d"), DraggedItem.Index, HoveredIndex);
            return FReply::Handled();
        }
    }
    return FReply::Unhandled();
}

FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsDragging)
    {
        MoveItem(InMouseEvent, false, false);
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UInventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsDragging)
    {
        uint64 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
        if (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex))
        {
            // Clear previous highlights
            if (ForegroundBorders.IsValidIndex(OriginalSlotIndex))
            {
                ForegroundBorders[OriginalSlotIndex]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
            }
            if (PreviousSlotIndex != INDEX_NONE && ForegroundBorders.IsValidIndex(PreviousSlotIndex))
            {
                ForegroundBorders[PreviousSlotIndex]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
            }

            // Place the dragged item in the hovered slot
            Items[HoveredIndex] = DraggedItem;
            UpdateSlotUI(HoveredIndex);
            UE_LOG(LogTemp, Log, TEXT("Dropped item %d into slot %d"), DraggedItem.Index, HoveredIndex);
        }
        else
        {
            // Return to original slot if no valid slot is hovered
            Items[OriginalSlotIndex] = DraggedItem;
            UpdateSlotUI(OriginalSlotIndex);
            if (ForegroundBorders.IsValidIndex(OriginalSlotIndex))
            {
                ForegroundBorders[OriginalSlotIndex]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
            }
        }

        bIsDragging = false;
        DraggedItemIndex = INDEX_NONE;
        PreviousSlotIndex = INDEX_NONE;
        OriginalSlotIndex = INDEX_NONE;
        DraggedItem = FItem();

        if (Grid) Grid->ForceLayoutPrepass();
        if (Canvas) Canvas->ForceLayoutPrepass();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

void UInventory::AddItem(AActor* ItemActor)
{
    if (!ItemActor) return;

    if (ItemCounter >= 12)
    {
        bIsInventoryFull = true;
        return;
    }

    uint64 EmptySlotIndex = FindFirstEmptySlot();
    if (EmptySlotIndex == INDEX_NONE)
    {
        bIsInventoryFull = true;
        return;
    }

    Items[EmptySlotIndex] = FItem();
    Items[EmptySlotIndex].ReferencedActorClass = ItemActor->GetClass();
    Items[EmptySlotIndex].WorldLocation = ItemActor->GetActorLocation();
    Items[EmptySlotIndex].Index = ItemCounter;

    UpdateSlotUI(EmptySlotIndex);
    ItemActor->Destroy();
    ItemCounter++;
    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);
}

uint64 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent)
{
    if (!Grid || ForegroundBorders.Num() == 0) return INDEX_NONE;

    if (Canvas) Canvas->ForceLayoutPrepass();
    if (Grid) Grid->ForceLayoutPrepass();
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        if (Border) Border->ForceLayoutPrepass();
    }

    FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
    uint64 ClosestIndex = INDEX_NONE;
    float MinDistance = FLT_MAX;

    for (uint64 Row = 0; Row < MaxRows; Row++)
    {
        for (uint64 Col = 0; Col < MaxColumns; Col++)
        {
            uint64 Index = Row * MaxColumns + Col;
            if (!ForegroundBorders.IsValidIndex(Index) || !ForegroundBorders[Index]) continue;

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

    return ClosestIndex;
}

void UInventory::MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished)
{
    if (!bIsDragging) return;

    uint64 HoveredIndex = FindHoveredItemIndex(MouseEvent);
    if (HoveredIndex == INDEX_NONE || !Items.IsValidIndex(HoveredIndex)) return;

    if (PreviousSlotIndex != INDEX_NONE && PreviousSlotIndex != HoveredIndex && ForegroundBorders.IsValidIndex(PreviousSlotIndex))
    {
        ForegroundBorders[PreviousSlotIndex]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
    }

    if (ForegroundBorders.IsValidIndex(HoveredIndex))
    {
        ForegroundBorders[HoveredIndex]->SetBrushColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
    }

    PreviousSlotIndex = HoveredIndex;

    // Calculate direction
    uint64 FromRow = OriginalSlotIndex / MaxColumns;
    uint64 FromCol = OriginalSlotIndex % MaxColumns;
    uint64 ToRow = HoveredIndex / MaxColumns;
    uint64 ToCol = HoveredIndex % MaxColumns;
    EDirection Direction = GetMoveDirection(FromRow, FromCol, ToRow, ToCol);

    if (Items[HoveredIndex].ReferencedActorClass)
    {
        // Swap with occupied slot in real-time
        FItem TempItem = Items[HoveredIndex];
        Items[HoveredIndex] = DraggedItem;
        Items[OriginalSlotIndex] = TempItem;
        UpdateSlotUI(OriginalSlotIndex);
        UpdateSlotUI(HoveredIndex);
        DraggedItemIndex = HoveredIndex;
        OriginalSlotIndex = HoveredIndex; // Update OriginalSlotIndex to new position
    }
    else
    {
        // Move to empty slot
        Items[HoveredIndex] = DraggedItem;
        Items[OriginalSlotIndex] = FItem();
        UpdateSlotUI(OriginalSlotIndex);
        UpdateSlotUI(HoveredIndex);
        DraggedItemIndex = HoveredIndex;
        OriginalSlotIndex = HoveredIndex; // Update OriginalSlotIndex to new position
    }

    if (Grid) Grid->ForceLayoutPrepass();
    if (Canvas) Canvas->ForceLayoutPrepass();
}

EDirection UInventory::GetMoveDirection(uint64 RowA, uint64 ColA, uint64 RowB, uint64 ColB)
{
    if (RowA == RowB && ColA < ColB) return EDirection::Right;
    if (RowA == RowB && ColA > ColB) return EDirection::Left;
    if (ColA == ColB && RowA < RowB) return EDirection::Down;
    if (ColA == ColB && RowA > RowB) return EDirection::Up;
    return EDirection::None;
}

void UInventory::ShiftItems(uint64 StartIndex, uint64 EndIndex, EDirection Direction, bool bUpdateUI)
{
    if (!Items.IsValidIndex(StartIndex) || !Items.IsValidIndex(EndIndex)) return;
    if (StartIndex == EndIndex) return;

    int32 Step = 0;
    if (Direction == EDirection::Left || Direction == EDirection::Right)
    {
        Step = (Direction == EDirection::Right) ? 1 : -1;
        if (EndIndex / MaxColumns != StartIndex / MaxColumns) return;
    }
    else if (Direction == EDirection::Up || Direction == EDirection::Down)
    {
        Step = (Direction == EDirection::Down) ? static_cast<int32>(MaxColumns) : -static_cast<int32>(MaxColumns);
        if (EndIndex % MaxColumns != StartIndex % MaxColumns) return;
    }
    else
    {
        return;
    }

    TArray<uint64> Indices;
    int32 CurrentIndex = StartIndex;
    while (true)
    {
        Indices.Add(CurrentIndex);
        if (CurrentIndex == EndIndex) break;
        CurrentIndex += Step;
    }

    if (Step > 0)
    {
        for (int32 i = Indices.Num() - 1; i > 0; --i)
        {
            Items[Indices[i]] = Items[Indices[i - 1]];
            if (bUpdateUI) UpdateSlotUI(Indices[i]);
        }
    }
    else
    {
        for (int32 i = 0; i < Indices.Num() - 1; ++i)
        {
            Items[Indices[i]] = Items[Indices[i + 1]];
            if (bUpdateUI) UpdateSlotUI(Indices[i]);
        }
    }
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
        ForegroundBorders[SlotIndex]->ForceLayoutPrepass();
    }
}

void UInventory::CreateItemIcon(uint64 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !ForegroundBorders.IsValidIndex(SlotIndex)) return;

    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox) return;

    // Clear existing content to prevent duplication
    SizeBox->ClearChildren();

    UOverlay* IconOverlay = NewObject<UOverlay>(this);
    IconOverlay->SetVisibility(ESlateVisibility::Visible);
    SizeBox->SetContent(IconOverlay);

    UImage* ItemIcon = NewObject<UImage>(this);
    ItemIcon->SetVisibility(ESlateVisibility::Visible);
    UOverlaySlot* ImageSlot = IconOverlay->AddChildToOverlay(ItemIcon);
    ImageSlot->SetHorizontalAlignment(HAlign_Fill);
    ImageSlot->SetVerticalAlignment(VAlign_Fill);

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
    if (!ForegroundBorders.IsValidIndex(SlotIndex) || !Items.IsValidIndex(SlotIndex)) return;
    if (!Items[SlotIndex].ReferencedActorClass) return;

    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox) return;

    UOverlay* IconOverlay = Cast<UOverlay>(SizeBox->GetContent());
    if (!IconOverlay)
    {
        IconOverlay = NewObject<UOverlay>(this);
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
        ItemCounterText = NewObject<UTextBlock>(this);
        ItemCounterText->SetVisibility(ESlateVisibility::Visible);
        UOverlaySlot* TextOverlaySlot = IconOverlay->AddChildToOverlay(ItemCounterText);
        TextOverlaySlot->SetHorizontalAlignment(HAlign_Center);
        TextOverlaySlot->SetVerticalAlignment(VAlign_Center);
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
        if (!Items[i].ReferencedActorClass) return i;
    }
    return INDEX_NONE;
}

void UInventory::RemoveItemIcon(uint64 SlotIndex)
{
    if (ForegroundBorders.IsValidIndex(SlotIndex))
    {
        if (TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent()))
        {
            SizeBox->ClearChildren();
            SizeBox->ForceLayoutPrepass();
        }
    }
}

void UInventory::Create()
{
    if (!Grid || !WidgetTree) return;

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

            UBorder* SlotBorder = NewObject<UBorder>(this);
            SlotBorder->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
            SlotBorder->SetVisibility(ESlateVisibility::Visible);

            USizeBox* SizeBox = NewObject<USizeBox>(this);
            SizeBox->SetWidthOverride(100.0f);
            SizeBox->SetHeightOverride(100.0f);

            SlotBorder->SetContent(SizeBox);
            GridSlot = Grid->AddChildToUniformGrid(SlotBorder, Rows, Columns);
            GridSlot->SetHorizontalAlignment(HAlign_Center);
            GridSlot->SetVerticalAlignment(VAlign_Center);

            ForegroundBorders[Index] = SlotBorder;
            SlotBorder->ForceLayoutPrepass();
            SizeBox->ForceLayoutPrepass();
        }
    }

    if (Grid) Grid->ForceLayoutPrepass();
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

// Removed unused methods
EDirection UInventory::SortItem(FItem& MovedItem, FItem& ItemToMove)
{
    return EDirection::None;
}

int32 UInventory::FindItemIndex(const FItem& TargetItem) const
{
    return INDEX_NONE;
}

void UInventory::ScheduleSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction)
{
}

void UInventory::StartSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction)
{
}

FVector2D UInventory::GetSlotPosition(uint64 SlotIndex) const
{
    return FVector2D::ZeroVector;
}

float UInventory::CustomEaseInOut(float T)
{
    return T;
}
