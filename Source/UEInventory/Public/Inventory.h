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
	TArray<FItem> Items;
	
protected:

	virtual void NativeOnInitialized() override;

public:
	
	void Create(uint64 Rows, uint64 Columns);

	void AddItem(TWeakObjectPtr<UTexture2D> NewItem);
	
	void RemoveItem();
	
	void MoveItem();

	void SortItem(FItem MovedItem, FItem ItemToMove);
	
	void Open();
	
	void Close();
	
	TArray<FItem>& GetItems();
};