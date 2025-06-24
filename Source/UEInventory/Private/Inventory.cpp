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
      HoveredSlotIndex(INDEX_NONE),
      OriginSlotIndex(INDEX_NONE),
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
        HoveredSlotIndex = FindHoveredSlot(InMouseEvent);

        if (HoveredSlotIndex != INDEX_NONE && Items.IsValidIndex(HoveredSlotIndex))
        {
            // ✅ Check if the item is actually valid (e.g., has a reference to an object or class)
            if (Items[HoveredSlotIndex].WorldObjectReference)
            {
                OriginSlotIndex = HoveredSlotIndex;

                DraggedItem = Items[HoveredSlotIndex];

                DragState = EDragState::Select;

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
        if (UCanvasPanelSlot* DraggedItemWidgetSlot = Cast<UCanvasPanelSlot>(DraggedItemWidget->Slot))
        {
            DraggedItemWidgetSlot->SetPosition(MouseWidgetLocalPosition - FVector2D(50.f, 50.f));
        }
        return FReply::Handled();
    }

    // 2) Still in “Select” state — check if we’ve exited the original slot’s bounds
    if (OriginSlotIndex != INDEX_NONE && ForegroundBorders.IsValidIndex(OriginSlotIndex))
    {
        UBorder* OriginBorder = ForegroundBorders[OriginSlotIndex].Get();
        if (OriginBorder)
        {
            const FGeometry SlotGeomeometry = OriginBorder->GetCachedGeometry();

            const FVector2D SlotTopLeft = SlotGeomeometry.LocalToAbsolute(FVector2D::ZeroVector);

            const FVector2D SlotBottumRight = SlotTopLeft + SlotGeomeometry.GetLocalSize();

            const int32 Row = HoveredSlotIndex / MaxColumns;

            const int32 Column = HoveredSlotIndex % MaxColumns;

            bool bIsMouseOutsideSlot = (Row == 0 && MouseScreenSpacePosition.Y < SlotTopLeft.Y) ||
                (Row == MaxRows - 1 && MouseScreenSpacePosition.Y > SlotBottumRight.Y) ||
                (Column == 0 && MouseScreenSpacePosition.X < SlotTopLeft.X) ||
                (Column == MaxColumns - 1 && MouseScreenSpacePosition.X > SlotBottumRight.X);

            if (bIsMouseOutsideSlot)
            {
                // 1) Clear the slot’s SizeBox to leave it visually empty
                if (USizeBox* Box = Cast<USizeBox>(OriginBorder->GetContent()))
                    Box->ClearChildren();

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
    }
    // Always run your interior MoveItem logic
    UpdateInteriorDrag(InMouseEvent);

    return FReply::Handled();
}

FReply UInventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 0) Only handle if we’re dragging
    if (DragState != EDragState::Select && DragState != EDragState::Moved)
        return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

    if (DraggedItemWidget)
    {
        Canvas->RemoveChild(DraggedItemWidget);
        DraggedItemWidget = nullptr;
    }

    // 2) Mouse position and hovered slot
    MouseScreenSpacePosition = InMouseEvent.GetScreenSpacePosition();

    HoveredSlotIndex = FindHoveredSlot(InMouseEvent);

    // 3) Inventory background hit test
    bool bisMouseOutsideInventoryBounds = false;

    if (BackgroundBorder)
    {
        const FGeometry BackgroundBorderGeometry = BackgroundBorder->GetCachedGeometry();

        bisMouseOutsideInventoryBounds = BackgroundBorderGeometry.IsUnderLocation(MouseScreenSpacePosition);
    }

    // 4) Decide what to do
    if (HoveredSlotIndex != INDEX_NONE && Items.IsValidIndex(HoveredSlotIndex))
    {
        // — Dropped on a slot —
        if (!Items[HoveredSlotIndex].WorldObjectReference)
        {
            Items[HoveredSlotIndex] = DraggedItem; // A) empty → move
            Items[OriginSlotIndex] = FItem{};
        }
        else if (HoveredSlotIndex == OriginSlotIndex)
            Items[OriginSlotIndex] = DraggedItem; // B) same slot → cancel
        else
        {
            // C) occupied → swap
            Items[OriginSlotIndex] = Items[HoveredSlotIndex];
            Items[HoveredSlotIndex] = DraggedItem;
        }
    }
    else if (bisMouseOutsideInventoryBounds)
    {
        // — Dropped on inventory background (but *not* on a slot) → cancel
        if (OriginSlotIndex != INDEX_NONE && Items.IsValidIndex(OriginSlotIndex))
            Items[OriginSlotIndex] = DraggedItem;
    }
    else
    {
        // — Dropped off-grid → spawn in world & clear original —
        UWorld* World = GetWorld();

        if (World)
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn; 

            AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), DraggedItem.WorldObjectTransform, SpawnParameters);
            
            if (MeshActor)
            {
                UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent();

                MeshComponent->SetMobility(EComponentMobility::Movable);
                
                if (UStaticMesh* Mesh = DraggedItem.StaticMesh.LoadSynchronous())
                    MeshComponent->SetStaticMesh(Mesh);

                for (int32 index = 0; index < DraggedItem.StoredMaterials.Num(); ++index)
                {
                    if (DraggedItem.StoredMaterials[index].IsValid())
                        MeshComponent->SetMaterial(index, DraggedItem.StoredMaterials[index].LoadSynchronous());
                }
            }
        }

        if (OriginSlotIndex != INDEX_NONE && Items.IsValidIndex(OriginSlotIndex))
            Items[OriginSlotIndex] = FItem{};
    }

    // 5) Reset drag
    DraggedItem = FItem{};
    OriginSlotIndex = INDEX_NONE;
    DragState = EDragState::Released;

    // 6) Refresh visuals
    RefreshInventory();

    return FReply::Handled().ReleaseMouseCapture();
}

void UInventory::AddItem(AActor* ItemActor)
{
    if (!ItemActor) return;

    if (ItemCounter >= MaxRows * MaxColumns)
    {
        bIsInventoryFull = true;
        return;
    }

    uint64 EmptySlot = FindFirstEmptySlot();
    if (EmptySlot == INDEX_NONE)
    {
        bIsInventoryFull = true;
        return;
    }
    
    // Build a set of all used item-indices
    TSet<int32> UsedIndices;
    for (const FItem& Item : Items)
    {
        if (Item.WorldObjectReference)
        {
            UsedIndices.Add(Item.Index);
        }
    }

    // Find the smallest non-negative integer not in UsedIndices
    int32 ValidIndex = 0;
    while (UsedIndices.Contains(ValidIndex))
    {
        ++ValidIndex;
    }

    FItem& NewItem = Items[EmptySlot];

    NewItem = FItem(); // Clear defaults

    NewItem.WorldObjectReference = ItemActor->GetClass();

    NewItem.WorldObjectTransform = ItemActor->GetActorTransform();

    // Use global counter as fixed, ever-increasing index
    NewItem.Index = ValidIndex;

    if (UStaticMeshComponent* MeshComponent = ItemActor->FindComponentByClass<UStaticMeshComponent>())
    {
        if (MeshComponent->GetStaticMesh())
        {
            NewItem.StaticMesh = MeshComponent->GetStaticMesh();
        }

        for (int32 i = 0; i < MeshComponent->GetNumMaterials(); ++i)
        {
            UMaterialInterface* MaterialInterface = MeshComponent->GetMaterial(i);
            if (IsValid(MaterialInterface))
            {
                NewItem.StoredMaterials.Add(MaterialInterface);
            }
        }
    }

    RefreshInventory();

    ItemActor->Destroy();

    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);
}

int32 UInventory::FindHoveredSlot(const FPointerEvent& InMouseEvent)
{
    MouseScreenSpacePosition = InMouseEvent.GetScreenSpacePosition();

    int32 NearestSlotIndex = INDEX_NONE;

    float MinimumDistanceToSlot = FLT_MAX;

    bool bAnyValidGeometry = false;

    for (int32 Row = 0; Row < int32(MaxRows); ++Row)
    {
        for (int32 Column = 0; Column < int32(MaxColumns); ++Column)
        {
            const int32 Index = Row * MaxColumns + Column;

            if (!ForegroundBorders.IsValidIndex(Index) || !ForegroundBorders[Index])
            {
                #if	WITH_EDITOR
                     UE_LOG(LogTemp, Error, TEXT("Hovered slot index %d is invalid on FindHoveredSlot()"), Index);
                #else
                     UE_LOG(LogTemp, Fatal, TEXT("Hovered slot index %d is invalid on FindHoveredSlot()"), Index);
                #endif

                continue;
            }

            const FGeometry SlotGeometry = ForegroundBorders[Index]->GetCachedGeometry();
            const FVector2D SlotAbsoluteTopLeft = SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
            const FVector2D SlotAbsoluteSize = SlotGeometry.GetLocalSize();
            const FVector2D SlotAbsoluteBottomRight = SlotAbsoluteTopLeft + SlotAbsoluteSize;

            bAnyValidGeometry = true;

            if (MouseScreenSpacePosition.X >= SlotAbsoluteTopLeft.X && MouseScreenSpacePosition.X <= SlotAbsoluteBottomRight.X &&
                MouseScreenSpacePosition.Y >= SlotAbsoluteTopLeft.Y && MouseScreenSpacePosition.Y <= SlotAbsoluteBottomRight.Y)
            {
                const FVector2D& HalfSlotScale = SlotAbsoluteSize * 0.5f;

                const FVector2D& SlotCenter = SlotAbsoluteTopLeft + HalfSlotScale;

                const float Distance = FVector2D::Distance(MouseScreenSpacePosition, SlotCenter);

                if (Distance < MinimumDistanceToSlot)
                {
                    MinimumDistanceToSlot = Distance;

                    NearestSlotIndex = Index;

                    #if	WITH_EDITOR
                         UE_LOG(LogTemp, Log, TEXT("Hovered slot index %d has mouse hovering over at a distance of %f"), Index, Distance); 
                    #endif
                }
            }
        }
    }

    return NearestSlotIndex;
}

void UInventory::RefreshInventory()
{
    // 1) Force layout so geometries update
    if (Grid)   Grid->ForceLayoutPrepass();
    if (Canvas) Canvas->ForceLayoutPrepass();

    // 2) Loop through all slots
    for (int32 SlotIndex = 0; SlotIndex < Items.Num(); ++SlotIndex)
    {
        if (!ForegroundBorders.IsValidIndex(SlotIndex))
            continue;

        UBorder* SlotBorder = ForegroundBorders[SlotIndex].Get();
        if (!SlotBorder) continue;

        USizeBox* SizeBox = Cast<USizeBox>(SlotBorder->GetContent());
        if (!SizeBox) continue;

        // Clear old visuals
        SizeBox->ClearChildren();

        // If ghosting this slot, leave blank
        if (DragState == EDragState::Moved && SlotIndex == OriginSlotIndex)
            continue;

        // Draw icon if item present
        const FItem& Item = Items[SlotIndex];
        if (Item.WorldObjectReference)
            CreateItemIcon(SlotIndex);

        // Make sure border is visible & laid out
        SlotBorder->SetVisibility(ESlateVisibility::Visible);
        SlotBorder->ForceLayoutPrepass();
    }
}

void UInventory::UpdateInteriorDrag(const FPointerEvent& MouseEvent)
{
    if (DragState != EDragState::Moved && DragState != EDragState::Select)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveItem: Not dragging, exiting"));

        #if	WITH_EDITOR
            UE_LOG(LogTemp, Error, TEXT("Item needs to selected and moving for executiong interior drag"));
        #else
            UE_LOG(LogTemp, Fatal, TEXT("Item needs to selected and moving for executiong interior drag"));
        #endif
        
        return;
    }

    HoveredSlotIndex = FindHoveredSlot(MouseEvent);

    if (HoveredSlotIndex == INDEX_NONE || !Items.IsValidIndex(HoveredSlotIndex))
    {
        #if	WITH_EDITOR
             UE_LOG(LogTemp, Error, TEXT("Hovered slot index %d is invalid on UpdateInteriorDrag()"), HoveredSlotIndex);
        #else
             UE_LOG(LogTemp, Fatal, TEXT("Hovered slot index %d is invalid on UpdateInteriorDrag()"), HoveredSlotIndex);
        #endif
        
        return;
    }

    if (HoveredSlotIndex == OriginSlotIndex)
    {
        #if	WITH_EDITOR
             UE_LOG(LogTemp, Log, TEXT("When hovered slot index %d is the same as original slot index then don't perform interior "), HoveredSlotIndex, OriginSlotIndex);
        #endif
        return;
    }

    if (Items[HoveredSlotIndex].WorldObjectReference)
    { 
        Items[OriginSlotIndex] = Items[HoveredSlotIndex];

        Items[HoveredSlotIndex] = DraggedItem;

        #if	WITH_EDITOR
             UE_LOG(LogTemp, Log, TEXT("Swapped item %d with item in slot %d on UpdateInteriorDrag()"), DraggedItem.Index, HoveredSlotIndex);
        #endif
    }
    else
    {
        Items[HoveredSlotIndex] = DraggedItem;
        
        Items[OriginSlotIndex] = FItem();
        
        UE_LOG(LogTemp, Log, TEXT("MoveItem: Moved item %d to empty slot %d"), DraggedItem.Index, HoveredSlotIndex);

        #if	WITH_EDITOR
             UE_LOG(LogTemp, Log, TEXT("Moved item %d to empty slot %d on UpdateInteriorDrag()"), raggedItem.Index, HoveredSlotIndex);
        #endif
    }


   OriginSlotIndex = HoveredSlotIndex;
   HoveredSlotIndex = OriginSlotIndex;

   RefreshInventory();
}

void UInventory::CreateItemIcon(uint32 SlotIndex)
{
    // Validate indices
    if (!Items.IsValidIndex(SlotIndex) || !ForegroundBorders.IsValidIndex(SlotIndex))
        return;

    // Get the SizeBox from the border
    TObjectPtr<USizeBox> SizeBox = Cast<USizeBox>(ForegroundBorders[SlotIndex]->GetContent());
    if (!SizeBox)
        return;

    // Ensure we have a single overlay container
    UOverlay* IconOverlay = Cast<UOverlay>(SizeBox->GetContent());
    if (!IconOverlay)
    {
        IconOverlay = NewObject<UOverlay>(this);
        IconOverlay->SetVisibility(ESlateVisibility::Visible);
        SizeBox->SetContent(IconOverlay);
    }
    else // Clear out any old icon/text
        IconOverlay->ClearChildren();

    // --- 1) Create and configure the item icon ---

    UImage* ItemIcon = NewObject<UImage>(this);
    ItemIcon->SetColorAndOpacity(FLinearColor::Blue);
    ItemIcon->SetVisibility(ESlateVisibility::Visible);

    // Fill the overlay slot
    if (UOverlaySlot* ImageSlot = IconOverlay->AddChildToOverlay(ItemIcon))
    {
        ImageSlot->SetHorizontalAlignment(HAlign_Fill);
        ImageSlot->SetVerticalAlignment(VAlign_Fill);
    }


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

void UInventory::Create()
{
    if (!Grid || !WidgetTree) return;

    Grid->ClearChildren();

    ForegroundBorders.Empty();
    
    Items.Empty();

    Items.SetNum(MaxRows * MaxColumns);
    ForegroundBorders.SetNum(MaxRows * MaxColumns);

    for (int32 Rows = 0; Rows < (int32)MaxRows; ++Rows)
    {
        for (int32 Columns = 0; Columns < (int32)MaxColumns; ++Columns)
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
