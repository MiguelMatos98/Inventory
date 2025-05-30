#include "Inventory.h"

uint32 UInventory::ItemCounter = 0;

UInventory::UInventory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
 ,MaxRows(3)
 , MaxColumns(4)
 , bIsInventoryFull(false)
 , DraggedItemWidget(nullptr)
 , bPendingRemoval(false)
 , DraggedItemIndex(INDEX_NONE)
 , OriginalSlotIndex(INDEX_NONE)
 , PreviousSlotIndex(INDEX_NONE)
 , bIsDragging(false)
 , bDragStarted(false)
 , DragStartPosition(FVector2D::ZeroVector)
 , bIsSliding(false)
 , SlideFromIndex(INDEX_NONE)
 , SlideToIndex(INDEX_NONE)
 , SlideProgress(0.0f)
 , SlideDuration(0.2f)
 , bAnimationScheduled(false)
 , ScheduledFromIndex(INDEX_NONE)
 , ScheduledToIndex(INDEX_NONE)
 , ScheduledDirection(EDirection::None)
 , MoveCount(0)
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
    ItemCounter = 0;

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

    // Set fixed background size to match the provided snippet
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
            bDragStarted = false;
            DragStartPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

            UE_LOG(LogTemp, Log, TEXT("NativeOnMouseButtonDown: Started drag potential for item %d from slot %d"),
                DraggedItem.Index, HoveredIndex);
            return FReply::Handled();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("NativeOnMouseButtonDown: No valid item at HoveredIndex=%d"), HoveredIndex);
        }
    }
    return FReply::Unhandled();
}

FReply UInventory::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
   if (!bIsDragging)
    {
        return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
    }

    FVector2D CurrentPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    UE_LOG(LogTemp, Log,
        TEXT("NativeOnMouseMove: Dragging item %d, MousePos=%s, bDragStarted=%s"),
        DraggedItem.Index,
        *CurrentPosition.ToString(),
        bDragStarted ? TEXT("True") : TEXT("False"));

    // If we've already popped out once, just move the floating widget:
    if (bDragStarted && DraggedItemWidget && Canvas)
    {
        if (UCanvasPanelSlot* WidgetSlot = Cast<UCanvasPanelSlot>(DraggedItemWidget->Slot))
        {
            WidgetSlot->SetPosition(CurrentPosition - FVector2D(50.0f, 50.0f));
        }
        return FReply::Handled();
    }

    // Otherwise, check whether cursor just crossed outside the original slot:
    {
        // Force layout so slot geometry is up to date:
        if (BackgroundBorder) BackgroundBorder->ForceLayoutPrepass();
        if (Canvas)          Canvas->ForceLayoutPrepass();
        if (Grid)            Grid->ForceLayoutPrepass();
        for (TObjectPtr<UBorder> Border : ForegroundBorders)
        {
            if (Border) Border->ForceLayoutPrepass();
        }

        bool bIsOutside = false;
        const FVector2D MouseAbs = InMouseEvent.GetScreenSpacePosition();

        if (ForegroundBorders.IsValidIndex(DraggedItemIndex)
            && ForegroundBorders[DraggedItemIndex])
        {
            const FGeometry SlotGeom =
                ForegroundBorders[DraggedItemIndex]->GetCachedGeometry();
            const FVector2D SlotTopLeft  = SlotGeom.LocalToAbsolute(FVector2D::ZeroVector);
            const FVector2D SlotSize     = SlotGeom.GetLocalSize();
            const FVector2D SlotBotRight = SlotTopLeft + SlotSize;

            if (SlotSize.X < 10.0f || SlotSize.Y < 10.0f)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("NativeOnMouseMove: Slot %d geometry invalid: Size=%s. Skipping edge check."),
                    DraggedItemIndex, *SlotSize.ToString());
                return FReply::Handled();
            }

            const int32 Row = DraggedItemIndex / MaxColumns;
            const int32 Col = DraggedItemIndex % MaxColumns;
            const float Padding = 1.0f, Buffer = 1.0f;

            if (Row == 0
                && MouseAbs.Y < SlotTopLeft.Y - Padding - Buffer)
            {
                bIsOutside = true;
            }
            else if (Row == MaxRows - 1
                && MouseAbs.Y > SlotBotRight.Y + Padding + Buffer)
            {
                bIsOutside = true;
            }
            else if (Col == 0
                && MouseAbs.X < SlotTopLeft.X - Padding - Buffer)
            {
                bIsOutside = true;
            }
            else if (Col == MaxColumns - 1
                && MouseAbs.X > SlotBotRight.X + Padding + Buffer)
            {
                bIsOutside = true;
            }

            if (bIsOutside)
            {
                // ───────────────────────────────────────────────────────────────────────────────
                // 1) “Unparent” the slot’s icon by clearing only its USizeBox children.
                //    This makes the slot appear empty without destroying the UBorder/SizeBox.
                // ───────────────────────────────────────────────────────────────────────────────
                UBorder* SlotBorder = ForegroundBorders[DraggedItemIndex];
                if (SlotBorder)
                {
                    if (USizeBox* SizeBox = Cast<USizeBox>(SlotBorder->GetContent()))
                    {
                        SizeBox->ClearChildren();
                    }
                }

                // 2) Mark that drag‐out has begun:
                bDragStarted = true;

                // 3) Build the floating widget at the mouse position:
                if (Canvas)
                {
                    DraggedItemWidget = NewObject<UOverlay>(this);
                    DraggedItemWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

                    // (a) Icon or blue square
                    UImage* ItemImage = NewObject<UImage>(this);
                    ItemImage->SetVisibility(ESlateVisibility::Visible);
                    if (DraggedItem.IconTexture.IsValid())
                    {
                        ItemImage->SetBrushFromTexture(DraggedItem.IconTexture.Get());
                    }
                    else
                    {
                        ItemImage->SetColorAndOpacity(FLinearColor::Blue);
                    }
                    UOverlaySlot* ImgSlot = DraggedItemWidget->AddChildToOverlay(ItemImage);
                    ImgSlot->SetHorizontalAlignment(HAlign_Fill);
                    ImgSlot->SetVerticalAlignment(VAlign_Fill);

                    // (b) Red index text
                    UTextBlock* CounterText = NewObject<UTextBlock>(this);
                    CounterText->SetVisibility(ESlateVisibility::Visible);
                    CounterText->SetText(FText::AsNumber(DraggedItem.Index));
                    CounterText->SetColorAndOpacity(FLinearColor::Red);
                    CounterText->SetJustification(ETextJustify::Center);
                    CounterText->SetFont(
                        FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 20));
                    UOverlaySlot* TxtSlot = DraggedItemWidget->AddChildToOverlay(CounterText);
                    TxtSlot->SetHorizontalAlignment(HAlign_Center);
                    TxtSlot->SetVerticalAlignment(VAlign_Center);

                    if (UCanvasPanelSlot* WidgetSlot = Canvas->AddChildToCanvas(DraggedItemWidget))
                    {
                        WidgetSlot->SetSize(FVector2D(100.0f, 100.0f));
                        WidgetSlot->SetPosition(CurrentPosition - FVector2D(50.0f, 50.0f));
                        WidgetSlot->SetZOrder(100);
                    }
                }

                UE_LOG(LogTemp, Log,
                    TEXT("Drag started for item %d from slot %d"),
                    DraggedItem.Index, DraggedItemIndex);
            }
        }

        // If now dragging outside, update the floating widget’s position:
        if (bDragStarted && DraggedItemWidget && Canvas)
        {
            if (UCanvasPanelSlot* WidgetSlot = Cast<UCanvasPanelSlot>(DraggedItemWidget->Slot))
            {
                WidgetSlot->SetPosition(CurrentPosition - FVector2D(50.0f, 50.0f));
            }
        }

        // Always call MoveItem even after pop‐out (to handle interior sorting)
        MoveItem(InMouseEvent, /*bItemMovementStarted=*/ false, /*bItemMovementFinished=*/ false);
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply UInventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
     if (!bIsDragging)
    {
        return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    }

    // 1) Force a layout pass so that hit tests are accurate
    if (Canvas) Canvas->ForceLayoutPrepass();
    if (Grid)   Grid->ForceLayoutPrepass();
    for (auto& B : ForegroundBorders)
    {
        if (B) B->ForceLayoutPrepass();
    }

    // 2) Figure out which slot (if any) the mouse is over now
    const uint32 HoveredIndex = FindHoveredItemIndex(InMouseEvent);

    // 3) Remove the floating widget if it exists
    if (DraggedItemWidget && Canvas)
    {
        Canvas->RemoveChild(DraggedItemWidget);
        DraggedItemWidget = nullptr;
    }

    // 4) Determine whether OriginalSlotIndex was on an edge of the grid
    const uint32 OrigRow = OriginalSlotIndex / MaxColumns;
    const uint32 OrigCol = OriginalSlotIndex % MaxColumns;
    const bool bOriginalWasEdge =
        (OrigRow == 0) ||
        (OrigRow == MaxRows - 1) ||
        (OrigCol == 0) ||
        (OrigCol == MaxColumns - 1);

    // 5) Check if mouse is still inside the grid’s bounding box
    bool bMouseInsideGrid = false;
    if (Grid)
    {
        const FGeometry GridGeom       = Grid->GetCachedGeometry();
        const FVector2D TopLeft        = GridGeom.GetAbsolutePosition();
        const FVector2D BottomRight    = TopLeft + GridGeom.GetAbsoluteSize();
        const FVector2D MouseScreenPos = InMouseEvent.GetScreenSpacePosition();

        bMouseInsideGrid =
            MouseScreenPos.X >= TopLeft.X   && MouseScreenPos.X <= BottomRight.X &&
            MouseScreenPos.Y >= TopLeft.Y   && MouseScreenPos.Y <= BottomRight.Y;
    }

    bool bHandled = false;

    //
    // CASE A: Drag never “popped out” (interior slot copy)
    //
    if (!bOriginalWasEdge && !bPendingRemoval)
    {
        if (bMouseInsideGrid)
        {
            if (HoveredIndex == OriginalSlotIndex)
            {
                // Dropped back onto the same slot → restore it
                Items[OriginalSlotIndex] = DraggedItem;
                UpdateSlotUI(OriginalSlotIndex);
            }
            else if (HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex))
            {
                // Dropped onto a different valid slot inside the grid
                if (!Items[HoveredIndex].IsValidItem())
                {
                    // Target slot is empty → move there
                    Items[HoveredIndex]      = DraggedItem;
                    Items[OriginalSlotIndex] = FItem();
                }
                else
                {
                    // Target slot occupied → swap
                    FItem Temp = Items[HoveredIndex];
                    Items[HoveredIndex]      = DraggedItem;
                    Items[OriginalSlotIndex] = Temp;
                }

                UpdateSlotUI(OriginalSlotIndex);
                UpdateSlotUI(HoveredIndex);
            }
            else
            {
                // Inside grid but over “padding” → restore to original
                Items[OriginalSlotIndex] = DraggedItem;
                UpdateSlotUI(OriginalSlotIndex);
            }
        }
        else
        {
            // Dropped outside entire grid → spawn actor in world, then remove from inventory
            UE_LOG(LogTemp, Log, TEXT("--- Entering spawn section (interior-drag branch) ---"));

            if (DraggedItem.StaticMesh.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("Spawning AStaticMeshActor with mesh: %s"), *DraggedItem.StaticMesh->GetName());
            }
            else if (DraggedItem.ReferencedActorClass)
            {
                UE_LOG(LogTemp, Log, TEXT("Spawning ReferencedActorClass: %s"), *DraggedItem.ReferencedActorClass->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("DraggedItem has neither StaticMesh nor ReferencedActorClass"));
            }

            UWorld* World = GetWorld();
            if (World)
            {
                // 1) Log and visualize the stored world transform
                const FTransform& ItemTransform = DraggedItem.WorldTransform;
                const FVector ItemLocation      = ItemTransform.GetLocation();
                const FVector ItemScale3D       = ItemTransform.GetScale3D();
                const FRotator ItemRotation     = ItemTransform.GetRotation().Rotator();

                UE_LOG(LogTemp, Log,
                    TEXT("DraggedItem.WorldTransform → Location=(%.3f, %.3f, %.3f), Rotation=(%.3f, %.3f, %.3f), Scale=(%.3f, %.3f, %.3f)"),
                    ItemLocation.X, ItemLocation.Y, ItemLocation.Z,
                    ItemRotation.Pitch, ItemRotation.Yaw, ItemRotation.Roll,
                    ItemScale3D.X, ItemScale3D.Y, ItemScale3D.Z);

                // 2) Draw a debug sphere at that location for 5 seconds so you can see where it would be
                DrawDebugSphere(World, ItemLocation, 25.0f, 12, FColor::Red, false, 100.0f);

                // 3) Override only the scale—use the same location/rotation, but force scale = (1,1,1)
                FTransform SpawnTransform;
                SpawnTransform.SetLocation(ItemLocation);
                SpawnTransform.SetRotation(ItemTransform.GetRotation());
                SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));

                UE_LOG(LogTemp, Log,
                    TEXT("Using SpawnTransform (forced scale 1): Location=(%.3f, %.3f, %.3f), Rotation=(%.3f, %.3f, %.3f), Scale=(1,1,1)"),
                    SpawnTransform.GetLocation().X, SpawnTransform.GetLocation().Y, SpawnTransform.GetLocation().Z,
                    SpawnTransform.GetRotation().Rotator().Pitch, SpawnTransform.GetRotation().Rotator().Yaw, SpawnTransform.GetRotation().Rotator().Roll);

                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride =
                    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                // If we stored a UStaticMesh, spawn an AStaticMeshActor
                if (DraggedItem.StaticMesh.IsValid())
                {
                    AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(
                        AStaticMeshActor::StaticClass(),
                        SpawnTransform,
                        SpawnParams);

                    if (MeshActor)
                    {
                        UStaticMesh* LoadedMesh = DraggedItem.StaticMesh.LoadSynchronous();
                        if (LoadedMesh)
                        {
                            UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
                            MeshComp->SetStaticMesh(LoadedMesh);
                            MeshComp->SetMobility(EComponentMobility::Movable);
                            MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                            MeshActor->SetActorEnableCollision(true);
                            MeshActor->SetActorHiddenInGame(false);
                            MeshComp->SetVisibility(true);
                            MeshActor->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

                            FVector FinalLoc  = MeshComp->GetComponentLocation();
                            FVector FinalScale= MeshComp->GetComponentScale();
                            UE_LOG(LogTemp, Log,
                                TEXT("MeshActor spawned SUCCESS. Final Location=(%.3f, %.3f, %.3f), Final Scale=(%.3f, %.3f, %.3f)"),
                                FinalLoc.X, FinalLoc.Y, FinalLoc.Z,
                                FinalScale.X, FinalScale.Y, FinalScale.Z);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Failed to load StaticMesh asset"));
                            MeshActor->Destroy();
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn AStaticMeshActor"));
                    }
                }
                // Otherwise, fall back to spawning the original actor class
                else if (DraggedItem.ReferencedActorClass)
                {
                    AActor* TrueActor = World->SpawnActor<AActor>(
                        DraggedItem.ReferencedActorClass,
                        SpawnTransform,
                        SpawnParams);

                    if (TrueActor)
                    {
                        TrueActor->SetActorEnableCollision(true);
                        TrueActor->SetActorHiddenInGame(false);
                        TrueActor->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

                        FVector ActorLoc   = TrueActor->GetActorLocation();
                        FVector ActorScale = TrueActor->GetActorScale3D();
                        UE_LOG(LogTemp, Log,
                            TEXT("ReferencedActorClass spawned SUCCESS. Location=(%.3f, %.3f, %.3f), Scale=(%.3f, %.3f, %.3f)"),
                            ActorLoc.X, ActorLoc.Y, ActorLoc.Z,
                            ActorScale.X, ActorScale.Y, ActorScale.Z);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn ReferencedActorClass actor"));
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No valid World to spawn item"));
            }

            // Remove from inventory
            RemoveItem(OriginalSlotIndex);
            UpdateSlotUI(OriginalSlotIndex);
        }

        bHandled = true;
    }
    //
    // CASE B: Drag started on an edge (pop-out)
    //
    else
    {
        if (bMouseInsideGrid && HoveredIndex != INDEX_NONE && Items.IsValidIndex(HoveredIndex))
        {
            if (!Items[HoveredIndex].IsValidItem())
            {
                // Dropped onto an empty slot → place it there
                Items[HoveredIndex]      = DraggedItem;
                Items[OriginalSlotIndex] = FItem();
                UpdateSlotUI(OriginalSlotIndex);
                UpdateSlotUI(HoveredIndex);
            }
            else
            {
                // Dropped onto occupied → swap + fallback to first empty
                FItem Other = Items[HoveredIndex];
                Items[HoveredIndex]      = DraggedItem;
                Items[OriginalSlotIndex] = FItem();

                const uint32 FirstEmpty = FindFirstEmptySlot();
                if (FirstEmpty != INDEX_NONE)
                {
                    Items[FirstEmpty] = Other;
                    UpdateSlotUI(FirstEmpty);
                }
                else
                {
                    // No empty left → restore the “other” back to original
                    Items[OriginalSlotIndex] = Other;
                    UpdateSlotUI(OriginalSlotIndex);
                }
                UpdateSlotUI(HoveredIndex);
            }

            bHandled = true;
        }
        else
        {
            // Still outside → spawn actor in world and remove
            UE_LOG(LogTemp, Log, TEXT("--- Entering spawn section (edge-drag branch) ---"));

            if (DraggedItem.StaticMesh.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("Spawning AStaticMeshActor with mesh: %s"), *DraggedItem.StaticMesh->GetName());
            }
            else if (DraggedItem.ReferencedActorClass)
            {
                UE_LOG(LogTemp, Log, TEXT("Spawning ReferencedActorClass: %s"), *DraggedItem.ReferencedActorClass->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("DraggedItem has neither StaticMesh nor ReferencedActorClass"));
            }

            UWorld* World = GetWorld();
            if (World)
            {
                // 1) Log and visualize the stored world transform
                const FTransform& ItemTransform = DraggedItem.WorldTransform;
                const FVector ItemLocation      = ItemTransform.GetLocation();
                const FVector ItemScale3D       = ItemTransform.GetScale3D();
                const FRotator ItemRotation     = ItemTransform.GetRotation().Rotator();

                UE_LOG(LogTemp, Log,
                    TEXT("DraggedItem.WorldTransform → Location=(%.3f, %.3f, %.3f), Rotation=(%.3f, %.3f, %.3f), Scale=(%.3f, %.3f, %.3f)"),
                    ItemLocation.X, ItemLocation.Y, ItemLocation.Z,
                    ItemRotation.Pitch, ItemRotation.Yaw, ItemRotation.Roll,
                    ItemScale3D.X, ItemScale3D.Y, ItemScale3D.Z);

                // 2) Draw a debug sphere at that location for 5 seconds
                DrawDebugSphere(World, ItemLocation, 25.0f, 12, FColor::Blue, false, 5.0f);

                // 3) Override only the scale—use the same location/rotation, but force scale = (1,1,1)
                FTransform SpawnTransform;
                SpawnTransform.SetLocation(ItemLocation);
                SpawnTransform.SetRotation(ItemTransform.GetRotation());
                SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));

                UE_LOG(LogTemp, Log,
                    TEXT("Using SpawnTransform (forced scale 1): Location=(%.3f, %.3f, %.3f), Rotation=(%.3f, %.3f, %.3f), Scale=(1,1,1)"),
                    SpawnTransform.GetLocation().X, SpawnTransform.GetLocation().Y, SpawnTransform.GetLocation().Z,
                    SpawnTransform.GetRotation().Rotator().Pitch, SpawnTransform.GetRotation().Rotator().Yaw, SpawnTransform.GetRotation().Rotator().Roll);

                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride =
                    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                if (DraggedItem.StaticMesh.IsValid())
                {
                    AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(
                        AStaticMeshActor::StaticClass(),
                        SpawnTransform,
                        SpawnParams);

                    if (MeshActor)
                    {
                        UStaticMesh* LoadedMesh = DraggedItem.StaticMesh.LoadSynchronous();
                        if (LoadedMesh)
                        {
                            UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
                            MeshComp->SetStaticMesh(LoadedMesh);
                            MeshComp->SetMobility(EComponentMobility::Movable);
                            MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                            MeshActor->SetActorEnableCollision(true);
                            MeshActor->SetActorHiddenInGame(false);
                            MeshComp->SetVisibility(true);
                            MeshActor->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

                            FVector FinalLoc   = MeshComp->GetComponentLocation();
                            FVector FinalScale = MeshComp->GetComponentScale();
                            UE_LOG(LogTemp, Log,
                                TEXT("MeshActor spawned SUCCESS. Final Location=(%.3f, %.3f, %.3f), Final Scale=(%.3f, %.3f, %.3f)"),
                                FinalLoc.X, FinalLoc.Y, FinalLoc.Z,
                                FinalScale.X, FinalScale.Y, FinalScale.Z);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Failed to load StaticMesh asset"));
                            MeshActor->Destroy();
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn AStaticMeshActor"));
                    }
                }
                else if (DraggedItem.ReferencedActorClass)
                {
                    AActor* TrueActor = World->SpawnActor<AActor>(
                        DraggedItem.ReferencedActorClass,
                        SpawnTransform,
                        SpawnParams);

                    if (TrueActor)
                    {
                        TrueActor->SetActorEnableCollision(true);
                        TrueActor->SetActorHiddenInGame(false);
                        TrueActor->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

                        FVector ActorLoc   = TrueActor->GetActorLocation();
                        FVector ActorScale = TrueActor->GetActorScale3D();
                        UE_LOG(LogTemp, Log,
                            TEXT("ReferencedActorClass spawned SUCCESS. Location=(%.3f, %.3f, %.3f), Scale=(%.3f, %.3f, %.3f)"),
                            ActorLoc.X, ActorLoc.Y, ActorLoc.Z,
                            ActorScale.X, ActorScale.Y, ActorScale.Z);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn ReferencedActorClass actor"));
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No valid World to spawn item"));
            }

            // Remove from inventory
            RemoveItem(OriginalSlotIndex);
            UpdateSlotUI(OriginalSlotIndex);
            bHandled = true;
        }
    }

    // 6) Clear drag state & force a final redraw of all slots
    bIsDragging       = false;
    bDragStarted      = false;
    bPendingRemoval   = false;
    DraggedItem       = FItem();
    DraggedItemIndex  = INDEX_NONE;
    PreviousSlotIndex = INDEX_NONE;
    OriginalSlotIndex = INDEX_NONE;

    if (bHandled)
    {
        for (uint32 i = 0; i < static_cast<uint32>(Items.Num()); ++i)
        {
            UpdateSlotUI(i);
        }
    }

    return FReply::Handled().ReleaseMouseCapture();
}

void UInventory::AddItem(AActor* ItemActor)
{
    if (!ItemActor) return;

    // If the inventory is already full, do nothing.
    if (ItemCounter >= static_cast<uint64>(MaxRows * MaxColumns))
    {
        bIsInventoryFull = true;
        return;
    }

    // Find the first empty slot.
    uint64 EmptySlotIndex = FindFirstEmptySlot();
    if (EmptySlotIndex == INDEX_NONE)
    {
        bIsInventoryFull = true;
        return;
    }

    // Populate a new FItem in that slot.
    FItem& NewItem = Items[EmptySlotIndex];
    NewItem = FItem();  // Reset to defaults.

    // Store the actor class so we can spawn it later if needed.
    NewItem.ReferencedActorClass = ItemActor->GetClass();

    // Store the world transform at the moment of pickup.
    NewItem.WorldTransform = ItemActor->GetActorTransform();

    // Store the index in the inventory.
    NewItem.Index = ItemCounter;

    // Attempt to grab the static mesh from the actor's components:
    if (UStaticMeshComponent* MeshComp = ItemActor->FindComponentByClass<UStaticMeshComponent>())
    {
        if (MeshComp->GetStaticMesh())
        {
            // Save that mesh into our FItem so we can reassign it when spawning back into the world.
            NewItem.StaticMesh = MeshComp->GetStaticMesh();
        }
    }

    // Update the UI for this new slot.
    UpdateSlotUI(EmptySlotIndex);

    // Destroy the actor we just picked up.
    ItemActor->Destroy();

    // Increment counter and check if the inventory is now full.
    ItemCounter++;
    bIsInventoryFull = (FindFirstEmptySlot() == INDEX_NONE);
}

void UInventory::RemoveItem(int32 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].ReferencedActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveItem: Invalid slot %d or no item to remove"), SlotIndex);
        return;
    }

    Items[SlotIndex] = FItem();
    UpdateSlotUI(SlotIndex);   // <-- this ensures the slot is cleared visually

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

uint32 UInventory::FindHoveredItemIndex(const FPointerEvent& InMouseEvent) const
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

            UE_LOG(LogTemp, Log, TEXT("FindHoveredItemIndex: Slot %d, TopLeft=%s, Size=%s, BottomRight=%s, MousePos=%s"),
                Index, *SlotAbsTopLeft.ToString(), *SlotAbsSize.ToString(), *SlotAbsBottomRight.ToString(), *MousePos.ToString());

            if (SlotAbsSize.X < 10.0f || SlotAbsSize.Y < 10.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("FindHoveredItemIndex: Slot %d geometry too small: Size=%s"),
                    Index, *SlotAbsSize.ToString());
                continue;
            }

            bAnyValidGeometry = true;

            if (MousePos.X >= SlotAbsTopLeft.X && MousePos.X <= SlotAbsBottomRight.X &&
                MousePos.Y >= SlotAbsTopLeft.Y && MousePos.Y <= SlotAbsBottomRight.Y)
            {
                FVector2D SlotCenter = SlotAbsTopLeft + (SlotAbsSize / 2.0f);
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

EDirection UInventory::GetMoveDirection(uint32 RowA, uint32 ColA, uint32 RowB, uint32 ColB) const
{
    if (RowA == RowB && ColA < ColB) return EDirection::Right;
    if (RowA == RowB && ColA > ColB) return EDirection::Left;
    if (ColA == ColB && RowA < RowB) return EDirection::Down;
    if (ColA == ColB && RowA > RowB) return EDirection::Up;
    return EDirection::None;
}

void UInventory::ShiftItems(uint32 StartIndex, uint32 EndIndex, EDirection Direction, bool bUpdateUI)
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

void UInventory::UpdateSlotUI(uint32 SlotIndex)
{
    if (!Items.IsValidIndex(SlotIndex) || !ForegroundBorders.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateSlotUI: Invalid SlotIndex=%d"), SlotIndex);
        return;
    }

    UBorder* SlotBorder = ForegroundBorders[SlotIndex];
    if (!SlotBorder) return;

    // We assume each UBorder’s content is always a USizeBox that we clear and repopulate.
    USizeBox* SizeBox = Cast<USizeBox>(SlotBorder->GetContent());
    if (!SizeBox) return;

    // 1) If dragging from an edge and this is the original slot, clear its icon but leave the border intact.
    if (bIsDragging && bDragStarted && SlotIndex == OriginalSlotIndex)
    {
        SizeBox->ClearChildren();
        return;
    }

    // 2) Otherwise, clear everything and redraw the actual item icon (with its red index) if present.
    SizeBox->ClearChildren();

    if (Items[SlotIndex].ReferencedActorClass)
    {
        CreateItemIcon(SlotIndex);
        CreateIconCounterText(SlotIndex);
    }
    // else: leave blank

    SlotBorder->SetVisibility(ESlateVisibility::Visible);
    SlotBorder->ForceLayoutPrepass();

    UE_LOG(LogTemp, Log, TEXT(
        "UpdateSlotUI: Slot %d border updated, HasItem=%s"),
        SlotIndex,
        Items[SlotIndex].ReferencedActorClass ? TEXT("True") : TEXT("False"));
}

void UInventory::CreateItemIcon(uint32 SlotIndex)
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

void UInventory::CreateIconCounterText(uint32 SlotIndex)
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

uint32 UInventory::FindFirstEmptySlot() const
{
    for (uint64 i = 0; i < static_cast<uint64>(Items.Num()); i++)
    {
        if (!Items[i].ReferencedActorClass) return i;
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

const TArray<FItem>& UInventory::GetItems() const
{
    return Items;
}

TArray<UBorder*> UInventory::GetForegroundBorders() const
{
    TArray<UBorder*> RawBorders;
    for (TObjectPtr<UBorder> Border : ForegroundBorders)
    {
        RawBorders.Add(Border.Get());
    }
    return RawBorders;
}

UUniformGridPanel* UInventory::GetGrid() const
{
    return Grid.Get();
}

EDirection UInventory::SortItem(FItem& MovedItem, FItem& ItemToMove)
{
    return EDirection::None;
}

uint32 UInventory::FindItemIndex(const FItem& TargetItem) const
{
    return INDEX_NONE;
}

void UInventory::ScheduleSlideAnimation(uint32 FromIndex, uint32 ToIndex, EDirection Direction)
{
    ScheduledFromIndex = FromIndex;
    ScheduledToIndex = ToIndex;
    ScheduledDirection = Direction;
    bAnimationScheduled = true;
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

float UInventory::CustomEaseInOut(float T) const
{
    return T < 0.5f ? 2.0f * T * T : -1.0f + (4.0f - 2.0f * T) * T;
}
