#include "Inventory.h"

uint64 UInventory::ItemCounter = 0;

UInventory::UInventory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer),
    DraggedItemIndex(INDEX_NONE),
    DraggedOverlay(nullptr)
{
}

void UInventory::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    bIsInventoryFull = false;
    DraggedItemIndex = INDEX_NONE;
    DraggedOverlay = nullptr;

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

FReply UInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
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

FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (DraggedItemIndex != INDEX_NONE)
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
    Items[EmptySlotIndex].Index = ItemCounter; // Use the current ItemCounter value

    UE_LOG(LogTemp, Log, TEXT("AddItem: Added item to slot %d with Index %d, ActorClass %s"),
        EmptySlotIndex, Items[EmptySlotIndex].Index,
        *Items[EmptySlotIndex].ReferencedActorClass->GetName());

    UpdateSlotUI(EmptySlotIndex);
    ItemActor->Destroy();

    // Increment ItemCounter and wrap around at 13 (so indices are 0 to 12)
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
    BackgroundBorder->ForceLayoutPrepass();
    BackgroundBorder->InvalidateLayoutAndVolatility();
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

    for (uint64 Row = 0; Row < MaxRows; Row++)
    {
        for (uint64 Col = 0; Col < MaxColumns; Col++)
        {
            uint64 Index = Row * MaxColumns + Col;
            if (!ForegroundBorders.IsValidIndex(Index) || !ForegroundBorders[Index])
            {
                continue;
            }

            FGeometry SlotGeometry = ForegroundBorders[Index]->GetCachedGeometry();
            FVector2D SlotAbsTopLeft = SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
            FVector2D SlotAbsSize = SlotGeometry.GetLocalSize();
            FVector2D SlotAbsBottomRight = SlotAbsTopLeft + SlotAbsSize;

            const float SlotTolerance = 10.0f;
            if (MousePos.X >= SlotAbsTopLeft.X - SlotTolerance && MousePos.X <= SlotAbsBottomRight.X + SlotTolerance &&
                MousePos.Y >= SlotAbsTopLeft.Y - SlotTolerance && MousePos.Y <= SlotAbsBottomRight.Y + SlotTolerance)
            {
                UE_LOG(LogTemp, Log, TEXT("Picked Slot[%d] at MousePos: X=%f, Y=%f"), Index, MousePos.X, MousePos.Y);
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

    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("USizeBox not found for slot %d in CreateItemIcon"), SlotIndex);
        return;
    }

    TObjectPtr<UOverlay> IconOverlay = Cast<UOverlay>(SizeBox->GetContent());
    if (!IconOverlay)
    {
        IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
        if (!IconOverlay)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create UOverlay for slot %d"), SlotIndex);
            return;
        }
        SizeBox->SetContent(IconOverlay);
    }

    UImage* ItemIcon = nullptr;
    for (int32 i = 0; i < IconOverlay->GetChildrenCount(); ++i)
    {
        if (UImage* Image = Cast<UImage>(IconOverlay->GetChildAt(i)))
        {
            ItemIcon = Image;
            break;
        }
    }

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

        TObjectPtr<UOverlaySlot> TextOverlaySlot = IconOverlay->AddChildToOverlay(ItemCounterText);
        if (TextOverlaySlot)
        {
            TextOverlaySlot->SetHorizontalAlignment(HAlign_Center); // Center horizontally
            TextOverlaySlot->SetVerticalAlignment(VAlign_Center);   // Center vertically
            TextOverlaySlot->SetPadding(FMargin(0, 0, 0, 0));      // Remove padding to ensure true centering
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

TObjectPtr<UOverlay> UInventory::FindDraggedOverlay(uint64 ItemIndex)
{
    if (!Canvas || ItemIndex != DraggedItemIndex)
    {
        return nullptr;
    }
    return DraggedOverlay;
}

void UInventory::MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished)
{
    FGeometry CanvasGeometry = Canvas->GetCachedGeometry();

    if (bItemMovementStarted && DraggedItemIndex == INDEX_NONE)
    {
        uint64 HoveredIndex = FindHoveredItemIndex(MouseEvent);
        if (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex) && Items[HoveredIndex].ReferencedActorClass)
        {
            DraggedItemIndex = HoveredIndex;
            DraggedItem = Items[HoveredIndex];
            RemoveItemIcon(HoveredIndex);

            TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[HoveredIndex]->GetContent());
            if (SizeBox)
            {
                DraggedOverlay = Cast<UOverlay>(SizeBox->GetContent());
                if (DraggedOverlay)
                {
                    SizeBox->SetContent(nullptr);
                    Canvas->AddChild(DraggedOverlay);

                    UCanvasPanelSlot* DraggedSlot = Cast<UCanvasPanelSlot>(DraggedOverlay->Slot);
                    if (DraggedSlot)
                    {
                        DraggedSlot->SetSize(FVector2D(100.0f, 100.0f));
                        DraggedSlot->SetZOrder(100);

                        FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
                        FVector2D LocalPos = CanvasGeometry.AbsoluteToLocal(ScreenPos) - FVector2D(1.0f, 1.0f);
                        FVector2D WidgetSize = DraggedOverlay->GetDesiredSize();
                        FVector2D CenterOffset = FVector2D(WidgetSize.X * 0.4f, WidgetSize.Y * 0.3f);
                        DraggedSlot->SetPosition(LocalPos - CenterOffset);
                    }
                }
            }
            UE_LOG(LogTemp, Log, TEXT("Started dragging item from slot %d"), HoveredIndex);
        }
    }
    else if (!bItemMovementStarted && !bItemMovementFinished && DraggedItemIndex != INDEX_NONE)
    {
        if (DraggedOverlay)
        {
            FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
            FVector2D LocalPos = CanvasGeometry.AbsoluteToLocal(ScreenPos);
            UCanvasPanelSlot* DraggedSlot = Cast<UCanvasPanelSlot>(DraggedOverlay->Slot);
            if (DraggedSlot)
            {
                FVector2D WidgetSize = DraggedOverlay->GetDesiredSize();
                FVector2D CenterOffset = FVector2D(WidgetSize.X * 0.4f, WidgetSize.Y * 0.3f);
                DraggedSlot->SetPosition(LocalPos - CenterOffset);
            }
        }

        uint64 HoveredIndex = FindHoveredItemIndex(MouseEvent);
        if (HoveredIndex != INDEX_NONE && HoveredIndex != DraggedItemIndex)
        {
            ShiftItems(DraggedItemIndex, HoveredIndex);
            DraggedItemIndex = HoveredIndex;
        }
    }
    else if (bItemMovementFinished && DraggedItemIndex != INDEX_NONE)
    {
        uint64 TargetIndex = FindHoveredItemIndex(MouseEvent);
        if (TargetIndex != INDEX_NONE && DraggedOverlay)
        {
            Canvas->RemoveChild(DraggedOverlay);
            TObjectPtr<USizeBox> TargetSizeBox = Cast<USizeBox>(ForegroundBorders[TargetIndex]->GetContent());
            if (TargetSizeBox)
            {
                TargetSizeBox->SetContent(DraggedOverlay);
                Items[TargetIndex] = DraggedItem;
                UpdateSlotUI(TargetIndex);
            }
        }
        else
        {
            if (DraggedOverlay)
            {
                Canvas->RemoveChild(DraggedOverlay);
                TObjectPtr<USizeBox> OriginalSizeBox = Cast<USizeBox>(ForegroundBorders[DraggedItemIndex]->GetContent());
                if (OriginalSizeBox)
                {
                    OriginalSizeBox->SetContent(DraggedOverlay);
                    Items[DraggedItemIndex] = DraggedItem;
                    UpdateSlotUI(DraggedItemIndex);
                }
            }
        }
        DraggedItemIndex = INDEX_NONE;
        DraggedItem = FItem();
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
    }

    return Direction;
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

void UInventory::UpdateSlideTick()
{
    // Placeholder for animation logic if needed
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

void UInventory::ShiftItems(uint64 FromIndex, uint64 ToIndex)
{
    if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex) || FromIndex == ToIndex)
    {
        return;
    }

    int32 RowFrom = FromIndex / MaxColumns;
    int32 ColFrom = FromIndex % MaxColumns;
    int32 RowTo = ToIndex / MaxColumns;
    int32 ColTo = ToIndex % MaxColumns;

    if (RowFrom != RowTo)
    {
        Items.Swap(FromIndex, ToIndex);
        UpdateSlotUI(FromIndex);
        UpdateSlotUI(ToIndex);
        return;
    }

    FItem TempItem = Items[FromIndex];
    if (ColFrom < ColTo)
    {
        for (uint64 i = FromIndex; i < ToIndex; i++)
        {
            Items[i] = Items[i + 1];
            UpdateSlotUI(i);
        }
    }
    else
    {
        for (uint64 i = FromIndex; i > ToIndex; i--)
        {
            Items[i] = Items[i - 1];
            UpdateSlotUI(i);
        }
    }
    Items[ToIndex] = TempItem;
    UpdateSlotUI(ToIndex);
}
