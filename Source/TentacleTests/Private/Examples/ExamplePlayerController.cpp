// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "ExamplePlayerController.h"

#include "Contexts/DiLocalPlayerSubsystem.h"
#include "Contexts/DiWorldSubsystem.h"

AExamplePlayerController::AExamplePlayerController()
{
	DiContextComponent = CreateDefaultSubobject<UDiContextComponent>("DiContextComponent");
}

void AExamplePlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(this);
	TScriptInterface<IDiContextInterface> WorldDiContext = DI::TryFindDiContext(World);
	ForkingDiContainer->AddParentContainer(WorldDiContext->GetDiContainer().AsShared(), 0);
	
	DiContextComponent->SetAsParentOnAllComponentsOf(this);
}

void AExamplePlayerController::ReceivedPlayer()
{
	if (class ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (TScriptInterface<IDiContextInterface> LocalPlayerDiContext = DI::TryFindDiContext(LocalPlayer))
		{
			TSharedRef<DI::FChainedDiContainer> LocalPlayerContainer = LocalPlayerDiContext->GetDiContainer().AsShared();
			ForkingDiContainer->AddParentContainer(LocalPlayerContainer, 1);
		}
	}
}

void AExamplePlayerController::OnNetCleanup(class UNetConnection* Connection)
{
	TryRemoveDiChildFromLocalPlayerContainer();
	Super::OnNetCleanup(Connection);
}

void AExamplePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TryRemoveDiChildFromLocalPlayerContainer();
	Super::EndPlay(EndPlayReason);
}

void AExamplePlayerController::SetPawn(APawn* InPawn)
{
	if (TScriptInterface<IDiContextInterface> PawnDiContext = DI::TryGetLocalDiContext(GetPawn()))
	{
		PawnDiContext->GetDiContainer().SetParentContainer(nullptr);
	}

	Super::SetPawn(InPawn);

	if (TScriptInterface<IDiContextInterface> PawnDiContext = DI::TryGetLocalDiContext(GetPawn()))
	{
		PawnDiContext->GetDiContainer().SetParentContainer(GetDiContainer().AsShared());
	}
}


void AExamplePlayerController::TryRemoveDiChildFromLocalPlayerContainer()
{
	if (class ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (TScriptInterface<IDiContextInterface> LocalPlayerDiContext = DI::TryFindDiContext(LocalPlayer))
		{
			TSharedRef<DI::FChainedDiContainer> LocalPlayerContainer = LocalPlayerDiContext->GetDiContainer().AsShared();
			ForkingDiContainer->RemoveParentContainer(LocalPlayerContainer);
		}
	}
}