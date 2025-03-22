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
	
protected:

	virtual void NativeOnInitialized() override;

public:

	static constexpr uint64  MaxColumns = 4;
	static constexpr uint64  MaxRows = 3;

	UFUNCTION()
	int64 FindHoveredItemIndex(const FPointerEvent& InMouseEvent);
	
	UFUNCTION()
	void SpawnItemIcon(FVector2D ScreenPosition);

	UFUNCTION()
	void Create(uint64 Rows, uint64 Columns);

	UFUNCTION()
	void AddItem(AActor* ItemActor);

	UFUNCTION()
	void RemoveItem();

	UFUNCTION()
	void MoveItem(const FPointerEvent& InMouseEvent, bool bStartMove, bool bEndMove);

	UFUNCTION()
	void SortItem(FItem MovedItem, FItem ItemToMove);

	UFUNCTION()
	void Open();

	UFUNCTION()
	void Close();

	UFUNCTION()
	bool GetIsInventoryFull();
	
	UFUNCTION()
	TArray<FItem>& GetItems();

	TArray<TObjectPtr<UBorder>> GetForegroundBorders();

	TObjectPtr<UUniformGridPanel> GetGrid();
};