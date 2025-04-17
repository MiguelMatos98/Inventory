#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
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
#include "Item.h"
#include "Inventory.generated.h"

UENUM(BlueprintType)
enum class EDirection : uint8
{
    None,
    Up,
    Down,
    Left,
    Right
};

UCLASS()
class UInventory : public UUserWidget
{
    GENERATED_BODY()

public:
    UInventory(const FObjectInitializer& ObjectInitializer);

    static uint64 ItemCounter;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(AActor* ItemActor);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RemoveItem();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Open();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Close();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool GetIsInventoryFull() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<FItem>& GetItems();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<UBorder*> GetForegroundBorders();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UUniformGridPanel* GetGrid();

    void MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished);
    uint64 FindHoveredItemIndex(const FPointerEvent& InMouseEvent);

protected:
    void CreateItemIcon(uint64 SlotIndex);
    void CreateIconCounterText(uint64 SlotIndex);
    uint64 FindFirstEmptySlot() const;
    UOverlay* FindDraggedOverlay(uint64 ItemIndex);
    UOverlay* CreateTemporaryDragOverlay(const FItem& Item);
    EDirection GetMoveDirection(uint64 RowA, uint64 ColA, uint64 RowB, uint64 ColB);
    EDirection SortItem(FItem& MovedItem, FItem& ItemToMove);
    int32 FindItemIndex(const FItem& TargetItem) const;
    void UpdateSlotUI(uint64 SlotIndex);
    void RemoveItemIcon(uint64 SlotIndex);
    void ShiftItems(uint64 StartIndex, uint64 EndIndex, EDirection Direction, bool bUpdateUI);
    void StartSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction);
    FVector2D GetSlotPosition(uint64 SlotIndex) const;
    float CustomEaseInOut(float T);
    void Create();

    void ScheduleSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction);

private:
    UPROPERTY()
    uint64 MaxRows = 3;

    UPROPERTY()
    uint64 MaxColumns = 4;

    UPROPERTY()
    bool bIsInventoryFull;

    UPROPERTY()
    TArray<FItem> Items;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> ForegroundBorders;

    UPROPERTY()
    TArray<TObjectPtr<UOverlay>> IconSlots;

    UPROPERTY()
    TArray<bool> bCounterTextUpdated;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> Canvas;

    UPROPERTY()
    TObjectPtr<UBorder> BackgroundBorder;

    UPROPERTY()
    UCanvasPanelSlot* BackgroundBorderSlot;

    UPROPERTY()
    TObjectPtr<UTextBlock> Title;

    UPROPERTY()
    TObjectPtr<UUniformGridPanel> Grid;

    UPROPERTY()
    UUniformGridSlot* GridSlot;

    UPROPERTY()
    int32 DraggedItemIndex;

    UPROPERTY()
    int32 OriginalSlotIndex;

    UPROPERTY()
    int32 PreviousSlotIndex; // Added to track the previous slot during dragging

    UPROPERTY()
    FItem DraggedItem;

    UPROPERTY()
    TObjectPtr<UOverlay> DraggedOverlay;

    UPROPERTY()
    bool bIsDragging;

    UPROPERTY()
    bool bIsSliding;

    UPROPERTY()
    int32 SlideFromIndex;

    UPROPERTY()
    int32 SlideToIndex;

    UPROPERTY()
    float SlideProgress;

    UPROPERTY()
    float SlideDuration;

    UPROPERTY()
    FItem SlidingItem;

    UPROPERTY()
    TArray<TObjectPtr<UOverlay>> SlidingOverlays;

    UPROPERTY()
    TArray<int32> SlideFromIndices;

    UPROPERTY()
    TArray<int32> SlideToIndices;

    UPROPERTY()
    TArray<FItem> SlidingItems;

    UPROPERTY()
    bool bAnimationScheduled;

    UPROPERTY()
    uint64 ScheduledFromIndex;

    UPROPERTY()
    uint64 ScheduledToIndex;

    UPROPERTY()
    EDirection ScheduledDirection;

    UPROPERTY()
    int32 MoveCount;
};
