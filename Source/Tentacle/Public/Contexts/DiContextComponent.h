// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "DiContainerObject.h"
#include "DiContextInterface.h"
#include "TentacleSettings.h"
#include "Components/ActorComponent.h"
#include "Container/ChainedDiContainer.h"

#include "DiContextComponent.generated.h"

/**
 * Component to put DiContext functionality on blueprint created actors that cannot implement IDiContextInterface themselves.
 * @see DI::TryFindDiContext(UObject*)
 */
UCLASS(meta=(BlueprintSpawnableComponent), NotBlueprintable)
class TENTACLE_API UDiContextComponent : public UActorComponent, public IDiContextInterface
{
	GENERATED_BODY()

public:
	UDiContextComponent();

	// - UActorComponent
	virtual void InitializeComponent() override;
	// - UObject
	static void AddReferencedObjects(UObject* Self, FReferenceCollector& Collector);
	// - IDIContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override { return *DiContainer; }
	virtual const DI::FChainedDiContainer& GetDiContainer()  const override { return *DiContainer; }
	// --

	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="Dependency Injection")
	void SetAsParentOnAllComponentsOf(AActor* Actor) const;


protected:
	UPROPERTY(EditAnywhere, Category="Dependency Injection", meta=(EditCondition="GetDefault<UTentacleSettings>()->bEnableScopeSubsystems"))
	bool bRegisterWorldAsParent = false;

	TSharedRef<DI::FChainedDiContainer> DiContainer = MakeShared<DI::FChainedDiContainer>();
};
