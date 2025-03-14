#pragma once

#include "CoreMinimal.h"
#include "Item.generated.h"

USTRUCT(BlueprintType)
struct FItem
{
	GENERATED_BODY();

	// The captured texture from the render target
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TWeakObjectPtr<UTexture2D> Texture;

	// Store the original 2D screen position (or world position if you prefer)
	UPROPERTY(VisibleAnywhere, Category = "Items")
	FVector OriginalPosition;

	FItem();

	FItem(UTexture2D* Texture, const FVector& OriginalPosition);
};
