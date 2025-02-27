// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"

void UInventory::NativeConstruct()
{
	Super::NativeConstruct();
}

void UInventory::Create(uint64 Rows, uint64 Columns)
{
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


