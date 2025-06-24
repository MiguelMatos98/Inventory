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


enum class EDragState : uint8
{
    Null,
    Select,
    Moved,
    Released
};

UCLASS()
class UInventory : public UUserWidget
{
    GENERATED_BODY()

public:
    UInventory(const FObjectInitializer& ObjectInitializer);

    // NativeOnInitialized used for creating and set up inventory's UI
    virtual void NativeOnInitialized() override;

    // NativeNativeConstruct used for reconstructing inventory Widgets 
    virtual void NativeConstruct() override;

    // ******************** Mouse native events for mouse input detection ********************

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // **************************************************************************************

    // ******************** Open and Close use for inventory toggle on tab ********************

    void Open();

    void Close();
    // **************************************************************************************

    void AddItem(AActor* ItemActor);

    bool GetIsInventoryFull() const;

    const TArray<FItem>& GetItems() const;

    // This return inventory grey slots 
    TArray<TObjectPtr<UBorder>>  GetForegroundBorders() const;

    TObjectPtr<UUniformGridPanel> GetGrid() const;

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
    TObjectPtr<UVerticalBox> BackgroundVerticalBox;

    UPROPERTY()
    TObjectPtr<UTextBlock> Title;

    UPROPERTY()
    TObjectPtr<UVerticalBoxSlot> TitleVerticalBoxSlot;

    UPROPERTY()
    TObjectPtr<UUniformGridPanel> Grid;

    UPROPERTY()
    TObjectPtr<UVerticalBoxSlot> GridVerticalBoxSlot;

    UPROPERTY()
    TObjectPtr<UUniformGridSlot> GridSlot;

    // Widget used for dragging item viusally outsid ethe inventory
    UPROPERTY()
    TObjectPtr<UOverlay> DraggedItemWidget;

    static uint32 ItemCounter;

    UPROPERTY()
    int32 HoveredSlotIndex;

    UPROPERTY()
    int32 OriginSlotIndex;

    UPROPERTY()
    FItem DraggedItem;

    UPROPERTY()
    FVector2D MouseScreenSpacePosition;

    UPROPERTY()
    FVector2D MouseWidgetLocalPosition;

    EDragState DragState;

private:

    void Create();

    void RefreshInventory();

    void CreateItemIcon(uint32 SlotIndex);

    int32 FindFirstEmptySlot() const;

    void UpdateInteriorDrag(const FPointerEvent& MouseEvent);

    int32 FindHoveredSlot(const FPointerEvent& InMouseEvent);
};
