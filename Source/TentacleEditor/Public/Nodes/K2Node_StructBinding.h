// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Editor/BlueprintGraph/Classes/K2Node_CallFunction.h"
#include "K2Node_StructBinding.generated.h"


UCLASS()
class TENTACLEEDITOR_API UK2Node_StructBinding : public UK2Node_CallFunction
{
	GENERATED_BODY()

	// UK2Node
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	// --
};
