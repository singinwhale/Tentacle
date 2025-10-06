// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TentacleSettings.generated.h"

/**
 * Developer settings for the Tentacle DI System.
 */
UCLASS(Config=Game)
class TENTACLE_API UTentacleSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Set to true to allow the auto-creation of scope subsystems
	 * (GameInstance-, World-, LocalPlayer-, Engine-Subsystem)
	 */
	UPROPERTY(EditAnywhere, Config, Category="Dependency Injection")
	bool bEnableScopeSubsystems = false;
	
	UPROPERTY(EditAnywhere, Config, Category="Dependency Injection", meta = (EditCondition = bEnableScopeSubsystems))
	bool bEnableDefaultChaining = false;
};
