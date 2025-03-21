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
	bool bIsSelected;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TSoftObjectPtr<UTexture2D> IconTexture;  // Store texture instead of UI widget
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TSubclassOf<AActor> ReferencedActorClass;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	FVector WorldLocation;
	
	UPROPERTY(VisibleAnywhere, Category = "Items")
	FVector2D IconPosition;

	FItem()
		: bIsSelected(false)
		, IconTexture(nullptr)
		, ReferencedActorClass(nullptr)
		, WorldLocation(FVector::ZeroVector)
		, IconPosition(FVector2D::ZeroVector)
	{
	}
};
