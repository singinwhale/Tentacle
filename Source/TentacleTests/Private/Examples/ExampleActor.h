// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Container/ChainedDiContainer.h"
#include "Contexts/AutoInjector.h"
#include "Contexts/DiContextInterface.h"
#include "GameFramework/Actor.h"
#include "ExampleActor.generated.h"

UCLASS(HideDropdown, NotBlueprintable)
class TENTACLETESTS_API AExampleActor : public AActor, public IAutoInjector, public IDiContextInterface
{
	GENERATED_BODY()

public:
	// ReSharper disable once CppEnforceOverridingFunctionStyle
	virtual void ComponentRegistered(UActorComponent* Component);

	// - IDiContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override;
	virtual const DI::FChainedDiContainer& GetDiContainer() const override;
	// --

	TSharedRef<DI::FChainedDiContainer> DiContainer = MakeShared<DI::FChainedDiContainer>();
};
