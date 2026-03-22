// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

TENTACLE_API DECLARE_LOG_CATEGORY_EXTERN(LogDependencyInjection, Log, All);

class FTentacleModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
