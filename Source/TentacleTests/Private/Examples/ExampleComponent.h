// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Contexts/AutoInjectableInterface.h"
#include "ExampleComponent.generated.h"


class USimpleUService;

UCLASS(HideDropdown, NotBlueprintable)
class TENTACLETESTS_API UExampleComponent : public UActorComponent, public IAutoInjectableInterface
{
	GENERATED_BODY()

public:
	// - AActor
	virtual void BeginPlay() override;
	// - IAutoInjector
	virtual void AutoInject_Implementation(const TScriptInterface<IDiContextInterface>& DiContext) override;
	// --

	TObjectPtr<USimpleUService> InjectDependencies(TObjectPtr<USimpleUService> SimpleUService);
	void InjectDependenciesWithExtraArgs(TObjectPtr<USimpleUService> InSimpleUService, FString ExtraString);

	UPROPERTY()
	TObjectPtr<USimpleUService> SimpleUService;

	FString ExtraString;
};
