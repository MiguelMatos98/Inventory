// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "ItemActor.generated.h"

/*
	New Item actor created for potentially avoiding using icons with indexing
	and instead use actors with proper textures
 */
UCLASS()
class UEINVENTORY_API AItemActor : public AStaticMeshActor
{
	GENERATED_BODY()

protected: 
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> Texture;

public:

	TObjectPtr<UTexture2D> GetTexture() const;
};
