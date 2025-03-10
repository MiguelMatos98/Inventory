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

	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("WidgetTree is invalid"));
		return;
	}
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	if (!Canvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("Canvas is invalid"));
		return;
	}
		// Add canvas to the root widget
		WidgetTree->RootWidget = Canvas;

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	if (!BackgroundBorder)
	{
		UE_LOG(LogTemp, Warning, TEXT("BackgroundBorder is invalid"));
		return;
	}
		BackgroundBorder->SetBrushColor(FLinearColor::Gray);  // Example border color

	UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (!VerticalBox) {
		UE_LOG(LogTemp, Warning, TEXT("VerticalBox is invalid"));
		return;
	}
	
	Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (Title)
	{
		UE_LOG(LogTemp, Error, TEXT("Title is valid"));
		Title->SetText(FText::FromString(TEXT("Inventory")));
	
		//	Grab inventory's name slot and offset it 
		if (UVerticalBoxSlot* TitleVSlot = VerticalBox->AddChildToVerticalBox(Title))
		{
			UE_LOG(LogTemp, Error, TEXT("TitleVSlot is valid"));
			// Center horizontally, add some top padding
			TitleVSlot->SetHorizontalAlignment(HAlign_Center);
			TitleVSlot->SetPadding(FMargin(0, 20, 0, 10));
		}
	}
	
	// Create instance of UUniformGridPanel at run time
	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	if (Grid)
	{
		UE_LOG(LogTemp, Error, TEXT("Grid is valid"));
		// Reduce padding in the grid itself to avoid extra spacing
		Grid->SetSlotPadding(FMargin(10, 16, 10, 16));  // Reducing slot padding to minimal

		if (UVerticalBoxSlot* GridVSlot = VerticalBox->AddChildToVerticalBox(Grid))
		{
			UE_LOG(LogTemp, Error, TEXT("GridVSlot is valid"));
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
		BackgroundBorderSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		BackgroundBorderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		BackgroundBorderSlot->SetOffsets(FMargin(-50, 50, 510, 500));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BackgroundBorderSlot is invalid"));
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
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
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
    SetVisibility(ESlateVisibility::Collapsed);
}

TArray<FItem>& UInventory::GetItems()
{
	return Items;
}

