// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Container/ChainedDiContainer.h"
#include "Contexts/DiContextInterface.h"
#include "DiContainerObject.generated.h"

/**
 * DI Context that can be passed around and used in blueprint.
 */
UCLASS(BlueprintType)
class TENTACLE_API UDiContainerObject : public UObject, public IDiContextInterface
{
	GENERATED_BODY()

public:

	// GC
	static void AddReferencedObjects(UObject* Self, FReferenceCollector& Collector);
	// --
public:
	// - IDIContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override { return *DiContainer; };
	virtual const DI::FChainedDiContainer& GetDiContainer()  const override { return *DiContainer; };
	// --

protected:
	TSharedRef<DI::FChainedDiContainer> DiContainer = MakeShared<DI::FChainedDiContainer>();
};
