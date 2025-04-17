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
    SlideDuration(0.0f),
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
    if (!Canvas)
    {
        UE_LOG(LogTemp, Warning, TEXT("Canvas is invalid"));
        return;
    }
    WidgetTree->RootWidget = Canvas;

    BackgroundBorder = NewObject<UBorder>(this);
    if (!BackgroundBorder)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundBorder is invalid"));
        return;
    }
    BackgroundBorder->SetBrushColor(FLinearColor::Gray);

    Title = NewObject<UTextBlock>(this);
    if (!Title)
    {
        UE_LOG(LogTemp, Warning, TEXT("Title is invalid"));
        return;
    }
    Title->SetText(FText::FromString(TEXT("Inventory")));

    UVerticalBox* ContentBox = NewObject<UVerticalBox>(this);
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

    Grid = NewObject<UUniformGridPanel>(this);
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

    if (BackgroundBorder)
    {
        BackgroundBorder->ForceLayoutPrepass();
        BackgroundBorder->InvalidateLayoutAndVolatility();
    }
    if (Grid)
    {
        Grid->ForceLayoutPrepass();
        Grid->InvalidateLayoutAndVolatility();
    }
    if (Canvas)
    {
        Canvas->ForceLayoutPrepass();
        Canvas->InvalidateLayoutAndVolatility();
    }

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
            if (Canvas)
            {
                Canvas->ForceLayoutPrepass();
                Canvas->InvalidateLayoutAndVolatility();
            }
            if (Grid)
            {
                Grid->ForceLayoutPrepass();
                Grid->InvalidateLayoutAndVolatility();
            }
            for (TObjectPtr<UBorder> Border : ForegroundBorders)
            {
                if (Border)
                {
                    Border->ForceLayoutPrepass();
                    Border->InvalidateLayoutAndVolatility();
                }
            }
        }
    }

    // Update drag overlay position
    if (bIsDragging && DragOverlay.IsValid())
    {
        FVector2D MousePos = GetWorld()->GetGameViewport()->GetMousePosition();
        UCanvasPanelSlot* OverlaySlot = Cast<UCanvasPanelSlot>(DragOverlay->Slot);
        if (OverlaySlot)
        {
            OverlaySlot->SetPosition(MousePos - FVector2D(50.0f, 50.0f)); // Center overlay on mouse
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
            PreviousSlotIndex = HoveredIndex;
            DraggedItem = Items[HoveredIndex];
            bIsDragging = true;

            // Create temporary drag overlay
            DragOverlay = CreateTemporaryDragOverlay(DraggedItem);
            if (DragOverlay.IsValid())
            {
                UCanvasPanelSlot* OverlaySlot = Canvas->AddChildToCanvas(DragOverlay.Get());
                if (OverlaySlot)
                {
                    OverlaySlot->SetPosition(InMouseEvent.GetScreenSpacePosition() - FVector2D(50.0f, 50.0f));
                    OverlaySlot->SetSize(FVector2D(100.0f, 100.0f));
                    OverlaySlot->SetZOrder(100); // Ensure overlay is on top
                }
            }

            // Highlight dragged slot
            if (ForegroundBorders.IsValidIndex(DraggedItemIndex))
            {
                ForegroundBorders[DraggedItemIndex]->SetBrushColor(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f));
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
        // Move to empty slot if hovered over one
        uint64 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
        if (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex) && !Items[HoveredIndex].ReferencedActorClass && HoveredIndex != DraggedItemIndex)
        {
            Items[HoveredIndex] = DraggedItem;
            Items[DraggedItemIndex] = FItem();
            UpdateSlotUI(DraggedItemIndex);
            UpdateSlotUI(HoveredIndex);
            UE_LOG(LogTemp, Log, TEXT("Dropped item %d onto empty slot %d"), DraggedItem.Index, HoveredIndex);
        }

        // Reset dragging state
        if (ForegroundBorders.IsValidIndex(DraggedItemIndex))
        {
            ForegroundBorders[DraggedItemIndex]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
        }
        if (PreviousSlotIndex != INDEX_NONE && ForegroundBorders.IsValidIndex(PreviousSlotIndex))
        {
            ForegroundBorders[PreviousSlotIndex]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
        }

        // Remove drag overlay
        RemoveTemporaryDragOverlay();

        bIsDragging = false;
        DraggedItemIndex = INDEX_NONE;
        PreviousSlotIndex = INDEX_NONE;
        OriginalSlotIndex = INDEX_NONE;
        DraggedItem = FItem();

        // Force layout update
        if (Grid)
        {
            Grid->ForceLayoutPrepass();
            Grid->InvalidateLayoutAndVolatility();
        }
        if (Canvas)
        {
            Canvas->ForceLayoutPrepass();
            Canvas->InvalidateLayoutAndVolatility();
        }

        return FReply::Handled();
    }
    return FReply::Unhandled();
}

FReply UInventory::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
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

    ItemCounter++;
    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);
}

uint64 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent)
{
    if (!Grid || ForegroundBorders.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid or ForegroundBorders are missing!"));
        return INDEX_NONE;
    }

    if (Canvas)
    {
        Canvas->ForceLayoutPrepass();
        Canvas->InvalidateLayoutAndVolatility();
    }
    if (Grid)
    {
        Grid->ForceLayoutPrepass();
        Grid->InvalidateLayoutAndVolatility();
    }
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

    UOverlay* IconOverlay = NewObject<UOverlay>(this);
    if (!IconOverlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create UOverlay for slot %d"), SlotIndex);
        return;
    }
    IconOverlay->SetVisibility(ESlateVisibility::Visible);
    SizeBox->SetContent(IconOverlay);

    UImage* ItemIcon = NewObject<UImage>(this);
    if (!ItemIcon)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create UImage for slot %d"), SlotIndex);
        IconOverlay->RemoveFromParent();
        return;
    }
    ItemIcon->SetVisibility(ESlateVisibility::Visible);
    UOverlaySlot* ImageSlot = IconOverlay->AddChildToOverlay(ItemIcon);
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

    UOverlay* IconOverlay = Cast<UOverlay>(SizeBox->GetContent());
    if (!IconOverlay)
    {
        IconOverlay = NewObject<UOverlay>(this);
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
        ItemCounterText = NewObject<UTextBlock>(this);
        if (!ItemCounterText)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create UTextBlock for slot %d"), SlotIndex);
            return;
        }
        ItemCounterText->SetVisibility(ESlateVisibility::Visible);
        UOverlaySlot* TextOverlaySlot = IconOverlay->AddChildToOverlay(ItemCounterText);
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

void UInventory::MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished)
{
    if (!bIsDragging)
    {
        return;
    }

    if (bItemMovementStarted || bItemMovementFinished)
    {
        return;
    }

    // Find the currently hovered slot
    uint64 HoveredIndex = FindHoveredItemIndex(MouseEvent);
    if (HoveredIndex == INDEX_NONE || HoveredIndex == DraggedItemIndex)
    {
        return;
    }

    // Highlight the hovered slot
    if (ForegroundBorders.IsValidIndex(HoveredIndex))
    {
        ForegroundBorders[HoveredIndex]->SetBrushColor(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
    }
    if (PreviousSlotIndex != INDEX_NONE && PreviousSlotIndex != HoveredIndex && ForegroundBorders.IsValidIndex(PreviousSlotIndex))
    {
        ForegroundBorders[PreviousSlotIndex]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
    }

    // Handle sorting for occupied slot
    if (Items.IsValidIndex(HoveredIndex) && Items[HoveredIndex].ReferencedActorClass)
    {
        SortItem(DraggedItemIndex, HoveredIndex);
        DraggedItemIndex = HoveredIndex; // Update dragged index to new position
        UE_LOG(LogTemp, Log, TEXT("Swapped item %d with slot %d during drag"), DraggedItem.Index, HoveredIndex);
    }

    PreviousSlotIndex = HoveredIndex;
}

EDirection UInventory::GetMoveDirection(uint64 RowA, uint64 ColA, uint64 RowB, uint64 ColB)
{
    if (RowA == RowB && ColA < ColB) return EDirection::Right;
    if (RowA == RowB && ColA > ColB) return EDirection::Left;
    if (ColA == ColB && RowA < RowB) return EDirection::Down;
    if (ColA == ColB && RowA > RowB) return EDirection::Up;
    return EDirection::None;
}

EDirection UInventory::SortItem(uint64 FromIndex, uint64 ToIndex)
{
    if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("SortItem: Invalid indices - From=%d, To=%d"), FromIndex, ToIndex);
        return EDirection::None;
    }

    // Swap the items in the Items array
    FItem TempItem = Items[FromIndex];
    Items[FromIndex] = Items[ToIndex];
    Items[ToIndex] = TempItem;

    // Update the UI for both slots
    UpdateSlotUI(FromIndex);
    UpdateSlotUI(ToIndex);

    // Determine direction for animation
    uint64 FromRow = FromIndex / MaxColumns;
    uint64 FromCol = FromIndex % MaxColumns;
    uint64 ToRow = ToIndex / MaxColumns;
    uint64 ToCol = ToIndex % MaxColumns;

    EDirection Direction = GetMoveDirection(FromRow, FromCol, ToRow, ToCol);
    if (Direction != EDirection::None)
    {
        ScheduleSlideAnimation(FromIndex, ToIndex, Direction);
        jätt

            // Force layout update
            if (Grid)
            {
                Grid->ForceLayoutPrepass();
                Grid->InvalidateLayoutAndVolatility();
            }
        if (Canvas)
        {
            Canvas->ForceLayoutPrepass();
            Canvas->InvalidateLayoutAndVolatility();
        }

        return Direction;
    }

    int32 UInventory::FindItemIndex(const FItem & TargetItem) const
    {
        for (int32 i = 0; i < Items.Num(); ++i)
        {
            if (Items[i].Index == TargetItem.Index && Items[i].ReferencedActorClass)
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
            ForegroundBorders[SlotIndex]->ForceLayoutPrepass();
            ForegroundBorders[SlotIndex]->InvalidateLayoutAndVolatility();
        }
    }

    void UInventory::RemoveItemIcon(uint64 SlotIndex)
    {
        if (ForegroundBorders.IsValidIndex(SlotIndex))
        {
            if (TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent()))
            {
                SizeBox->SetContent(nullptr);
                SizeBox->ForceLayoutPrepass();
                SizeBox->InvalidateLayoutAndVolatility();
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

        if (StartIndex == EndIndex)
        {
            return;
        }

        // Swap the items at StartIndex and EndIndex
        FItem TempItem = Items[StartIndex];
        Items[StartIndex] = Items[EndIndex];
        Items[EndIndex] = TempItem;

        // Update UI for the affected slots
        if (bUpdateUI)
        {
            UpdateSlotUI(StartIndex);
            UpdateSlotUI(EndIndex);
        }

        UE_LOG(LogTemp, Log, TEXT("ShiftItems: Swapped item from %d to %d"), StartIndex, EndIndex);
    }

    void UInventory::ScheduleSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction)
    {
        bAnimationScheduled = true;
        ScheduledFromIndex = FromIndex;
        ScheduledToIndex = ToIndex;
        ScheduledDirection = Direction;

        if (Canvas)
        {
            Canvas->ForceLayoutPrepass();
            Canvas->InvalidateLayoutAndVolatility();
        }
        if (Grid)
        {
            Grid->ForceLayoutPrepass();
            Grid->InvalidateLayoutAndVolatility();
        }
        for (TObjectPtr<UBorder> Border : ForegroundBorders)
        {
            if (Border)
            {
                Border->ForceLayoutPrepass();
                Border->InvalidateLayoutAndVolatility();
            }
        }
    }

    void UInventory::StartSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction)
    {
        if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("StartSlideAnimation: Invalid indices - From=%d, To=%d"), FromIndex, ToIndex);
            return;
        }

        if (SlideDuration <= 0.0f)
        {
            ShiftItems(FromIndex, ToIndex, Direction, true);
            return;
        }

        bIsSliding = true;
        SlideFromIndex = FromIndex;
        SlideToIndex = ToIndex;
        SlideProgress = 0.0f;
        SlidingItem = Items[FromIndex];

        SlidingOverlays.Empty();
        SlideFromIndices.Empty();
        SlideToIndices.Empty();
        SlidingItems.Empty();

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

        bool bForward = (FromIndex < ToIndex);
        for (int32 i = 0; i < Indices.Num(); ++i)
        {
            uint64 CurrentSlot = Indices[i];
            int32 NextIndex = bForward ? (i + 1) % Indices.Num() : (i - 1 + Indices.Num()) % Indices.Num();
            uint64 NextSlot = Indices[NextIndex];

            SlidingItems.Add(Items[CurrentSlot]);
            SlideFromIndices.Add(CurrentSlot);
            SlideToIndices.Add(NextSlot);
        }

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

                UBorder* SlotBorder = NewObject<UBorder>(this);
                if (!SlotBorder)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to create SlotBorder at Index=%d"), Index);
                    continue;
                }
                SlotBorder->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
                SlotBorder->SetVisibility(ESlateVisibility::Visible);

                USizeBox* SizeBox = NewObject<USizeBox>(this);
                if (!SizeBox)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to create SizeBox at Index=%d"), Index);
                    SlotBorder->RemoveFromParent();
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
                    SlotBorder->RemoveFromParent();
                    continue;
                }

                ForegroundBorders[Index] = SlotBorder;

                SlotBorder->ForceLayoutPrepass();
                SlotBorder->InvalidateLayoutAndVolatility();
                SizeBox->ForceLayoutPrepass();
                SizeBox->InvalidateLayoutAndVolatility();
            }
        }

        if (Grid)
        {
            Grid->ForceLayoutPrepass();
            Grid->InvalidateLayoutAndVolatility();
        }
    }

    UOverlay* UInventory::FindDraggedOverlay(uint64 ItemIndex)
    {
        return DragOverlay.Get();
    }

    UOverlay* UInventory::CreateTemporaryDragOverlay(const FItem & Item)
    {
        UOverlay* Overlay = NewObject<UOverlay>(this);
        if (!Overlay)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create drag overlay"));
            return nullptr;
        }
        Overlay->SetVisibility(ESlateVisibility::Visible);

        UImage* ItemIcon = NewObject<UImage>(this);
        if (!ItemIcon)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create drag overlay image"));
            Overlay->RemoveFromParent();
            return nullptr;
        }
        ItemIcon->SetVisibility    if (Item.IconTexture.IsValid())
        {
            ItemIcon->SetBrushFromTexture(Item.IconTexture.Get());
        }
        else
        {
            ItemIcon->SetColorAndOpacity(FLinearColor::Blue);
        }

        UOverlaySlot* ImageSlot = Overlay->AddChildToOverlay(ItemIcon);
        if (ImageSlot)
        {
            ImageSlot->SetHorizontalAlignment(HAlign_Fill);
            ImageSlot->SetVerticalAlignment(VAlign_Fill);
        }

        UTextBlock* CounterText = NewObject<UTextBlock>(this);
        if (!CounterText)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create drag overlay text"));
            Overlay->RemoveFromParent();
            return nullptr;
        }
        CounterText->SetText(FText::AsNumber(Item.Index));
        CounterText->SetColorAndOpacity(FLinearColor::Red);
        CounterText->SetJustification(ETextJustify::Center);
        CounterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));

        UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(CounterText);
        if (TextSlot)
        {
            TextSlot->SetHorizontalAlignment(HAlign_Center);
            TextSlot->SetVerticalAlignment(VAlign_Center);
        }

        return Overlay;
    }

    void UInventory::RemoveTemporaryDragOverlay()
    {
        if (DragOverlay.IsValid())
        {
            DragOverlay->RemoveFromParent();
            DragOverlay = nullptr;
        }
    }
