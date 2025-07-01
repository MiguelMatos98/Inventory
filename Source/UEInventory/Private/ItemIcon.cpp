// Fill out your copyright notice in the Description page of Project Settings.
#include "ItemIcon.h"

AItemIcon::AItemIcon()
{
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>( "Scene Capture" );

	SceneCapture->bCaptureEveryFrame = true;

	// Keep capturing object while camera moves
	SceneCapture->bCaptureOnMovement = true; 
	
	//	Filtering capture for only 
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	RootComponent = SceneCapture;
}

void AItemIcon::CaptureObject()
{
	AActor* ObjectToCaptureRef = ObjectToCapture.Get();

	// Clear list of shown actors and assign the object to capture immediately 
	SceneCapture->ShowOnlyActors.Empty();
	SceneCapture->ShowOnlyActors.Add(ObjectToCaptureRef);
}

