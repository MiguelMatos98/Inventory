// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"
#include "Blueprint/WidgetTree.h"


void UInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());

		if (RootCanvas)
		{
			// Add canvas to the root widget
			WidgetTree->RootWidget = RootCanvas;
			
			// Create instance of UUniformGridPanel at run time
			Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	
			if (Grid)
			{
				// Add inventory grid to the canvas panel
				RootCanvas->AddChild(Grid);

				GridSlot = Cast<UCanvasPanelSlot>(Grid->Slot);
				
				if (GridSlot)
				{
					GridSlot->SetAnchors(FAnchors(1, 0, 1, 0));
					GridSlot->SetOffsets(FMargin(-300, 50, 0, 0));
				}

				Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				Title->SetText(FText::FromString(TEXT("Inventory")));
				RootCanvas->AddChild(Title);

				//	Grab inventory's name slot and offset it 
				TitleSlot = Cast<UCanvasPanelSlot>(Title->Slot);
				if (TitleSlot)	
				{
					TitleSlot->SetAnchors(FAnchors(1, 0, 1, 0));
					TitleSlot->SetOffsets(FMargin(-300, 10, 0, 0));
				}
			}
		
		}
	}
	
	if (Title)
	{
		Title->SetText(FText::FromString(TEXT("Inventory")));
	}

	Create(3,4);
}

void UInventory::Create(uint64 Rows, uint64 Columns)
{
	// Clear all the slots before creating the inventory grid
	SlotBorders.Empty(); 
	
	for (uint64 CurrentRow = 0; CurrentRow < Rows; ++CurrentRow)
	{
		for (uint64 CurrentColumn = 0; CurrentColumn < Columns; ++CurrentColumn)
		{
			// Add a border to the slot array
			SlotBorders.Add(WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()));

			SlotBorders.Last()->SetBrushColor(FLinearColor::Gray);  // Example border color
			SlotBorders.Last()->SetPadding(FMargin(5, 5, 5, 5));  // Example padding
			
			// Add last slot border to the inventory grid
			Grid->AddChildToUniformGrid(SlotBorders.Last(), CurrentRow, CurrentColumn);
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


