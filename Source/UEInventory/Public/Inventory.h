// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "UEInventory/Item.h"
#include "Inventory.generated.h"

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
	TObjectPtr<UUniformGridPanel> InventoryGrid;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UTextBlock> InventoryName;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TArray<FItem> Items;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	bool bIsVisible;
	
protected:

	virtual void NativeConstruct() override;

public:

	void Create(uint64 Rows, uint64 Columns);

	void AddItem(UTexture2D NewItem);

	void RemoveItem();

	void MoveItem();

	void SortItem(FItem MovedItem, FItem ItemToMove);

	void Open();

	void Close();

	bool GetIsVisible() const;

	void SetIsVisible(bool bVisible);

	TArray<FItem>& GetItems();
};