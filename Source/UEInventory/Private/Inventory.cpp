// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"

void UInventory::NativeConstruct()
{
	Super::NativeConstruct();
}

void UInventory::Create(uint64 Rows, uint64 Columns)
{
}

void UInventory::AddItem(UTexture2D NewItem)
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
}

void UInventory::Close()
{
}

bool UInventory::GetIsVisible() const
{
	return bIsVisible;
}

void UInventory::SetIsVisible(bool bVisible)
{
	bIsVisible = bVisible;
}

TArray<FItem>& UInventory::GetItems()
{
	return Items;
}


