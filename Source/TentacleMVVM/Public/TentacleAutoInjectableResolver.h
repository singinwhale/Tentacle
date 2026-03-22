// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"

#include "TentacleAutoInjectableResolver.generated.h"

/**
 * ViewModel resolver that uses the Tentacle DI IAutoInjectable Framework to initialize a ViewModel when it is created.
 * @note Does not take care of Deinitialization.
 */
UCLASS()
class TENTACLEMVVM_API UTentacleAutoInjectableResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
#if WITH_EDITOR
	virtual bool DoesSupportViewModelClass(const UClass* Class) const override;
#endif
};
