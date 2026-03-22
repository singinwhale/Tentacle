// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_AddPinInterface.h"
#include "UObject/Object.h"
#include "K2Node_ResolveMany.generated.h"

USTRUCT()
struct FK2Node_ResolveMany_BindingData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsStructBinding = false;

	UPROPERTY()
	bool bHasBindingName = false;

	UPROPERTY()
	int32 ID = 0;
};


/**
 * Resolve many different kind of bindings from a Dependency Injection Context.
 */
UCLASS()
class TENTACLEEDITOR_API UK2Node_ResolveMany : public UK2Node, public IK2Node_AddPinInterface
{
	GENERATED_BODY()

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual void PostReconstructNode() override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual bool IsNodePure() const override { return false; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	//~ End UK2Node Interface

	// - K2Node_AddPinInterface
	virtual void AddInputPin() override;
	virtual void RemoveInputPin(UEdGraphPin* Pin) override;
	virtual bool CanRemovePin(const UEdGraphPin* Pin) const override;
	// --
private:

	void UpdateOutputPinFromInputPin(UEdGraphPin* Pin);
	void AddObjectBinding(bool bHasName);
	int32 GetNewPinID() const;
	void AddStructBinding(bool bHasName);
	void AddDefaultObjectBinding();
	void AddDefaultStructBinding();
	void AddBindingPins(int32 ID, bool bHasName, FName InputPinCategory, FName OutputPinCategory, UObject* InputPinSubcategory);
	void AddBindingNamePin(UEdGraphPin* InputPin);
	void RemoveBindingNamePin(UEdGraphPin* NameInputPin);
	void ConvertToStructDependency(UEdGraphPin* InputPin);
	void ConvertToObjectDependency(UEdGraphPin* InputPin);
	UEdGraphPin* FindInputPinByDependencyIndex(int32 DependencyIndex) const;
	TArray<UEdGraphPin*> GetAllInputDependencyPins() const;
	int32 FindDependencyIndexByPin(UEdGraphPin* InputPin) const;
	UEdGraphPin* FindInputPin(UEdGraphPin* Pin) const;
	UEdGraphPin* FindOutputPin(UEdGraphPin* InputPin) const;
	UEdGraphPin* FindBindingNamePin(UEdGraphPin* InputPin) const;
	UEdGraphPin* FindStructBindingValidPin(UEdGraphPin* InputPin) const;
	UEdGraphPin* FindDiContextPin() const;
	static bool IsBindingInputPin(UEdGraphPin* InputPin);

	UPROPERTY()
	TArray<FK2Node_ResolveMany_BindingData> Bindings;

	static UScriptStruct*const DefaultStructClass;
	static UClass*const DefaultObjectClass;
	static const TCHAR*const PN_DiContext;
	static const TCHAR*const PN_BindingType;
	static const TCHAR*const PN_Binding;
	static const TCHAR*const PN_BindingName;
	static const TCHAR*const PN_BindingIsValid;
};
