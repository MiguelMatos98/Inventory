
#include "Item.h"

FItem::FItem()
	: Texture(nullptr), OriginalPosition(FVector::ZeroVector)
{
	
}

FItem::FItem(UTexture2D* Texture, const FVector& OriginalPosition)
{
	this->Texture = Texture;
	this->OriginalPosition = OriginalPosition;
}
