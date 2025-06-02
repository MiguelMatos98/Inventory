#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Texture2D.h"
#include "Item.generated.h"

USTRUCT(BlueprintType)
struct FItem
{
    GENERATED_BODY()

    // Item Texture 
    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSoftObjectPtr<UTexture2D> Texture;

    // Item Reference to World Object 
    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSubclassOf<AActor> WorldObjectReverence;

    // Item's World Object Transform
    UPROPERTY(VisibleAnywhere, Category = "Items")
    FTransform WorldObjectTransform;

    // Item Reference to World Object's Mesh  
    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    // Item's Array of World Object's Materials
    UPROPERTY()
    TArray<TSoftObjectPtr<UMaterialInterface>> StoredMaterials;

    // Item index
    UPROPERTY(VisibleAnywhere, Category = "Items")
    int32 Index;

    // Default Constructor 
    FItem();

    // Operator Overload for Item asignment 
    bool operator==(const FItem& Other) const;
};

