// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "TentacleSettings.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Contexts/DiContextComponent.h"
#include "Contexts/DIContextInterface.h"
#include "Contexts/DiEngineSubsystem.h"
#include "Contexts/DiGameInstanceSubsystem.h"
#include "Contexts/DiLocalPlayerSubsystem.h"
#include "Contexts/DiWorldSubsystem.h"

TScriptInterface<const IDiContextInterface> DI::TryFindDiContext(const UObject* StartObject)
{
	if (!StartObject)
		return nullptr;

	if (TScriptInterface<const IDiContextInterface> LocalDiContext = TryGetLocalDiContext(StartObject))
		return LocalDiContext;

	if (const UActorComponent* ActorComponent = Cast<UActorComponent>(StartObject))
	{
		if (AActor* OwnerActor = ActorComponent->GetOwner())
		{
			return TryFindDiContext(OwnerActor);
		}
	}

	
	if (const UWidget* Widget = Cast<UWidget>(StartObject))
	{
		if (Cast<UGameInstance>(Widget->GetOuter()))
		{
			return TryFindDiContext(Widget->GetOwningPlayer());
		}
	}

	if (UObject* Outer = StartObject->GetOuter())
	{
		return TryFindDiContext(Outer);
	}

	return nullptr;
}

TScriptInterface<IDiContextInterface> DI::TryFindDiContext(UObject* StartObject)
{
	return const_cast<UObject*>(TryFindDiContext(const_cast<const UObject*>(StartObject)).GetObject());
}

TScriptInterface<const IDiContextInterface> DI::TryGetLocalDiContext(const UObject* StartObject)
{
	if (!StartObject)
		return nullptr;

	if (StartObject->Implements<UDiContextInterface>())
		return TScriptInterface<const IDiContextInterface>(StartObject);

	if (const AActor* Actor = Cast<AActor>(StartObject))
	{
		if (UDiContextComponent* DiContextComponent = Actor->FindComponentByClass<UDiContextComponent>())
		{
			return DiContextComponent;
		}
	}

	if (GetDefault<UTentacleSettings>()->bEnableScopeSubsystems)
	{
		if (const UWorld* World = Cast<UWorld>(StartObject))
		{
			return World->GetSubsystem<UDiWorldSubsystem>();
		}
		if (const UGameInstance* GameInstance = Cast<UGameInstance>(StartObject))
		{
			return GameInstance->GetSubsystem<UDiGameInstanceSubsystem>();
		}
		if (const UEngine* Engine = Cast<UEngine>(StartObject))
		{
			return Engine->GetEngineSubsystem<UDiEngineSubsystem>();
		}
		if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(StartObject))
		{
			return LocalPlayer->GetSubsystem<UDiLocalPlayerSubsystem>();
		}
	}

	return nullptr;
}

TScriptInterface<IDiContextInterface> DI::TryGetLocalDiContext(UObject* StartObject)
{
	return const_cast<UObject*>(TryGetLocalDiContext(const_cast<const UObject*>(StartObject)).GetObject());
}
