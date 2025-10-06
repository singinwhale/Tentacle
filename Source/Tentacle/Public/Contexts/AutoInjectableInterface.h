// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AutoInjectableInterface.generated.h"

class IDiContextInterface;
UINTERFACE(Blueprintable)
class TENTACLE_API UAutoInjectableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface on components that require auto initialization.
 * If you are not sure if someone will call AutoInject on you already, you can use IAutoInjector to request initialization from a supporting context.
 */
class TENTACLE_API IAutoInjectableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void AutoInject(const TScriptInterface<IDiContextInterface>& DiContext);
	virtual void AutoInject_Implementation(const TScriptInterface<IDiContextInterface>& DiContext) {}
};

namespace DI
{
	/**
	 * Tries to walk up the ownership chain to find an object that implements IAutoInjector that can be requested to initialize AutoInjectableObject.
	 * @param AutoInjectableObject The injectable to be initialized
	 * @return true if an auto injector has been found
	 */
	TENTACLE_API bool RequestAutoInject(TScriptInterface<IAutoInjectableInterface> AutoInjectableObject);
	TENTACLE_API bool TryAutoInject(TScriptInterface<IDiContextInterface> DiContext, UObject* MaybeAutoInjectableObject);
}