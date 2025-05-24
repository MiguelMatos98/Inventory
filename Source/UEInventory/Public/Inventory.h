#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridSlot.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Styling/SlateTypes.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"
#include "Engine/GameViewportClient.h"
#include "Item.h"
#include "Slate.h"
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

    UFUNCTION(BlueprintCallable)
    void AddItem(AActor* ItemActor);

    UFUNCTION()
    void RemoveItem(uint64 SlotIndex);

    UFUNCTION()
    uint64 FindHoveredItemIndex(const FPointerEvent& InMouseEvent);

    UFUNCTION(BlueprintCallable)
    void MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished);

    UFUNCTION()
    EDirection GetMoveDirection(uint64 RowA, uint64 ColA, uint64 RowB, uint64 ColB);

    UFUNCTION()
    void ShiftItems(uint64 StartIndex, uint64 EndIndex, EDirection Direction, bool bUpdateUI);

    UFUNCTION()
    void UpdateSlotUI(uint64 SlotIndex);

    UFUNCTION()
    void CreateItemIcon(uint64 SlotIndex);

    UFUNCTION()
    void CreateIconCounterText(uint64 SlotIndex);

    UFUNCTION()
    uint64 FindFirstEmptySlot() const;

    UFUNCTION()
    void RemoveItemIcon(uint64 SlotIndex);

    UFUNCTION(BlueprintCallable)
    void Create();

    UFUNCTION(BlueprintCallable)
    void Open();

    UFUNCTION(BlueprintCallable)
    void Close();

    UFUNCTION(BlueprintCallable)
    bool GetIsInventoryFull() const;

    UFUNCTION(BlueprintCallable)
    TArray<FItem>& GetItems();

    UFUNCTION(BlueprintCallable)
    TArray<UBorder*> GetForegroundBorders();

    UFUNCTION(BlueprintCallable)
    UUniformGridPanel* GetGrid();

    UFUNCTION(BlueprintCallable)
    EDirection SortItem(FItem& MovedItem, FItem& ItemToMove);

    UFUNCTION(BlueprintCallable)
    int32 FindItemIndex(const FItem& TargetItem) const;

    UFUNCTION()
    void ScheduleSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction);

    UFUNCTION()
    void StartSlideAnimation(uint64 FromIndex, uint64 ToIndex, EDirection Direction);

    UFUNCTION()
    FVector2D GetSlotPosition(uint64 SlotIndex) const;

    UFUNCTION(BlueprintCallable)
    float CustomEaseInOut(float T);

    UFUNCTION(BlueprintCallable)
    bool IsEdgeSlot(int32 SlotIndex) const;

protected:
    UPROPERTY()
    TArray<FItem> Items;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> ForegroundBorders;

    UPROPERTY()
    TArray<TObjectPtr<USizeBox>> IconSlots;

    UPROPERTY()
    TArray<bool> bCounterTextUpdated;

    UPROPERTY()
    bool bIsInventoryFull;

    UPROPERTY()
    uint64 DraggedItemIndex;

    UPROPERTY()
    uint64 OriginalSlotIndex;

    UPROPERTY()
    uint64 PreviousSlotIndex;

    UPROPERTY()
    bool bIsDragging;

    UPROPERTY()
    bool bIsSliding;

    UPROPERTY()
    uint64 SlideFromIndex;

    UPROPERTY()
    uint64 SlideToIndex;

    UPROPERTY()
    float SlideProgress;

    UPROPERTY()
    float SlideDuration;

    UPROPERTY()
    TArray<TObjectPtr<UOverlay>> SlidingOverlays;

    UPROPERTY()
    TArray<uint64> SlideFromIndices;

    UPROPERTY()
    TArray<uint64> SlideToIndices;

    UPROPERTY()
    TArray<FItem> SlidingItems;

    UPROPERTY()
    FItem SlidingItem;

    UPROPERTY()
    bool bAnimationScheduled;

    UPROPERTY()
    uint64 ScheduledFromIndex;

    UPROPERTY()
    uint64 ScheduledToIndex;

    UPROPERTY()
    EDirection ScheduledDirection;

    UPROPERTY()
    uint64 MoveCount;
    
    static uint64 ItemCounter;

    UPROPERTY()
    FItem DraggedItem;

    UPROPERTY()
    TObjectPtr<UOverlay> DraggedItemWidget;

    UPROPERTY()
    bool bDragStarted;

    UPROPERTY()
    FVector2D DragStartPosition;

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
    TObjectPtr<UUniformGridSlot> GridSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 MaxRows;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 MaxColumns;
};
