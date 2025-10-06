// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "K2Node_BindObjectAsType.generated.h"

/**
 * Custom Node for binding an object as a specific type.
 * Mostly used to adapt the pins depending on which kind of dependency is bound.
 */
UCLASS()
class TENTACLEEDITOR_API UK2Node_BindObjectAsType : public UK2Node_CallFunction
{
	GENERATED_BODY()
public:

	// - UK2Node
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void PostReconstructNode() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	// --
private:
	void ConformPinTypes();
};
