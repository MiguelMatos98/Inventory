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
#include "UEInventory/Item.h"
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
#include "Inventory.generated.h"

UENUM(BlueprintType)
enum class EDirection : uint8
{
    Up,
    Down,
    Left,
    Right,
    None = 255
};

USTRUCT()
struct FSlideData
{
    GENERATED_BODY()

    TObjectPtr<UOverlay> WidgetA;
    TObjectPtr<UOverlay> WidgetB;
    FVector2D StartPosA;
    FVector2D StartPosB;
    FVector2D EndPosA;
    FVector2D EndPosB;
    uint64 IndexA;
    uint64 IndexB;
    float ElapsedTime;

    FSlideData()
        : WidgetA(nullptr), WidgetB(nullptr),
        StartPosA(FVector2D::ZeroVector), StartPosB(FVector2D::ZeroVector),
        EndPosA(FVector2D::ZeroVector), EndPosB(FVector2D::ZeroVector),
        IndexA(0), IndexB(0), ElapsedTime(0.0f)
    {
    }
};

UCLASS()
class UEINVENTORY_API UInventory : public UUserWidget
{
    GENERATED_BODY()

public:
    UInventory(const FObjectInitializer& ObjectInitializer);

    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(AActor* ItemActor);

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 MaxRows = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 MaxColumns = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    UWidgetAnimation* SlideSwap;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> Canvas;

    UPROPERTY()
    TObjectPtr<UCanvasPanelSlot> BackgroundBorderSlot;

    UPROPERTY()
    TObjectPtr<UBorder> BackgroundBorder;

    UPROPERTY()
    TObjectPtr<UTextBlock> Title;

    UPROPERTY()
    TObjectPtr<UUniformGridPanel> Grid;

    UPROPERTY()
    TObjectPtr<UUniformGridSlot> GridSlot;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> ForegroundBorders;

    UPROPERTY()
    TArray<FItem> Items;

    UPROPERTY()
    TArray<TObjectPtr<USizeBox>> IconSlots;

    UPROPERTY()
    bool bIsInventoryFull;

    UPROPERTY()
    uint64 DraggedItemIndex;

    UPROPERTY()
    FItem DraggedItem;

    UPROPERTY()
    TObjectPtr<UOverlay> DraggedOverlay;

    UPROPERTY()
    TArray<bool> bCounterTextUpdated;

    UPROPERTY()
    FSlideData CurrentSlideData;

    static uint64 ItemCounter;

    void Create();
    void CreateItemIcon(uint64 SlotIndex);
    void CreateIconCounterText(uint64 SlotIndex);
    uint64 FindFirstEmptySlot() const;
    TObjectPtr<UOverlay> FindDraggedOverlay(uint64 ItemIndex);
    EDirection GetMoveDirection(uint64 RowA, uint64 ColA, uint64 RowB, uint64 ColB);
    EDirection SortItem(FItem& MovedItem, FItem& ItemToMove);
    int32 FindItemIndex(const FItem& TargetItem) const;
    void RemoveItem();
    void UpdateSlideTick();
    void UpdateSlotUI(uint64 SlotIndex);
    void RemoveItemIcon(uint64 SlotIndex);
    void ShiftItems(uint64 FromIndex, uint64 ToIndex);
};
