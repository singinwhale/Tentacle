// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Contexts/AutoInjector.h"


void IAutoInjector::RequestInitialize(const TScriptInterface<IAutoInjectableInterface>& InitializationTarget)
{
	if (ensureAlwaysMsgf(InitializationTarget.GetObject() && InitializationTarget.GetObject()->Implements<UAutoInjectableInterface>(), TEXT("Invalid Target")))
	{
		TScriptInterface<IDiContextInterface> DiContext = CastChecked<UObject>(this);
		IAutoInjectableInterface::Execute_AutoInject(InitializationTarget.GetObject(), DiContext);
	}
}
