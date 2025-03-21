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
	TMap<TObjectPtr<UBorder>, uint64> ForegroundBorders;

    UPROPERTY(VisibleAnywhere, Category = "Inventory")
    TObjectPtr<UBorder> BackgroundBorder;

    UPROPERTY(VisibleAnywhere, Category = "Inventory")
    TObjectPtr<UCanvasPanelSlot> BackgroundBorderSlot;

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
    UFUNCTION()
    void SpawnItemIcon(FVector2D ScreenPosition);

    UFUNCTION()
    void Create(uint64 Rows, uint64 Columns);

    UFUNCTION()
    void AddItem(AActor* ItemActor);

    UFUNCTION()
    FItem RemoveItem();

    UFUNCTION()
    void MoveItem();

    UFUNCTION()
    void SortItems(FItem MovedItem, FItem ItemToMove, EDirection Direction);

    UFUNCTION()
    void Open();

    UFUNCTION()
    void Close();

    UFUNCTION()
    TArray<FItem>& GetItems();
    
    UFUNCTION()
    bool GetIsInventoryFull(); 
    
    UFUNCTION()
    void GetItemIndexAtPosition(FVector2D ScreenPosition, uint64& OutIndex);

	UFUNCTION()
	static uint64 GetMaxRows();

	UFUNCTION()
	static uint64 GetMaxColumns();
};