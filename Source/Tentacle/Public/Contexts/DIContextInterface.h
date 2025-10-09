// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "AutoInjectableInterface.h"
#include "Container/BindingHelper.h"
#include "Container/Injector.h"
#include "Container/ResolveHelper.h"
#include "UObject/Interface.h"
#include "DIContextInterface.generated.h"

namespace DI
{
	class FDiContainer;
	class FChainedDiContainer;
}


UINTERFACE(NotBlueprintable, BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class UDiContextInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface on UObjects that act as a Dependency Injection Context.
 * This Interface cannot be implemented in blueprint because BP cannot handle shared pointers.
 * To use DI Contexts in Blueprint see UDiContainerObject and UDiContextComponent.
 * @code
	// - IDiContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override { return *DiContainerGCd; }
	virtual const DI::FChainedDiContainer& GetDiContainer() const override { return *DiContainerGCd; }
	// --

	UPROPERTY()
	FChainedDiContainerGCd DiContainerGCd;
 * @endcode 
 */
class TENTACLE_API IDiContextInterface
{
	GENERATED_BODY()

public:
	virtual DI::FChainedDiContainer& GetDiContainer() = 0;
	virtual const DI::FChainedDiContainer& GetDiContainer() const = 0;

	/** @return The Resolve API for this context*/
	FORCEINLINE DI::TResolveHelper<DI::FChainedDiContainer> DiResolve() const
	{
		return DI::TResolveHelper(GetDiContainer());
	}
	/** @return The Bind API for this context*/
	FORCEINLINE DI::TBindingHelper<DI::FChainedDiContainer> DiBind()
	{
		return DI::TBindingHelper(GetDiContainer());
	}
	/** @return The Inject API for this context*/
	FORCEINLINE DI::TInjector<const DI::FChainedDiContainer> DiInject() const
	{
		return DI::TInjector(GetDiContainer());
	}
	/** @return The Inject API for this context*/
	FORCEINLINE DI::TInjector<DI::FChainedDiContainer> DiInject()
	{
		return DI::TInjector(GetDiContainer());
	}
};

namespace DI
{
	TENTACLE_API TScriptInterface<IDiContextInterface> TryFindDiContext(UObject* StartObject);
	TENTACLE_API TScriptInterface<IDiContextInterface> TryGetLocalDiContext(UObject* StartObject);
}
