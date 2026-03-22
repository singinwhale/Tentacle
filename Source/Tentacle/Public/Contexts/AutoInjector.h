// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "AutoInjectableInterface.h"
#include "DiContextInterface.h"
#include "UObject/Interface.h"
#include "AutoInjector.generated.h"

UINTERFACE(NotBlueprintable)
class TENTACLE_API UAutoInjector : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface if you want to allow objects in this context to request initialization from here.
 * This pattern is preferred over using the DiContext directly because the context parent can decide when and how the object is initialized.
 */
class TENTACLE_API IAutoInjector
{
	GENERATED_BODY()

public:
	/**
	 * Request to be initialized from this context.
	 * Implementers must call IAutoInjectable::AutoInject on the initialization target as soon as possible.
	 * @param InitializationTarget The object that wants AutoInject to be called as soon as possible.
	 */
	UFUNCTION(BlueprintCallable, Category="Dependency Injection")
	virtual void RequestInitialize(const TScriptInterface<IAutoInjectableInterface>& InitializationTarget);
};
