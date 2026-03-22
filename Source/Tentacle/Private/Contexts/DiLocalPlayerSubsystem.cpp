// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Contexts/DiLocalPlayerSubsystem.h"

#include "TentacleSettings.h"
#include "Contexts/DiGameInstanceSubsystem.h"

bool UDiLocalPlayerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return GetDefault<UTentacleSettings>()->bEnableScopeSubsystems;
}

void UDiLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	PlayerControllerChanged(LocalPlayer->GetPlayerController(nullptr));

	if (!GetDefault<UTentacleSettings>()->bEnableDefaultChaining)
		return;

	UDiGameInstanceSubsystem* DiGameInstanceSubsystem = GetLocalPlayerChecked()->GetGameInstance()->GetSubsystem<UDiGameInstanceSubsystem>();
	check(DiGameInstanceSubsystem);
	DiContainerGCd->SetParentContainer(DiGameInstanceSubsystem->GetDiContainer().AsShared());
}

void UDiLocalPlayerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	if (!GetDefault<UTentacleSettings>()->bEnableDefaultChaining)
		return;
	//TODO player controller must multiplex world and local player
}
