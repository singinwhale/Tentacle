// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "DiContainerObject.h"
#include "Container/ChainedDiContainer.h"
#include "DiContextInterface.h"
#include "Subsystems/EngineSubsystem.h"
#include "DiEngineSubsystem.generated.h"

/**
 * Di Context for the Engine.
 * Requires UTentacleSettings::bEnableScopeSubsystems to be enabled.
 * Engine is the final piece in the chain and should be used for binding application static dependencies.
 */
UCLASS()
class TENTACLE_API UDiEngineSubsystem : public UEngineSubsystem, public IDiContextInterface
{
	GENERATED_BODY()

public:
	// - USubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	// - IDIContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override { return *DiContainerGCd.DiContainer; };
	virtual const DI::FChainedDiContainer& GetDiContainer()  const override { return *DiContainerGCd.DiContainer; };
	// --

protected:
	UPROPERTY()
	FChainedDiContainerGCd DiContainerGCd;
};
