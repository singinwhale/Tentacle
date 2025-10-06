// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "K2Node_TryResolveInterface.generated.h"

/**
 * Custom function node for UDiBlueprintFunctionLibrary::TryResolveInterface
 */
UCLASS()
class TENTACLEEDITOR_API UK2Node_TryResolveInterface : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};
