#include "Inventory.h"

uint64 UInventory::ItemCounter = 0;

UInventory::UInventory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer),
    bIsInventoryFull(false),
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
    MoveCount(0),
    DraggedItemWidget(nullptr),
    bDragStarted(false),
    DragStartPosition(FVector2D::ZeroVector),
    MaxRows(3),
    MaxColumns(4)
{
    Items.SetNum(MaxRows * MaxColumns);
    ForegroundBorders.SetNum(MaxRows * MaxColumns);
    IconSlots.SetNum(MaxRows * MaxColumns);
    bCounterTextUpdated.SetNum(MaxRows * MaxColumns);
}

void UInventory::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    bIsInventoryFull = false;
    DraggedItemIndex = INDEX_NONE;
    OriginalSlotIndex = INDEX_NONE;
    PreviousSlotIndex = INDEX_NONE;
    bIsDragging = false;
    bIsSliding = false;
    SlideFromIndex = INDEX_NONE;
    SlideToIndex = INDEX_NONE;
    SlideProgress = 0.0f;
    bAnimationScheduled = false;
    ScheduledFromIndex = INDEX_NONE;
    ScheduledToIndex = INDEX_NONE;
    ScheduledDirection = EDirection::None;
    MoveCount = 0;
    bDragStarted = false;
    DragStartPosition = FVector2D::ZeroVector;

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
    TitleBoxSlot->SetPadding(FMargin(10, 10, 10, 10));

    Grid = NewObject<UUniformGridPanel>(this);
    Grid->SetSlotPadding(FMargin(10, 10, 10, 10));
    UVerticalBoxSlot* GridBoxSlot = ContentBox->AddChildToVerticalBox(Grid);
    GridBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    GridBoxSlot->SetVerticalAlignment(VAlign_Fill);
    GridBoxSlot->SetPadding(FMargin(10, 10, 10, 10));

    BackgroundBorder->SetContent(ContentBox);
    BackgroundBorderSlot = Canvas->AddChildToCanvas(BackgroundBorder);
    BackgroundBorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
    BackgroundBorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    BackgroundBorderSlot->SetOffsets(FMargin(0, -100, 510.0f, 500.0f));

    SetRenderScale(FVector2D(1.0f, 1.0f));
    Create();

    if (GEngine && GEngine->GameViewport)
    {
        FVector2D ViewportSize;
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        UE_LOG(LogTemp, Log, TEXT("Viewport Size: %s"), *ViewportSize.ToString());

        if (BackgroundBorder)
        {
            FGeometry BorderGeometry = BackgroundBorder->GetCachedGeometry();
            FVector2D BorderTopLeft = BorderGeometry.LocalToAbsolute(FVector2D::ZeroVector);
            FVector2D BorderSize = BorderGeometry.GetLocalSize();
            UE_LOG(LogTemp, Log, TEXT("BackgroundBorder: TopLeft=%s, Size=%s"), *BorderTopLeft.ToString(), *BorderSize.ToString());
        }
    }
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
        for (int32 i = 0; i < ForegroundBorders.Num(); ++i)
        {
            FVector2D Position = GetSlotPosition(i);
            FGeometry SlotGeometry = ForegroundBorders[i]->GetCachedGeometry();
            FVector2D SlotSize = SlotGeometry.GetLocalSize();
            if (SlotSize.X < 10.0f || SlotSize.Y < 10.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("NativeTick: Slot %d geometry invalid: Position=%s, Size=%s"),
                    i, *Position.ToString(), *SlotSize.ToString());
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
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (Canvas) Canvas->ForceLayoutPrepass();
        if (Grid)   Grid->ForceLayoutPrepass();
        for (TObjectPtr<UBorder> Border : ForegroundBorders)
        {
            if (Border) Border->ForceLayoutPrepass();
        }

        uint64 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
        if (HoveredIndex != INDEX_NONE
            && Items.IsValidIndex(HoveredIndex)
            && Items[HoveredIndex].ReferencedActorClass)
        {
            // Immediately begin dragging
            DraggedItemIndex   = HoveredIndex;
            OriginalSlotIndex  = HoveredIndex;
            DraggedItem        = Items[HoveredIndex];

            // Remove the item from its slot right away so it never shows in the grid
            Items[OriginalSlotIndex] = FItem();
            UpdateSlotUI(OriginalSlotIndex);

            bIsDragging        = true;
            bDragStarted       = true;  // no threshold—drag starts immediately
            DragStartPosition  = InGeometry.AbsoluteToLocal(
                InMouseEvent.GetScreenSpacePosition());

            UE_LOG(LogTemp, Log, TEXT(
                "NativeOnMouseButtonDown: Picked up item %d from slot %d"),
                DraggedItem.Index, HoveredIndex);

            // — Create floating “drag icon” immediately —
            if (Canvas)
            {
                DraggedItemWidget = NewObject<UOverlay>(this);
                DraggedItemWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

                // Icon image
                UImage* Icon = NewObject<UImage>(this);
                Icon->SetBrushFromTexture(
                    DraggedItem.IconTexture ? DraggedItem.IconTexture.Get() : nullptr);
                Icon->SetVisibility(ESlateVisibility::Visible);
                if (!DraggedItem.IconTexture)
                {
                    Icon->SetColorAndOpacity(FLinearColor::Blue);
                }
                UOverlaySlot* ImageSlot = DraggedItemWidget->AddChildToOverlay(Icon);
                ImageSlot->SetHorizontalAlignment(HAlign_Fill);
                ImageSlot->SetVerticalAlignment(VAlign_Fill);

                // (Optional) item index text
                UTextBlock* ItemCounterText = NewObject<UTextBlock>(this);
                ItemCounterText->SetVisibility(ESlateVisibility::Visible);
                ItemCounterText->SetText(FText::AsNumber(DraggedItem.Index));
                ItemCounterText->SetColorAndOpacity(FLinearColor::Red);
                ItemCounterText->SetJustification(ETextJustify::Center);
                ItemCounterText->SetFont(
                    FCoreStyle::GetDefaultFontStyle("Regular", 20));
                UOverlaySlot* TextSlot = DraggedItemWidget->AddChildToOverlay(ItemCounterText);
                TextSlot->SetHorizontalAlignment(HAlign_Center);
                TextSlot->SetVerticalAlignment(VAlign_Center);

                // Add to canvas so it floats
                if (UCanvasPanelSlot* CanvasSlot =
                        Canvas->AddChildToCanvas(DraggedItemWidget))
                {
                    FVector2D MouseLocalPos =
                        InGeometry.AbsoluteToLocal(
                            InMouseEvent.GetScreenSpacePosition());
                    CanvasSlot->SetSize(FVector2D(100, 100));
                    CanvasSlot->SetZOrder(100);
                    CanvasSlot->SetPosition(MouseLocalPos - FVector2D(50, 50));
                }
            }

            // Capture mouse so MouseMove/MouseUp still occur even if cursor leaves
            if (TSharedPtr<SWidget> SlateWidget = GetCachedWidget())
            {
                return FReply::Handled().CaptureMouse(SlateWidget.ToSharedRef());
            }
            return FReply::Handled();
        }
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}


FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsDragging && bDragStarted && DraggedItemWidget && Canvas)
    {
        // Update floating widget’s position to follow cursor
        const FVector2D MouseScreenPos = InMouseEvent.GetScreenSpacePosition();
        const FVector2D MouseLocalPos  = InGeometry.AbsoluteToLocal(MouseScreenPos);

        if (UCanvasPanelSlot* CanvasSlot =
                Cast<UCanvasPanelSlot>(DraggedItemWidget->Slot))
        {
            CanvasSlot->SetPosition(MouseLocalPos - FVector2D(50, 50));
        }

        // (Optional) If you want grid‐cell highlighting or pre‐move logic:
        // MoveItem(InMouseEvent, false, false);

        return FReply::Handled();
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}


FReply UInventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bIsDragging)
    {
        return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    }

    // Ensure layout is up-to-date
    if (Canvas) Canvas->ForceLayoutPrepass();
    if (Grid)   Grid->ForceLayoutPrepass();
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        if (Border) Border->ForceLayoutPrepass();
    }

    const uint64 HoveredIndex = FindHoveredItemIndex(InMouseEvent);
    const bool   bValidDrop   =
        (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex));

    // Remove the floating drag icon
    if (DraggedItemWidget && Canvas)
    {
        Canvas->RemoveChild(DraggedItemWidget);
        DraggedItemWidget = nullptr;
    }

    if (bValidDrop && HoveredIndex != OriginalSlotIndex)
    {
        // Swap or move without touching .Index fields
        if (Items[HoveredIndex].ReferencedActorClass)
        {
            FItem Temp             = Items[HoveredIndex];
            Items[HoveredIndex]    = DraggedItem;
            Items[OriginalSlotIndex] = Temp;
        }
        else
        {
            Items[HoveredIndex]      = DraggedItem;
            // The original slot stays empty
        }

        UpdateSlotUI(OriginalSlotIndex);
        UpdateSlotUI(HoveredIndex);
    }
    else
    {
        // Dropped outside or back on same slot
        if (!bValidDrop || HoveredIndex == INDEX_NONE)
        {
            // Remove the item entirely (slot is already empty)
            RemoveItem(OriginalSlotIndex);
        }
        else
        {
            // Dropped back onto original—reinsert item
            Items[OriginalSlotIndex] = DraggedItem;
            UpdateSlotUI(OriginalSlotIndex);
        }
    }

    // Reset dragging state and release capture
    bIsDragging       = false;
    bDragStarted      = false;
    DraggedItemIndex  = INDEX_NONE;
    PreviousSlotIndex = INDEX_NONE;
    OriginalSlotIndex = INDEX_NONE;
    DraggedItem       = FItem();
    DragStartPosition = FVector2D::ZeroVector;

    return FReply::Handled().ReleaseMouseCapture();
}

void UInventory::AddItem(AActor* ItemActor)
{
    if (!ItemActor) return;

    if (ItemCounter >= static_cast<uint64>(MaxRows * MaxColumns))
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

void UInventory::RemoveItem(uint64 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].ReferencedActorClass)
        return;

    Items[SlotIndex] = FItem();
    RemoveItemIcon(SlotIndex);
    if (ItemCounter > 0) ItemCounter--;

    uint64 NewIndex = 0;
    for (uint64 i = 0; i < static_cast<uint64>(Items.Num()); ++i)
    {
        if (Items[i].ReferencedActorClass)
        {
            Items[i].Index = NewIndex++;
            CreateIconCounterText(i);
        }
    }

    ItemCounter = NewIndex;
    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);
}

uint64 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent)
{
    if (!Grid || ForegroundBorders.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("FindHoveredItemIndex: Grid or ForegroundBorders invalid"));
        return INDEX_NONE;
    }

    if (Canvas) Canvas->ForceLayoutPrepass();
    if (Grid) Grid->ForceLayoutPrepass();
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        if (Border) Border->ForceLayoutPrepass();
    }

    FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
    uint64 ClosestIndex = INDEX_NONE;
    float MinDistance = FLT_MAX;

    bool bAnyValidGeometry = false;
    for (uint64 Row = 0; Row < MaxRows; Row++)
    {
        for (uint64 Col = 0; Col < MaxColumns; Col++)
        {
            uint64 Index = Row * MaxColumns + Col;
            if (!ForegroundBorders.IsValidIndex(Index) || !ForegroundBorders[Index])
            {
                UE_LOG(LogTemp, Warning, TEXT("FindHoveredItemIndex: Slot %d invalid or null"), Index);
                continue;
            }

            FGeometry SlotGeometry = ForegroundBorders[Index]->GetCachedGeometry();
            FVector2D SlotAbsTopLeft = SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
            FVector2D SlotAbsSize = SlotGeometry.GetLocalSize();
            FVector2D SlotAbsBottomRight = SlotAbsTopLeft + SlotAbsSize;

            // Log actual slot bounds
            UE_LOG(LogTemp, Log, TEXT("Slot %d (Row=%d, Col=%d) Actual Bounds: TopLeft=%s, BottomRight=%s, Size=%s"),
                Index, Row, Col, *SlotAbsTopLeft.ToString(), *SlotAbsBottomRight.ToString(), *SlotAbsSize.ToString());

            const float Margin = 5.0f;
            FVector2D AdjustedTopLeft = SlotAbsTopLeft + FVector2D(Margin, Margin);
            FVector2D AdjustedBottomRight = SlotAbsBottomRight - FVector2D(Margin, Margin);

            UE_LOG(LogTemp, Log, TEXT("FindHoveredItemIndex: Slot %d, AdjustedTopLeft=%s, AdjustedBottomRight=%s, MousePos=%s"),
                Index, *AdjustedTopLeft.ToString(), *AdjustedBottomRight.ToString(), *MousePos.ToString());

            if (SlotAbsSize.X < 10.0f || SlotAbsSize.Y < 10.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("FindHoveredItemIndex: Slot %d geometry too small: Size=%s"),
                    Index, *SlotAbsSize.ToString());
                continue;
            }

            bAnyValidGeometry = true;

            if (MousePos.X >= AdjustedTopLeft.X && MousePos.X <= AdjustedBottomRight.X &&
                MousePos.Y >= AdjustedTopLeft.Y && MousePos.Y <= AdjustedBottomRight.Y)
            {
                FVector2D SlotCenter = AdjustedTopLeft + ((AdjustedBottomRight - AdjustedTopLeft) / 2.0f);
                float Distance = FVector2D::Distance(MousePos, SlotCenter);
                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    ClosestIndex = Index;
                    UE_LOG(LogTemp, Log, TEXT("FindHoveredItemIndex: Slot %d hit, Distance=%f"), Index, Distance);
                }
            }
        }
    }

    if (ClosestIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("FindHoveredItemIndex: No valid slot hit. AnyValidGeometry=%s, MousePos=%s"),
            bAnyValidGeometry ? TEXT("True") : TEXT("False"), *MousePos.ToString());
    }

    return ClosestIndex;
}

void UInventory::MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished)
{
    if (!bIsDragging)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveItem: Not dragging, exiting"));
        return;
    }

    uint64 HoveredIndex = FindHoveredItemIndex(MouseEvent);
    UE_LOG(LogTemp, Log, TEXT("MoveItem: HoveredIndex=%d, OriginalSlotIndex=%d, DraggedItemIndex=%d"),
        HoveredIndex, OriginalSlotIndex, DraggedItemIndex);

    if (HoveredIndex == INDEX_NONE || !Items.IsValidIndex(HoveredIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveItem: Invalid HoveredIndex=%d or out of bounds"), HoveredIndex);
        return;
    }

    if (PreviousSlotIndex != INDEX_NONE && PreviousSlotIndex != HoveredIndex && ForegroundBorders.IsValidIndex(PreviousSlotIndex))
    {
        ForegroundBorders[PreviousSlotIndex]->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
    }

    if (ForegroundBorders.IsValidIndex(HoveredIndex))
    {
       // ForegroundBorders[HoveredIndex]->SetBrushColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
    }

    PreviousSlotIndex = HoveredIndex;

    if (HoveredIndex == OriginalSlotIndex)
    {
        UE_LOG(LogTemp, Log, TEXT("MoveItem: HoveredIndex same as OriginalSlotIndex, skipping"));
        return;
    }

    uint64 FromRow = OriginalSlotIndex / MaxColumns;
    uint64 FromCol = OriginalSlotIndex % MaxColumns;
    uint64 ToRow = HoveredIndex / MaxColumns;
    uint64 ToCol = HoveredIndex % MaxColumns;
    EDirection Direction = GetMoveDirection(FromRow, FromCol, ToRow, ToCol);
    UE_LOG(LogTemp, Log, TEXT("MoveItem: Moving from (%d,%d) to (%d,%d), Direction=%d"),
        FromRow, FromCol, ToRow, ToCol, (uint8)Direction);

    if (Items[HoveredIndex].ReferencedActorClass)
    {
        FItem TempItem = Items[HoveredIndex];
        Items[HoveredIndex] = DraggedItem;
        Items[OriginalSlotIndex] = TempItem;
        UpdateSlotUI(OriginalSlotIndex);
        UpdateSlotUI(HoveredIndex);
        UE_LOG(LogTemp, Log, TEXT("MoveItem: Swapped item %d with item in slot %d"), DraggedItem.Index, HoveredIndex);
    }
    else
    {
        Items[HoveredIndex] = DraggedItem;
        Items[OriginalSlotIndex] = FItem();
        UpdateSlotUI(OriginalSlotIndex);
        UpdateSlotUI(HoveredIndex);
        UE_LOG(LogTemp, Log, TEXT("MoveItem: Moved item %d to empty slot %d"), DraggedItem.Index, HoveredIndex);
    }

    DraggedItemIndex = HoveredIndex;
    OriginalSlotIndex = HoveredIndex;

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
    if (!Items.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateSlotUI: Invalid SlotIndex=%d"), SlotIndex);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UpdateSlotUI: Updating slot %d, HasItem=%s"),
        SlotIndex, Items[SlotIndex].ReferencedActorClass ? TEXT("True") : TEXT("False"));

    if (Items[SlotIndex].ReferencedActorClass)
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
        UE_LOG(LogTemp, Log, TEXT("UpdateSlotUI: Slot %d border updated"), SlotIndex);
    }
}

void UInventory::CreateItemIcon(uint64 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !ForegroundBorders.IsValidIndex(SlotIndex)) return;

    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox) return;

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
    bCounterTextUpdated.Empty();

    Items.SetNum(MaxRows * MaxColumns);
    ForegroundBorders.SetNum(MaxRows * MaxColumns);
    IconSlots.SetNum(MaxRows * MaxColumns);
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
            IconSlots[Index] = SizeBox;
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
    ScheduledFromIndex = FromIndex;
    ScheduledToIndex = ToIndex;
    ScheduledDirection = Direction;
    bAnimationScheduled = true;
}

void UInventory::StartSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction)
{
    // Placeholder for slide animation logic
}

FVector2D UInventory::GetSlotPosition(uint64 SlotIndex) const
{
    if (ForegroundBorders.IsValidIndex(SlotIndex) && ForegroundBorders[SlotIndex])
    {
        FGeometry SlotGeometry = ForegroundBorders[SlotIndex]->GetCachedGeometry();
        return SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
    }
    return FVector2D::ZeroVector;
}

float UInventory::CustomEaseInOut(float T)
{
    return T < 0.5f ? 2.0f * T * T : -1.0f + (4.0f - 2.0f * T) * T;
}

bool UInventory::IsEdgeSlot(int32 SlotIndex) const
{
    if (!Items.IsValidIndex(SlotIndex)) return false;
    int32 Row = SlotIndex / MaxColumns;
    int32 Col = SlotIndex % MaxColumns;
    return Row == 0 || Row == MaxRows - 1 || Col == 0 || Col == MaxColumns - 1;
}
