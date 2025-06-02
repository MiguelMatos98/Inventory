#include "Item.h"

FItem::FItem()
	: 	Texture(nullptr),
		WorldObjectReverence(nullptr),
		WorldObjectTransform(FTransform::Identity),
		Index(0)
{
}

bool FItem::operator==(const FItem& Other) const
{
	return WorldObjectReverence == Other.WorldObjectReverence && Index == Other.Index;
}
