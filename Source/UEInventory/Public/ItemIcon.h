// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ItemIcon.generated.h"

/*
	New Item icon actor created to capture a scene object, 
	providing the ability to create a static texture (A.K.A texture)
 */
UCLASS()
class UEINVENTORY_API AItemIcon : public AActor
{
	GENERATED_BODY()
	
protected:

	// Scene capture component responsible for capturing world object
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneCaptureComponent2D> SceneCapture; 

	// Using weak pointer to prenvent forced destruction
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TWeakObjectPtr<AActor> ObjectToCapture; 

	AItemIcon();

	// Method used for trigggering capture (Through editor)
	UFUNCTION(CallInEditor)
	void CaptureObject();

};
