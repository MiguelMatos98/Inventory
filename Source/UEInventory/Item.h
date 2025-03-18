#pragma once

#include "CoreMinimal.h"
#include "Item.generated.h"

class UImage;
class AActor;

USTRUCT(BlueprintType)
struct FItem
{
	GENERATED_BODY();
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TObjectPtr<UImage> Icon;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TSubclassOf<AActor> ReferencedActorClass;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	FVector WorldLocation;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	FVector2D IconPosition;
	
	FItem()
	: Icon(nullptr)
	, ReferencedActorClass(nullptr)
	, WorldLocation(FVector::ZeroVector)
	, IconPosition(FVector2D::ZeroVector)
	{
	}
};
