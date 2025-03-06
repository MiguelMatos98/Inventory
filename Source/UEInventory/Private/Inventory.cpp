// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"

#ifndef INVENTORY_HEADERS_H
#define INVENTORY_HEADERS_H

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "UEInventory/Item.h"
#include "Blueprint/WidgetTree.h"

#endif // !INVENTORY_HEADERS_H

void UInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	if (Canvas)
	{
		// Add canvas to the root widget
		WidgetTree->RootWidget = Canvas;

		BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		if (BackgroundBorder)
		{
			BackgroundBorder->SetBrushColor(FLinearColor::Gray);  // Example border color
		
			// Create instance of UUniformGridPanel at run time
			Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
			if (Grid)
			{
				// Reduce padding in the grid itself to avoid extra spacing
				Grid->SetSlotPadding(FMargin(10, 10, 10, 10));  // Reducing slot padding to minimal
				
				BackgroundBorder->SetContent(Grid);
			}
			
			// Add inventory grid to the canvas panel
			Canvas->AddChild(BackgroundBorder);
			
			BackgroundBorderSlot = Cast<UCanvasPanelSlot>(BackgroundBorder->Slot);
			if (BackgroundBorderSlot)
			{
				BackgroundBorderSlot->SetAnchors(FAnchors(1, 1, 1, 1));
				BackgroundBorderSlot->SetOffsets(FMargin(-300, -300, 300, 300));
			}
		}
		
		//CanvasSlot = Cast<UCanvasPanelSlot>(Grid->Slot); 
		//if (CanvasSlot)
		//{
		//	CanvasSlot->SetAnchors(FAnchors(1, 1, 1, 1));
		//	CanvasSlot->SetOffsets(FMargin(-385, 385, 385,  385));  
		//}
		

		Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (Title)
		{
			Title->SetText(FText::FromString(TEXT("Inventory")));
			Canvas->AddChild(Title);

			//	Grab inventory's name slot and offset it 
			TitleSlot = Cast<UCanvasPanelSlot>(Title->Slot);
			if (TitleSlot)	
			{
				TitleSlot->SetAnchors(FAnchors(1, 1, 1, 1));
				TitleSlot->SetOffsets(FMargin(-300, -340, 200, 40));
			}
		}
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

