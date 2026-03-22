// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Contexts/DiContainerObject.h"

void UDiContainerObject::AddReferencedObjects(UObject* Self, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(Self, Collector);
	static_cast<UDiContainerObject*>(Self)->DiContainer->AddReferencedObjects(Collector);
}
