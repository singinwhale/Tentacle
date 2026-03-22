// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Container/ChainedDiContainer.h"
#include "DiContextInterface.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "DiLocalPlayerSubsystem.generated.h"

/**
 * Di Context for Local Players.
 * Requires UTentacleSettings::bEnableScopeSubsystems to be enabled.
 * LocalPlayers chain to the game instance.
 * This context should be used for binding dependencies that are tied to a local player like player input, viewport clients etc.
 */
UCLASS()
class TENTACLE_API UDiLocalPlayerSubsystem : public ULocalPlayerSubsystem, public IDiContextInterface
{
	GENERATED_BODY()

public:
	// - USubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	// - ULocalPlayerSubsystem
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;
	// - IDIContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override { return *DiContainerGCd.DiContainer; };
	virtual const DI::FChainedDiContainer& GetDiContainer()  const override { return *DiContainerGCd.DiContainer; };
	// --

protected:
	UPROPERTY()
	FChainedDiContainerGCd DiContainerGCd;
};
