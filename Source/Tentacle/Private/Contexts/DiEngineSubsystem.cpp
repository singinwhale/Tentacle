// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Contexts/DiEngineSubsystem.h"

#include "TentacleSettings.h"
#include "Contexts/DiGameInstanceSubsystem.h"

bool UDiEngineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return GetDefault<UTentacleSettings>()->bEnableScopeSubsystems;
}
