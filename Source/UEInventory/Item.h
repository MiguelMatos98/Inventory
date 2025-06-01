#pragma once

#include "CoreMinimal.h"
#include "Item.generated.h"

class UImage;
class AActor;
class UTexture2D;

USTRUCT(BlueprintType)
struct FItem
{
    GENERATED_BODY()

    /** Icon texture for UI */
    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSoftObjectPtr<UTexture2D> IconTexture;

    /** Actor class to respawn when dropped */
    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSubclassOf<AActor> ReferencedActorClass;

    /** Transform to spawn actor at */
    UPROPERTY(VisibleAnywhere, Category = "Items")
    FTransform WorldTransform;

    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY()
    TArray<TSoftObjectPtr<UMaterialInterface>> StoredMaterials;

    /** Index in inventory */
    UPROPERTY(VisibleAnywhere, Category = "Items")
    int32 Index;

    /** Can this item be dragged in the inventory? */
    UPROPERTY(VisibleAnywhere, Category = "Items")
    bool bIsDraggable;

    FItem()
        : IconTexture(nullptr)
        , ReferencedActorClass(nullptr)
        , WorldTransform(FTransform::Identity)
        , Index(0)
        , bIsDraggable(true)
    {}

    
    bool IsValidItem() const { return ReferencedActorClass != nullptr; }

    bool operator==(const FItem& Other) const
    {
        return ReferencedActorClass == Other.ReferencedActorClass && Index == Other.Index;
    }

    bool operator!=(const FItem& Other) const
    {
        return !(*this == Other);
    }
};
