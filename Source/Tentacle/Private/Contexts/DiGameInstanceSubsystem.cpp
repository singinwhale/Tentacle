// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Contexts/DiGameInstanceSubsystem.h"

#include "TentacleSettings.h"
#include "Contexts/DiEngineSubsystem.h"
#include "Engine/Engine.h"

void UDiGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!GetDefault<UTentacleSettings>()->bEnableDefaultChaining)
		return;

	if (!GEngine)
		return;

	UDiEngineSubsystem* DiEngineSubsystem = GEngine->GetEngineSubsystem<UDiEngineSubsystem>();
	DiContainerGCd->SetParentContainer(DiEngineSubsystem->GetDiContainer().AsShared());
}

bool UDiGameInstanceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return GetDefault<UTentacleSettings>()->bEnableScopeSubsystems;
}
