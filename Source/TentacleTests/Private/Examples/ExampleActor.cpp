// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "ExampleActor.h"

void AExampleActor::ComponentRegistered(UActorComponent* Component)
{
	// uncomment if you have the engine mod
	// Super::ComponentRegistered(Component); 
	
	DI::TryAutoInject(this, Component);
}

DI::FChainedDiContainer& AExampleActor::GetDiContainer()
{
	return *DiContainer;
}

const DI::FChainedDiContainer& AExampleActor::GetDiContainer() const
{
	return *DiContainer;
}
