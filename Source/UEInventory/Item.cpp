#include "Item.h"

// Initializing Members With Default Values
FItem::FItem()
	: 	Texture(nullptr),
		WorldObjectReference(nullptr),
		WorldObjectTransform(FTransform::Identity),
		Index(0)
{
}

bool FItem::operator==(const FItem& Other) const
{
	// Returns True When Two Items Are Equal
	return WorldObjectReference == Other.WorldObjectReference && Index == Other.Index;
}