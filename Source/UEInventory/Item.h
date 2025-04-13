// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UUniformGridPanel;
class UUniformGridSlot;
class UTextBlock;
class UOverlay;
class UBorder;
class FReply;
struct FItem;

UENUM(BlueprintType)
enum class EDirection : uint8
{
	Up UMETA(DisplayName = "Up"),
	Down UMETA(DisplayName = "Down"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

UCLASS()
class UEINVENTORY_API UInventory : public UUserWidget
{
	GENERATED_BODY()

private:
	
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UCanvasPanel> Canvas;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UCanvasPanelSlot> CanvasSlot;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UUniformGridSlot> GridSlot;
	
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UUniformGridPanel> Grid;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UCanvasPanelSlot> TitleSlot;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UTextBlock> Title;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TArray<TObjectPtr<UBorder>> ForegroundBorders;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UCanvasPanelSlot>  BackgroundBorderSlot;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TArray<TObjectPtr<UCanvasPanelSlot>> IconSlots;
	
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TArray<FItem> Items;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	bool bIsInventoryFull;
	
	static uint64 ItemCounter;

	// Drag state variables
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	bool bIsDragging;
    
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	int32 DraggedItemIndex;

	UPROPERTY(EditAnywhere, Category = "Inventory|Animations", meta = (BindWidgetAnim))
	UWidgetAnimation* SlideSwap;
    
	
protected:

	virtual void NativeOnInitialized() override;

	// Override native mouse events:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
public:

	static constexpr uint64  MaxColumns = 4;
	static constexpr uint64  MaxRows = 3;

	
	UFUNCTION()
	uint64 FindHoveredItemIndex(const FPointerEvent& InMouseEvent);

	UFUNCTION()
	void CreateItemIcon(uint64 SlotIndex);

	UFUNCTION()
	void CreateIconCounterText(uint64 SlotIndex);
	
	UFUNCTION()
	uint64 FindFirstEmptySlot() const;

	TObjectPtr<UOverlay> FindDraggedOverlay(uint64 ItemIndex);

	UFUNCTION()
	void Create(uint64 Rows, uint64 Columns);

	UFUNCTION()
	void AddItem(AActor* ItemActor);

	UFUNCTION()
	void RemoveItem();

	UFUNCTION()
	EDirection GetMoveDirection(uint64 RowA, uint64 ColA, uint64 RowB, uint64 ColB);

	UFUNCTION()
	void MoveItem(const FPointerEvent& MouseEvent, bool bItemMovementStarted, bool bItemMovementFinished);

	UFUNCTION()
	EDirection SortItem(FItem& MovedItem, FItem& ItemToMove);

	UFUNCTION()
	int32 FindItemIndex(const FItem& TargetItem) const;

	UFUNCTION()
	void Open();

	UFUNCTION()
	void Close();

	UFUNCTION()
	bool GetIsInventoryFull() const;
	
	UFUNCTION()
	TArray<FItem>& GetItems();

	TArray<TObjectPtr<UBorder>> GetForegroundBorders();

	TObjectPtr<UUniformGridPanel> GetGrid();

	// Store animation data
	struct FSlideAnimationData
	{
		TObjectPtr<UBorder> WidgetA;
		TObjectPtr<UBorder> WidgetB;
		FVector2D StartPosA;
		FVector2D StartPosB;
		FVector2D EndPosA;
		FVector2D EndPosB;
		int32 IndexA;
		int32 IndexB;
		float ElapsedTime;
	};
	FSlideAnimationData CurrentSlideData;

	UFUNCTION()
	void UpdateSlideTick();

	// New helper to finalize grid after animation
	UFUNCTION()
	void FinalizeSlideAnimation();
};
