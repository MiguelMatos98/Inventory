#pragma once

#include "CoreMinimal.h"
#include "Item.generated.h"

USTRUCT(BlueprintType)
struct FItem
{
	GENERATED_BODY();
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TObjectPtr<UImage> Icon;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TObjectPtr<AActor> ReferencedActor;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	FVector WorldLocation;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	FVector2D IconPosition;
	
	FItem()
	: Icon(nullptr)
	, ReferencedActor(nullptr)
	, WorldLocation(FVector::ZeroVector)
	, IconPosition(FVector2D::ZeroVector)
	{
	}
};
