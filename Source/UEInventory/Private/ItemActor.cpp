// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemActor.h"


TObjectPtr<UTexture2D> AItemActor::GetTexture() const
{
	return Texture.LoadSynchronous();
}

