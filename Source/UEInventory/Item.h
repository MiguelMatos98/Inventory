#pragma once

#include "CoreMinimal.h"
#include "Engine\Texture2D.h"
#include "Item.generated.h"

USTRUCT(BlueprintType)
struct FItem
{
	GENERATED_BODY();

	UPROPERTY(VisibleAnywhere, Category = "Items")
	TWeakObjectPtr<UTexture2D> Texture;

	UPROPERTY(VisibleAnywhere, Category = "Items")
	bool bIsDraggable;

	UPROPERTY(VisibleAnywhere, Category = "Items")
	TWeakObjectPtr<AActor> TextureOwner;

	FItem();
	
	FItem(AActor* TextureOwner, UTexture2D* Texture, bool bIsDraggable);
};
