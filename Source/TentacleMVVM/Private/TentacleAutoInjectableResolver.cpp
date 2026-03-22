// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "TentacleAutoInjectableResolver.h"

#include "TentacleMVVM.h"
#include "Contexts/AutoInjectableInterface.h"
#include "Blueprint/UserWidget.h"
#include "Contexts/DiContextInterface.h"

UObject* UTentacleAutoInjectableResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UUserWidget* MutableUserWidget = const_cast<UUserWidget*>(UserWidget);
	UObject* Instance = NewObject<UObject>(MutableUserWidget, ExpectedType);
	if (TScriptInterface<IDiContextInterface> DiContextInterface = DI::TryFindDiContext(MutableUserWidget))
	{
		if (ensure(Instance)
			&& ensure(ExpectedType->ImplementsInterface(UAutoInjectableInterface::StaticClass())))
		{
			IAutoInjectableInterface::Execute_AutoInject(Instance, DiContextInterface);
		}
	}
	else
	{
		UE_LOG(LogTentacleMVVM, Error, TEXT("Could not find DI Context for Object %s"), *UserWidget->GetPathName());
	}
	return Instance;
}

#if WITH_EDITOR
bool UTentacleAutoInjectableResolver::DoesSupportViewModelClass(const UClass* Class) const
{
	return Class->ImplementsInterface(UAutoInjectableInterface::StaticClass());
}
#endif