
#include "Item.h"

FItem::FItem()
	: TextureOwner(nullptr), Texture(nullptr), bIsDraggable(false) 
{
	
}

FItem::FItem(AActor* TextureOwner, UTexture2D* Texture, bool bIsDraggable)
{
	this->TextureOwner = TextureOwner;
	this->Texture = Texture;
	this->bIsDraggable = bIsDraggable;
}
