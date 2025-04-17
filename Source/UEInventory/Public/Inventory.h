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
#include "Engine/GameViewportClient.h"
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

    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

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
    uint64 FindHoveredItemIndex(const FPointerEvent& InMouseEvent);
    void MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished);

protected:
    uint64 MaxRows = 3;

    uint64 MaxColumns = 4;

    UPROPERTY()
    bool bIsInventoryFull;

    UPROPERTY()
    TArray<FItem> Items;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> ForegroundBorders;

    UPROPERTY()
    TArray<TObjectPtr<USizeBox>> IconSlots;

    UPROPERTY()
    TArray<bool> bCounterTextUpdated;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> Canvas;

    UPROPERTY()
    TObjectPtr<UBorder> BackgroundBorder;

    UPROPERTY()
    TObjectPtr<UCanvasPanelSlot> BackgroundBorderSlot;

    UPROPERTY()
    TObjectPtr<UTextBlock> Title;

    UPROPERTY()
    TObjectPtr<UUniformGridPanel> Grid;

    UPROPERTY()
    UUniformGridSlot* GridSlot;

    static uint64 ItemCounter;

    // Dragging state
    UPROPERTY()
    int32 DraggedItemIndex;

    UPROPERTY()
    int32 OriginalSlotIndex;

    UPROPERTY()
    int32 PreviousSlotIndex;

    UPROPERTY()
    FItem DraggedItem;

    UPROPERTY()
    bool bIsDragging;

    // Sliding animation state
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
    int32 ScheduledFromIndex;

    UPROPERTY()
    int32 ScheduledToIndex;

    UPROPERTY()
    EDirection ScheduledDirection;

    UPROPERTY()
    uint64 MoveCount;

    void Create();
    void UpdateSlotUI(uint64 SlotIndex);
    void RemoveItemIcon(uint64 SlotIndex);
    void CreateItemIcon(uint64 SlotIndex);
    void CreateIconCounterText(uint64 SlotIndex);
    uint64 FindFirstEmptySlot() const;
    EDirection GetMoveDirection(uint64 RowA, uint64 ColA, uint64 RowB, uint64 ColB);
    EDirection SortItem(FItem& MovedItem, FItem& ItemToMove);
    int32 FindItemIndex(const FItem& TargetItem) const;
    void ShiftItems(uint64 StartIndex, uint64 EndIndex, EDirection Direction, bool bUpdateUI);
    void ScheduleSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction);
    void StartSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction);
    FVector2D GetSlotPosition(uint64 SlotIndex) const;
    float CustomEaseInOut(float T);
};
