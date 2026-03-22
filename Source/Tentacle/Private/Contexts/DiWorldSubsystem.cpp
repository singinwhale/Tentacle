// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Contexts/DiWorldSubsystem.h"

#include "TentacleSettings.h"
#include "Contexts/DiGameInstanceSubsystem.h"

UDiWorldSubsystem* UDiWorldSubsystem::TryGet(const UObject* WorldContext)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;

	return World->GetSubsystem<UDiWorldSubsystem>();
}

bool UDiWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return GetDefault<UTentacleSettings>()->bEnableScopeSubsystems;
}

void UDiWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!GetDefault<UTentacleSettings>()->bEnableDefaultChaining)
		return;

	const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
		return;

	UDiGameInstanceSubsystem* DiGameInstanceSubsystem = GameInstance->GetSubsystem<UDiGameInstanceSubsystem>();
	DiContainerGCd->SetParentContainer(DiGameInstanceSubsystem->GetDiContainer().AsShared());
}
