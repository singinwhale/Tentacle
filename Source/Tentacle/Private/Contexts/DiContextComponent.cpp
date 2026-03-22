// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Contexts/DiContextComponent.h"

#include "Contexts/DiWorldSubsystem.h"

UDiContextComponent::UDiContextComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UDiContextComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (bRegisterWorldAsParent)
	{
		if (UDiWorldSubsystem* DiWorldSubsystem = UDiWorldSubsystem::TryGet(this))
		{
			GetDiContainer().SetParentContainer(DiWorldSubsystem->GetDiContainer().AsShared());
		}
	}
}

void UDiContextComponent::SetAsParentOnAllComponentsOf(AActor* Actor) const
{
	if (!Actor)
		return;

	Actor->ForEachComponent(
		false,
		[this](UActorComponent* ActorComponent)
		{
			if (ActorComponent == this)
				return;

			if (IDiContextInterface* DiContext = Cast<IDiContextInterface>(ActorComponent))
			{
				DiContext->GetDiContainer().SetParentContainer(DiContainer);
			}
		}
	);
}

void UDiContextComponent::AddReferencedObjects(UObject* Self, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(Self, Collector);
	static_cast<UDiContextComponent*>(Self)->DiContainer->AddReferencedObjects(Collector);
}

