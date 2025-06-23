#include "Inventory.h"

uint32 UInventory::ItemCounter = 0;

UInventory::UInventory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer),
      MaxRows(3),
      MaxColumns(4),
      bIsInventoryFull(false),
      Canvas(nullptr),
      BackgroundBorder(nullptr),
      BackgroundBorderSlot(nullptr),
      BackgroundVerticalBox(nullptr),
      Title(nullptr),
      TitleVerticalBoxSlot(nullptr),
      Grid(nullptr),
      GridVerticalBoxSlot(nullptr),
      GridSlot(nullptr),
      DraggedItemWidget(nullptr),
      DragStartSlot(INDEX_NONE),
      OriginalSlot(INDEX_NONE),
      PreviousSlotIndex(INDEX_NONE),
      DraggedItem(FItem()),
      MouseScreenSpacePosition(FVector2D::ZeroVector),
      MouseWidgetLocalPosition(FVector2D::ZeroVector),
      DragState(EDragState::Null)
{
    // Set menber array's size to 12 (3x4)
    Items.SetNum(MaxRows * MaxColumns);
    ForegroundBorders.SetNum(MaxRows * MaxColumns);

    // Resetting item index back to zero when the player starts to play
    ItemCounter = 0;
    for (const FItem& Item : Items)
    {
        if (Item.WorldObjectReference)
            ItemCounter = FMath::Max(ItemCounter, static_cast<uint32>(Item.Index + 1));
    }
}

void UInventory::NativeOnInitialized()
{
    // Initialize widget memnbers and layout before inventory construction 

    Super::NativeOnInitialized();

    if (!WidgetTree)
    {
        #if WITH_EDITOR
             UE_LOG(LogTemp, Error, TEXT("WidgetTree is null"));
        #else
             UE_LOG(LogTemp, Fatal, TEXT("WidgetTree is null"));
        #endif

        return;
    }

    Canvas = NewObject<UCanvasPanel>(this);

    // It's mandatory to set the first widget element as the root widget of the widget's tree
    WidgetTree->RootWidget = Canvas;

    // Create vertical Box to hold background border and create a gray backround boder
    BackgroundVerticalBox = NewObject<UVerticalBox>(this);
    BackgroundBorder = NewObject<UBorder>(this);
    BackgroundBorder->SetBrushColor(FLinearColor::Gray);
    BackgroundBorder->SetPadding(FMargin(7.5f, 0.0f, 7.5f, 0.0f));

    // Name the inventory "Inventory"
    Title = NewObject<UTextBlock>(this);
    Title->SetText(FText::FromString(TEXT("Inventory")));

    // Need a vertical bocx slot for title middle adjustment
    TitleVerticalBoxSlot = BackgroundVerticalBox->AddChildToVerticalBox(Title);
    TitleVerticalBoxSlot->SetHorizontalAlignment(HAlign_Center);
    TitleVerticalBoxSlot->SetVerticalAlignment(VAlign_Top);
    TitleVerticalBoxSlot->SetPadding(FMargin(10, 10, 10, 10));

    // Create a grid for the inevntory and readjust grid alignment within baackground's vertical box 
    Grid = NewObject<UUniformGridPanel>(this);
    Grid->SetSlotPadding(FMargin(7.0f, 7.0f, 7.0f, 7.0f));
    GridVerticalBoxSlot = BackgroundVerticalBox->AddChildToVerticalBox(Grid);
    GridVerticalBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    GridVerticalBoxSlot->SetVerticalAlignment(VAlign_Fill);

    // Setting BackgourndBorder content to what is inside the BackgroundBorder's VerticalBox and position/anchoring
    BackgroundBorder->SetContent(BackgroundVerticalBox);
    BackgroundBorderSlot = Canvas->AddChildToCanvas(BackgroundBorder);
    BackgroundBorderSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
    BackgroundBorderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    BackgroundBorderSlot->SetOffsets(FMargin(-10.0f, 11.0f, 485.0f, 419.0f));

    // Call create method to colonize the inventory with slots
    Create();
}

void UInventory::NativeConstruct()
{
    Super::NativeConstruct();

    // Refresh inventory before it gets added to viewport
    RefreshInventory();
}

FReply UInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    {
        int32 HoveredSlot = FindHoveredSlot(InMouseEvent);

        if (HoveredSlot != INDEX_NONE && Items.IsValidIndex(HoveredSlot))
        {
            // ✅ Check if the item is actually valid (e.g., has a reference to an object or class)
            if (Items[HoveredSlot].WorldObjectReference)
            {
                DragStartSlot = HoveredSlot;

                OriginalSlot = HoveredSlot;

                DraggedItem = Items[HoveredSlot];

                DragState = EDragState::Select;

                // **clear the original slot immediately**—
                // prevents any “left behind” copies
                Items[OriginalSlot] = FItem{};

                TSharedPtr<SWidget> RootSlate = GetCachedWidget();
                if (!RootSlate.IsValid())
                {
                    #if	WITH_EDITOR
                        UE_LOG(LogTemp, Error, TEXT("Couldn't get cached root slate widget!"));
                    #else
                        UE_LOG(LogTemp, Fatal, TEXT("Couldn't get cached root slate widget!"));
                    #endif

                    return FReply::Unhandled();
                }

                return FReply::Handled().CaptureMouse(RootSlate.ToSharedRef());
            }
        }
    }
    return FReply::Unhandled();
}

FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // DragState neither Select nor Moved just exit fucntion
    if (DragState != EDragState::Select && DragState != EDragState::Moved)
        return Super::NativeOnMouseMove(InGeometry, InMouseEvent);

    Super::NativeOnMouseMove(InGeometry, InMouseEvent);

    MouseScreenSpacePosition = InMouseEvent.GetScreenSpacePosition();
    MouseWidgetLocalPosition = InGeometry.AbsoluteToLocal(MouseScreenSpacePosition);

    // — If already popped out, just update the ghost’s position —
    if (DragState == EDragState::Moved && DraggedItemWidget)
    {
        if (UCanvasPanelSlot* GhostSlot = Cast<UCanvasPanelSlot>(DraggedItemWidget->Slot))
        {
            GhostSlot->SetPosition(MouseWidgetLocalPosition - FVector2D(50.f, 50.f));
        }
        return FReply::Handled();
    }

    // — Still in “Select”: check boundary to pop out —
    RefreshInventory();

    if (UBorder* Border = ForegroundBorders[DragStartSlot].Get())
    {
        const FGeometry SlotGeomeometry = Border->GetCachedGeometry();

        const FVector2D SlotTopLeft = SlotGeomeometry.LocalToAbsolute(FVector2D::ZeroVector);

        const FVector2D SlotBottumRight = SlotTopLeft + SlotGeomeometry.GetLocalSize();

        const int32 Row = DragStartSlot / MaxColumns;
        const int32 Column = DragStartSlot % MaxColumns;

        bool bIsMouseOutsideSlot =
            (Row == 0 && MouseScreenSpacePosition.Y < SlotTopLeft.Y) ||
            (Row == MaxRows - 1 && MouseScreenSpacePosition.Y > SlotBottumRight.Y) ||
            (Column == 0 && MouseScreenSpacePosition.X < SlotTopLeft.X) ||
            (Column == MaxColumns - 1 && MouseScreenSpacePosition.X > SlotBottumRight.X);

        if (bIsMouseOutsideSlot)
        {
            // 1) Clear the slot’s SizeBox to leave it visually empty
            if (USizeBox* Box = Cast<USizeBox>(Border->GetContent()))
            {
                Box->ClearChildren();
            }

            // 2) Mark popped-out
            DragState = EDragState::Moved;

            // 3) Spawn the ghost widget at cursor
            if (Canvas)
            {
                DraggedItemWidget = NewObject<UOverlay>(this);
                DraggedItemWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

                // (a) Icon
                UImage* Image = NewObject<UImage>(this);
                Image->SetVisibility(ESlateVisibility::Visible);
                Image->SetColorAndOpacity(FLinearColor::Blue);

                UOverlaySlot* ImageSlot = DraggedItemWidget->AddChildToOverlay(Image);
                ImageSlot->SetHorizontalAlignment(HAlign_Fill);
                ImageSlot->SetVerticalAlignment(VAlign_Fill);

                // (b) Counter
                UTextBlock* Text = NewObject<UTextBlock>(this);
                Text->SetVisibility(ESlateVisibility::Visible);
                Text->SetText(FText::AsNumber(DraggedItem.Index));
                Text->SetColorAndOpacity(FLinearColor::Red);
                Text->SetJustification(ETextJustify::Center);
                Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 20));

                UOverlaySlot* TextSlot = DraggedItemWidget->AddChildToOverlay(Text);
                TextSlot->SetHorizontalAlignment(HAlign_Center);
                TextSlot->SetVerticalAlignment(VAlign_Center);

                if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(DraggedItemWidget))
                {
                    CanvasSlot->SetSize(FVector2D(100.f, 100.f));
                    CanvasSlot->SetPosition(MouseScreenSpacePosition - FVector2D(50.f, 50.f));
                    CanvasSlot->SetZOrder(100);
                }
            }
        }
    }

    // Always run your interior MoveItem logic
    MoveItem(InMouseEvent);
    return FReply::Handled();
}

FReply UInventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 0) Only handle if we’re dragging
    if (DragState != EDragState::Select && DragState != EDragState::Moved)
        return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

    // 1) Tear down ghost and refresh
    RefreshInventory();
    if (DraggedItemWidget)
    {
        Canvas->RemoveChild(DraggedItemWidget);
        DraggedItemWidget = nullptr;
    }

    // 2) Mouse position and hovered slot
    MouseScreenSpacePosition = InMouseEvent.GetScreenSpacePosition();
    int32 HoveredSlot = FindHoveredSlot(InMouseEvent);

    // 3) Inventory background hit test
    bool bOverInventoryBG = false;
    if (BackgroundBorder)
    {
        const FGeometry BGGeom = BackgroundBorder->GetCachedGeometry();
        bOverInventoryBG = BGGeom.IsUnderLocation(MouseScreenSpacePosition);
    }

    // 4) Decide what to do
    if (HoveredSlot != INDEX_NONE && Items.IsValidIndex(HoveredSlot))
    {
        // — Dropped on a slot —
        if (!Items[HoveredSlot].WorldObjectReference)
        {
            // A) empty → move
            Items[HoveredSlot] = DraggedItem;
        }
        else if (HoveredSlot == OriginalSlot)
        {
            // B) same slot → cancel
            Items[OriginalSlot] = DraggedItem;
        }
        else
        {
            // C) occupied → swap
            FItem Temp = Items[HoveredSlot];
            Items[HoveredSlot] = DraggedItem;
            Items[OriginalSlot] = Temp;
        }

        // Clear original if we truly moved
        if (HoveredSlot != OriginalSlot)
            Items[OriginalSlot] = FItem{};
    }
    else if (bOverInventoryBG)
    {
        // — Dropped on inventory background (but *not* on a slot) → cancel
        if (OriginalSlot != INDEX_NONE && Items.IsValidIndex(OriginalSlot))
            Items[OriginalSlot] = DraggedItem;
    }
    else
    {
        // — Dropped off-grid → spawn in world & clear original —
        if (UWorld* W = GetWorld())
        {
            FActorSpawnParameters P;
            P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            if (auto* A = W->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(),
                DraggedItem.WorldObjectTransform,
                P))
            {
                auto* MeshComp = A->GetStaticMeshComponent();
                MeshComp->SetMobility(EComponentMobility::Movable);
                if (auto* M = DraggedItem.StaticMesh.LoadSynchronous())
                    MeshComp->SetStaticMesh(M);
                for (int32 i = 0; i < DraggedItem.StoredMaterials.Num(); ++i)
                    if (DraggedItem.StoredMaterials[i].IsValid())
                        MeshComp->SetMaterial(i, DraggedItem.StoredMaterials[i].LoadSynchronous());
            }
        }
        if (OriginalSlot != INDEX_NONE && Items.IsValidIndex(OriginalSlot))
            Items[OriginalSlot] = FItem{};
    }

    // 5) Reset drag
    DraggedItem = FItem{};
    OriginalSlot = INDEX_NONE;
    DragState = EDragState::Released;

    // 6) Refresh visuals
    RefreshInventory();

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
    
    // Build a set of all used item-indices
    TSet<uint32> UsedIndices;
    for (const FItem& It : Items)
    {
        if (It.WorldObjectReference)
        {
            UsedIndices.Add(It.Index);
        }
    }

    // Find the smallest non-negative integer not in UsedIndices
    uint32 FreeIndex = 0;
    while (UsedIndices.Contains(FreeIndex))
    {
        ++FreeIndex;
    }

    FItem& NewItem = Items[EmptySlotIndex];
    NewItem = FItem(); // Clear defaults

    NewItem.WorldObjectReference = ItemActor->GetClass();
    NewItem.WorldObjectTransform = ItemActor->GetActorTransform();

    // Use global counter as fixed, ever-increasing index
    NewItem.Index = FreeIndex;

    if (UStaticMeshComponent* MeshComp = ItemActor->FindComponentByClass<UStaticMeshComponent>())
    {
        if (MeshComp->GetStaticMesh())
        {
            NewItem.StaticMesh = MeshComp->GetStaticMesh();
        }

        for (int32 i = 0; i < MeshComp->GetNumMaterials(); ++i)
        {
            UMaterialInterface* Mat = MeshComp->GetMaterial(i);
            if (IsValid(Mat))
            {
                NewItem.StoredMaterials.Add(Mat);
            }
        }
    }

    RefreshInventory();

    ItemActor->Destroy();
    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);
}

void UInventory::RemoveItem(int32 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].WorldObjectReference)
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveItem: Invalid slot %d or no item to remove"), SlotIndex);
        return;
    }

    Items[SlotIndex] = FItem();
    RefreshInventory();   // <-- this ensures the slot is cleared visually

    if (ItemCounter > 0) ItemCounter--;

    ItemCounter = 0;
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i].WorldObjectReference)
        {
            Items[i].Index = ItemCounter++;
            CreateItemIcon(i); // Rebuild UI, but don’t change the index
        }
    }
    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);
}

int32 UInventory::FindHoveredSlot(const FPointerEvent& InMouseEvent)
{
    if (!Grid || ForegroundBorders.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("FindHoveredSlot: Grid or ForegroundBorders invalid"));
        return INDEX_NONE;
    }

    RefreshInventory();

    const FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
    int32 ClosestIndex = INDEX_NONE;
    float MinDistance = FLT_MAX;
    bool bAnyValidGeometry = false;

    for (int32 Row = 0; Row < int32(MaxRows); ++Row)
    {
        for (int32 Col = 0; Col < int32(MaxColumns); ++Col)
        {
            const int32 Index = Row * MaxColumns + Col;

            if (!ForegroundBorders.IsValidIndex(Index) || !ForegroundBorders[Index])
            {
                UE_LOG(LogTemp, Warning, TEXT("FindHoveredSlot: Slot %d invalid or null"), Index);
                continue;
            }

            const FGeometry SlotGeometry = ForegroundBorders[Index]->GetCachedGeometry();
            const FVector2D SlotAbsTopLeft = SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
            const FVector2D SlotAbsSize = SlotGeometry.GetLocalSize();
            const FVector2D SlotAbsBottomRight = SlotAbsTopLeft + SlotAbsSize;

            bAnyValidGeometry = true;

            if (MousePos.X >= SlotAbsTopLeft.X && MousePos.X <= SlotAbsBottomRight.X &&
                MousePos.Y >= SlotAbsTopLeft.Y && MousePos.Y <= SlotAbsBottomRight.Y)
            {
                const FVector2D SlotCenter = SlotAbsTopLeft + (SlotAbsSize * 0.5f);
                const float Distance = FVector2D::Distance(MousePos, SlotCenter);

                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    ClosestIndex = Index;
                    UE_LOG(LogTemp, Log, TEXT("FindHoveredSlot: Slot %d hit, Distance=%f"), Index, Distance);
                }
            }
        }
    }

    if (ClosestIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT(
            "FindHoveredSlot: No valid slot hit. AnyValidGeometry=%s, MousePos=%s"),
            bAnyValidGeometry ? TEXT("True") : TEXT("False"),
            *MousePos.ToString());
    }

    return ClosestIndex;
}

void UInventory::RefreshInventory()
{
    // 1) Force layout so geometries update
    if (Grid)   Grid->ForceLayoutPrepass();
    if (Canvas) Canvas->ForceLayoutPrepass();

    const EDragState CurrentDragState = DragState;
    const int32        GhostSlot = OriginalSlot;

    // 2) Loop through all slots
    for (uint32 SlotIndex = 0; SlotIndex < (uint32)Items.Num(); ++SlotIndex)
    {
        if (!Items.IsValidIndex(SlotIndex) ||
            !ForegroundBorders.IsValidIndex(SlotIndex))
        {
            continue;
        }

        UBorder* SlotBorder = ForegroundBorders[SlotIndex].Get();
        if (!SlotBorder) continue;

        USizeBox* SizeBox = Cast<USizeBox>(SlotBorder->GetContent());
        if (!SizeBox) continue;

        // Clear old visuals
        SizeBox->ClearChildren();

        // If ghosting this slot, leave blank
        if (CurrentDragState == EDragState::Moved &&
            int32(SlotIndex) == GhostSlot)
        {
            continue;
        }

        // Draw icon if item present
        const FItem& Item = Items[SlotIndex];
        if (Item.WorldObjectReference)
        {
            CreateItemIcon(SlotIndex);
        }

        // Make sure border is visible & laid out
        SlotBorder->SetVisibility(ESlateVisibility::Visible);
        SlotBorder->ForceLayoutPrepass();
    }
}

void UInventory::MoveItem(const FPointerEvent& MouseEvent)
{
    if (DragState != EDragState::Moved && DragState != EDragState::Select)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveItem: Not dragging, exiting"));
        return;
    }

    uint64 HoveredSlot = FindHoveredSlot(MouseEvent);
    UE_LOG(LogTemp, Log, TEXT("MoveItem: HoveredIndex=%d, OriginalSlot=%d, DragStartSlot=%d"),
        HoveredSlot, OriginalSlot, DragStartSlot);

    if (HoveredSlot == INDEX_NONE || !Items.IsValidIndex(HoveredSlot))
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveItem: Invalid HoveredIndex=%d or out of bounds"), HoveredSlot);
        return;
    }

    PreviousSlotIndex = HoveredSlot;

    if (HoveredSlot == OriginalSlot)
    {
        UE_LOG(LogTemp, Log, TEXT("MoveItem: HoveredIndex same as OriginalSlotIndex, skipping"));
        return;
    }

    uint64 FromRow = OriginalSlot / MaxColumns;
    uint64 FromCol = OriginalSlot % MaxColumns;

    uint64 ToRow = HoveredSlot / MaxColumns;
    uint64 ToCol = HoveredSlot % MaxColumns;

    EDirection Direction = GetMoveDirection(FromRow, FromCol, ToRow, ToCol);
    UE_LOG(LogTemp, Log, TEXT("MoveItem: Moving from (%d,%d) to (%d,%d), Direction=%d"),
        FromRow, FromCol, ToRow, ToCol, (uint8)Direction);

    if (Items[HoveredSlot].WorldObjectReference)
    {
        FItem TempItem = Items[HoveredSlot];
        Items[HoveredSlot] = DraggedItem;
        Items[OriginalSlot] = TempItem;
        RefreshInventory();
        RefreshInventory();
        UE_LOG(LogTemp, Log, TEXT("MoveItem: Swapped item %d with item in slot %d"), DraggedItem.Index, HoveredSlot);
    }
    else
    {
        Items[HoveredSlot] = DraggedItem;
        Items[OriginalSlot] = FItem();
        RefreshInventory();
        RefreshInventory();
        UE_LOG(LogTemp, Log, TEXT("MoveItem: Moved item %d to empty slot %d"), DraggedItem.Index, HoveredSlot);
    }

    DragStartSlot = HoveredSlot;
    OriginalSlot = HoveredSlot;

    if (Grid) Grid->ForceLayoutPrepass();
    if (Canvas) Canvas->ForceLayoutPrepass();
}

EDirection UInventory::GetMoveDirection(uint32 RowA, uint32 ColA, uint32 RowB, uint32 ColB) const
{
    if (RowA == RowB && ColA < ColB) return EDirection::Right;
    if (RowA == RowB && ColA > ColB) return EDirection::Left;
    if (ColA == ColB && RowA < RowB) return EDirection::Down;
    if (ColA == ColB && RowA > RowB) return EDirection::Up;
    return EDirection::Null;
}



void UInventory::CreateItemIcon(uint32 SlotIndex)
{
    // Validate indices
    if (!Items.IsValidIndex(SlotIndex) || !ForegroundBorders.IsValidIndex(SlotIndex))
    {
        return;
    }

    // Get the SizeBox from the border
    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox)
    {
        return;
    }

    // Ensure we have a single overlay container
    UOverlay* IconOverlay = Cast<UOverlay>(SizeBox->GetContent());
    if (!IconOverlay)
    {
        IconOverlay = NewObject<UOverlay>(this);
        IconOverlay->SetVisibility(ESlateVisibility::Visible);
        SizeBox->SetContent(IconOverlay);
    }
    else
    {
        // Clear out any old icon/text
        IconOverlay->ClearChildren();
    }

    // --- 1) Create and configure the item icon ---

    UImage* ItemIcon = NewObject<UImage>(this);
    ItemIcon->SetVisibility(ESlateVisibility::Visible);

    // Fill the overlay slot
    if (UOverlaySlot* ImageSlot = IconOverlay->AddChildToOverlay(ItemIcon))
    {
        ImageSlot->SetHorizontalAlignment(HAlign_Fill);
        ImageSlot->SetVerticalAlignment(VAlign_Fill);
    }

        ItemIcon->SetColorAndOpacity(FLinearColor::Blue);

    // --- 2) If the item has a WorldObjectReference, add the counter ---

    if (Items[SlotIndex].WorldObjectReference)
    {
        UTextBlock* CounterText = NewObject<UTextBlock>(this);
        CounterText->SetVisibility(ESlateVisibility::Visible);

        if (UOverlaySlot* TextSlot = IconOverlay->AddChildToOverlay(CounterText))
        {
            TextSlot->SetHorizontalAlignment(HAlign_Center);
            TextSlot->SetVerticalAlignment(VAlign_Center);  // or VAlign_Center if preferred
        }

        CounterText->SetText(FText::AsNumber(Items[SlotIndex].Index));
        CounterText->SetColorAndOpacity(FLinearColor::Red);
        CounterText->SetJustification(ETextJustify::Center);
        CounterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
    }
}

int32 UInventory::FindFirstEmptySlot() const
{
    for (uint64 i = 0; i < static_cast<uint64>(Items.Num()); i++)
    {
        if (!Items[i].WorldObjectReference) return i;
    }
    return INDEX_NONE;
}

void UInventory::RemoveItemIcon(uint32 SlotIndex)
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
    Items.Empty();

    Items.SetNum(MaxRows * MaxColumns);
    ForegroundBorders.SetNum(MaxRows * MaxColumns);

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

const TArray<FItem>& UInventory::GetItems() const
{
    return Items;
}

TArray<TObjectPtr<UBorder>> UInventory::GetForegroundBorders() const
{
    return ForegroundBorders;
}

TObjectPtr<UUniformGridPanel> UInventory::GetGrid() const
{
    return Grid.Get();
}

FVector2D UInventory::GetSlotPosition(uint32 SlotIndex) const
{
    if (ForegroundBorders.IsValidIndex(SlotIndex) && ForegroundBorders[SlotIndex])
    {
        FGeometry SlotGeometry = ForegroundBorders[SlotIndex]->GetCachedGeometry();
        return SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
    }
    return FVector2D::ZeroVector;
}
