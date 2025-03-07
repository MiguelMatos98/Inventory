// Fill out your copyright notice in the Description page of Project Settings.

// New File

#include "Inventory.h"

#ifndef INVENTORY_HEADERS_H
#define INVENTORY_HEADERS_H

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "UEInventory/Item.h"
#include "Blueprint/WidgetTree.h"

#endif // !INVENTORY_HEADERS_H

void UInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree) return;

	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	if (!Canvas) return;
	
		// Add canvas to the root widget
		WidgetTree->RootWidget = Canvas;

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	if (!BackgroundBorder) return;
		
		BackgroundBorder->SetBrushColor(FLinearColor::Gray);  // Example border color

	UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (!VerticalBox) return;
	
	Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (Title)
	{
		Title->SetText(FText::FromString(TEXT("Inventory")));
	
		//	Grab inventory's name slot and offset it 
		UVerticalBoxSlot* TitleVSlot = VerticalBox->AddChildToVerticalBox(Title);
		if (TitleVSlot)
		{
			// Center horizontally, add some top padding
			TitleVSlot->SetHorizontalAlignment(HAlign_Center);
			TitleVSlot->SetPadding(FMargin(0, 20, 0, 10));
		}
	}
	
	// Create instance of UUniformGridPanel at run time
	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	if (Grid)
	{
		// Reduce padding in the grid itself to avoid extra spacing
		Grid->SetSlotPadding(FMargin(10, 10, 10, 10));  // Reducing slot padding to minimal
		
		UVerticalBoxSlot* GridVSlot = VerticalBox->AddChildToVerticalBox(Grid);
		if (GridVSlot)
		{
			// Fill remaining space in the VerticalBox
			GridVSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}
	
	BackgroundBorder->SetContent(VerticalBox);
	
	// Add inventory grid to the canvas panel
	Canvas->AddChild(BackgroundBorder);
	
	BackgroundBorderSlot = Cast<UCanvasPanelSlot>(BackgroundBorder->Slot);
	if (BackgroundBorderSlot)
	{
		BackgroundBorderSlot->SetAnchors(FAnchors(1, 1, 1, 1));
		BackgroundBorderSlot->SetOffsets(FMargin(-630, 50, 520, 520));
	}
	
	Create(3,4);
}

void UInventory::Create(uint64 Rows, uint64 Columns)
{

	if (!Grid)
	{
		return;
	}

	// Clear all the slots before creating the inventory grid
	Grid->ClearChildren();
	ForegroundBorders.Empty();

	for (uint64 CurrentRow = 0; CurrentRow < Rows; ++CurrentRow)
	{
		for (uint64 CurrentColumn = 0; CurrentColumn < Columns; ++CurrentColumn)
		{
			// Add a border to the slot array
			ForegroundBorders.Add(WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()));

			ForegroundBorders.Last()->SetBrushColor(FLinearColor::Black);  // Example border color
			
			// Add last slot border to the inventory grid
			GridSlot = Grid->AddChildToUniformGrid(ForegroundBorders.Last(), CurrentRow, CurrentColumn);

			if(GridSlot)
			{
				// Set the slot's alignment
				GridSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
				GridSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
			}
		}
	}
}

void UInventory::AddItem(TWeakObjectPtr<UTexture2D> NewItem)
{
}

void UInventory::RemoveItem()
{
}

void UInventory::MoveItem()
{
}

void UInventory::SortItem(FItem MovedItem, FItem ItemToMove)
{
}

void UInventory::Open()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UInventory::Close()
{
    SetVisibility(ESlateVisibility::Hidden);
}

TArray<FItem>& UInventory::GetItems()
{
	return Items;
}

