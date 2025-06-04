#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Item.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory.generated.h"

// Item Movement Direction Enum
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
    // Default Constructor
    UInventory(const FObjectInitializer& ObjectInitializer);

    // Native Method For Creating The Inventory UI
    virtual void NativeOnInitialized() override;

    // Native Method For Reconstructing Slate Geometry 
    virtual void NativeConstruct() override;

    // Native Method For Updating Inventory UI
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // Add Item Method
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(AActor* ItemActor);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RemoveItem(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Open();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Close();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool GetIsInventoryFull() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const TArray<FItem>& GetItems() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<UBorder*> GetForegroundBorders() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UUniformGridPanel* GetGrid() const;
    
    void MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished);
    
    uint32 FindHoveredItemIndex(const FPointerEvent& InMouseEvent) const;

private:

    uint32 MaxRows;
    
    uint32 MaxColumns;

    UPROPERTY()
    bool bIsInventoryFull;

    UPROPERTY()
    TArray<FItem> Items;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> ForegroundBorders;

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

    UPROPERTY()
    TObjectPtr<UOverlay> DraggedItemWidget;

    static uint32 ItemCounter;

    // Dragging state
    UPROPERTY()
    bool bPendingRemoval;

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

    UPROPERTY()
    bool bDragStarted;

    UPROPERTY()
    FVector2D DragStartPosition;

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
    bool bAnimationScheduled;

    UPROPERTY()
    int32 ScheduledFromIndex;

    UPROPERTY()
    int32 ScheduledToIndex;

    UPROPERTY()
    EDirection ScheduledDirection;

    void Create();
    void UpdateSlotUI(uint32 SlotIndex);
    void RemoveItemIcon(uint32 SlotIndex);
    void CreateItemIcon(uint32 SlotIndex);
    void CreateIconCounterText(uint32 SlotIndex);
    uint32 FindFirstEmptySlot() const;
    EDirection GetMoveDirection(uint32 RowA, uint32 ColA, uint32 RowB, uint32 ColB) const;
    FVector2D GetSlotPosition(uint32 SlotIndex) const;
};
