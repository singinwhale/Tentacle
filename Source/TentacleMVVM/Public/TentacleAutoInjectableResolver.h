// Fill out your copyright notice in the Description page of Project Settings.

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
	virtual UObject* CreateInstance(const UClass* ExpectedType, UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual bool DoesSupportViewModelClass(const UClass* Class) const override;
};
