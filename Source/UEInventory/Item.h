#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Texture2D.h"
#include "Item.generated.h"


// Direction enum created for setting Item movement
UENUM(BlueprintType)
enum class EItemState : uint8
{
    Null
};

USTRUCT(BlueprintType)
struct FItem
{
    GENERATED_BODY()

private:
    // (Made unaccessable just in case)
    // Item Texture (Not used but may need in future)
    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSoftObjectPtr<UTexture2D> Texture;

public:
    // Item Reference To World Object 
    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSubclassOf<AActor> WorldObjectReference;

    // Item's World Object Transform
    UPROPERTY(VisibleAnywhere, Category = "Items")
    FTransform WorldObjectTransform;

    // Item Reference To World Object's Mesh  
    UPROPERTY(VisibleAnywhere, Category = "Items")
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    // Item's Array Of World Object's Materials
    UPROPERTY()
    TArray<TSoftObjectPtr<UMaterialInterface>> StoredMaterials;

    // Item index
    UPROPERTY(VisibleAnywhere, Category = "Items")
    int32 Index;

    // Default Constructor 
    FItem();

    // Operator Overload For Item Assignment 
    bool operator==(const FItem& Other) const;
};

