// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "DiContainerObject.h"
#include "DiContextInterface.h"
#include "Container/ChainedDiContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DiGameInstanceSubsystem.generated.h"

/**
 * Di Context for the game instance.
 * Requires UTentacleSettings::bEnableScopeSubsystems to be enabled.
 * Game Instances chain to the engine.
 * This context should be used for things that share the lifetime of a game instance (especially relevant for PIE).
 */
UCLASS()
class TENTACLE_API UDiGameInstanceSubsystem : public UGameInstanceSubsystem, public IDiContextInterface
{
	GENERATED_BODY()

public:
	// - USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	// - IDIContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override { return *DiContainerGCd.DiContainer; };
	virtual const DI::FChainedDiContainer& GetDiContainer()  const override { return *DiContainerGCd.DiContainer; };
	// --

protected:
	UPROPERTY()
	FChainedDiContainerGCd DiContainerGCd;
};
