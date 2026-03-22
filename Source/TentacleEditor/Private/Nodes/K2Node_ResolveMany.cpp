// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#include "Nodes/K2Node_ResolveMany.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Knot.h"
#include "KismetCompiler.h"
#include "Contexts/DiBlueprintFunctionLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ToolMenu.h"
#include "ScopedTransaction.h"

UScriptStruct* const UK2Node_ResolveMany::DefaultStructClass = nullptr;
UClass* const UK2Node_ResolveMany::DefaultObjectClass = UObject::StaticClass();

const TCHAR* const UK2Node_ResolveMany::PN_DiContext = TEXT("DiContext");
const TCHAR* const UK2Node_ResolveMany::PN_BindingType = TEXT("BindingType");
const TCHAR* const UK2Node_ResolveMany::PN_Binding = TEXT("Binding");
const TCHAR* const UK2Node_ResolveMany::PN_BindingName = TEXT("BindingName");
const TCHAR* const UK2Node_ResolveMany::PN_BindingIsValid = TEXT("BindingIsValid");

void UK2Node_ResolveMany::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();


	// exec pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Interface, UDiContextInterface::StaticClass(), PN_DiContext);

	TArray<FK2Node_ResolveMany_BindingData> PreviousDependencies = MoveTemp(Bindings);
	for (const FK2Node_ResolveMany_BindingData& Binding : PreviousDependencies)
	{
		if (Binding.bIsStructBinding)
		{
			AddStructBinding(Binding.bHasBindingName);
		}
		else
		{
			AddObjectBinding(Binding.bHasBindingName);
		}
	}
	if (Bindings.IsEmpty())
	{
		AddInputPin();
	}
}

FText UK2Node_ResolveMany::GetTooltipText() const
{
	return INVTEXT("Resolve many different kind of bindings from a Dependency Injection Context");
}

FText UK2Node_ResolveMany::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return INVTEXT("Resolve Many");
}

FSlateIcon UK2Node_ResolveMany::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::FromSRGBColor(FColor::Magenta);
	return FSlateIcon();
}

void UK2Node_ResolveMany::UpdateOutputPinFromInputPin(UEdGraphPin* Pin)
{
	check(IsBindingInputPin(Pin));

	UEdGraphPin* MatchingOutputPin = FindOutputPin(Pin);
	MatchingOutputPin->Modify();
	int32 BindingIndex = FindDependencyIndexByPin(Pin);
	if (!Bindings.IsValidIndex(BindingIndex))
	{
		RemoveInputPin(Pin);
		return;
	}
	FK2Node_ResolveMany_BindingData& Binding = Bindings[BindingIndex];
	check(Bindings.IsValidIndex(BindingIndex));

	FString BindingName = FString();
	if (UEdGraphPin* BindingNamePin = FindBindingNamePin(Pin))
	{
		if (BindingNamePin->HasAnyConnections())
		{
			BindingName = TEXT(" (Dyn)");
		}
		else if (!BindingNamePin->DefaultValue.IsEmpty())
		{
			BindingName = FString::Printf(TEXT(" \"%s\""), *BindingNamePin->DefaultValue);
		}
	}

	if (Pin->HasAnyConnections())
	{
		MatchingOutputPin->PinType.PinSubCategoryObject = Pin->LinkedTo[0]->PinType.PinSubCategoryObject;
		MatchingOutputPin->PinFriendlyName = FText::FromString(
			FString::Printf(TEXT("%i %s%s"), Pin->PinName.GetNumber(), *MatchingOutputPin->PinType.PinSubCategoryObject->GetName(), *BindingName)
		);
	}
	else if (Pin->DefaultObject)
	{
		MatchingOutputPin->PinType.PinSubCategoryObject = Pin->DefaultObject;
		MatchingOutputPin->PinFriendlyName = FText::FromString(
			FString::Printf(TEXT("%i %s%s"), Pin->PinName.GetNumber(), *MatchingOutputPin->PinType.PinSubCategoryObject->GetName(), *BindingName)
		);
	}
	else
	{
		if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
		{
			MatchingOutputPin->PinType.PinSubCategoryObject = DefaultObjectClass;
		}
		else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
		{
			MatchingOutputPin->PinType.PinSubCategoryObject = DefaultStructClass;
		}
		MatchingOutputPin->PinFriendlyName = INVTEXT("None");
	}
}

void UK2Node_ResolveMany::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if (!IsBindingInputPin(Pin))
	{
		if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
		{
			Pin = FindInputPin(Pin);
		}
		else
		{
			Pin = nullptr;
		}
	}
	if (Pin)
	{
		UpdateOutputPinFromInputPin(Pin);
	}
}

void UK2Node_ResolveMany::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (!IsBindingInputPin(Pin))
	{
		if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
		{
			Pin = FindInputPin(Pin);
		}
		else
		{
			Pin = nullptr;
		}
	}
	if (Pin)
	{
		UpdateOutputPinFromInputPin(Pin);
	}
}

void UK2Node_ResolveMany::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	if (!Context->bIsDebugging)
	{
		static FName ResolveDependenciesNodeName = FName("UK2Node_ResolveMany");
		FText ResolveDependenciesNodeStr = INVTEXT("Resolve Dependencies Node");
		if (Context->Pin != NULL)
		{
			if (CanRemovePin(Context->Pin))
			{
				FToolMenuSection& Section = Menu->AddSection(ResolveDependenciesNodeName, ResolveDependenciesNodeStr);
				Section.AddMenuEntry(
					"RemovePin",
					INVTEXT("Remove Dependency"),
					INVTEXT("Remove this dependency"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateUObject(
							const_cast<UK2Node_ResolveMany*>(this),
							&UK2Node_ResolveMany::RemoveInputPin,
							const_cast<UEdGraphPin*>(Context->Pin)
						)
					)
				);

				if (Context->Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
				{
					Section.AddMenuEntry(
						"ConvertToObjectDependency",
						INVTEXT("Convert To Object Dependency"),
						INVTEXT("Convert the struct dependency to an object dependency"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateUObject(
								const_cast<UK2Node_ResolveMany*>(this),
								&UK2Node_ResolveMany::ConvertToObjectDependency,
								const_cast<UEdGraphPin*>(Context->Pin)
							)
						)
					);
				}

				if (Context->Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
				{
					Section.AddMenuEntry(
						"ConvertToStructDependency",
						INVTEXT("Convert To Struct Dependency"),
						INVTEXT("Convert the object dependency to a struct dependency"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateUObject(
								const_cast<UK2Node_ResolveMany*>(this),
								&UK2Node_ResolveMany::ConvertToStructDependency,
								const_cast<UEdGraphPin*>(Context->Pin)
							)
						)
					);
				}

				if (UEdGraphPin* BindingNamePin = FindBindingNamePin(const_cast<UEdGraphPin*>(Context->Pin)))
				{
					Section.AddMenuEntry(
						"RemoveBindingName",
						INVTEXT("Remove Binding Name"),
						INVTEXT("Remove the Binding Name from the dependency"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateUObject(
								const_cast<UK2Node_ResolveMany*>(this),
								&UK2Node_ResolveMany::RemoveBindingNamePin,
								BindingNamePin
							)
						)
					);
				}
				else
				{
					Section.AddMenuEntry(
						"AddBindingName",
						INVTEXT("Add Binding Name"),
						INVTEXT("Add A Binding Name to the dependency"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateUObject(
								const_cast<UK2Node_ResolveMany*>(this),
								&UK2Node_ResolveMany::AddBindingNamePin,
								const_cast<UEdGraphPin*>(Context->Pin)
							)
						)
					);
				}
			}
		}
		else if (CanAddPin())
		{
			FToolMenuSection& Section = Menu->AddSection(ResolveDependenciesNodeName, ResolveDependenciesNodeStr);
			Section.AddMenuEntry(
				"AddObjectPin",
				INVTEXT("Add Object Dependency"),
				INVTEXT("Add an object dependency input pin"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateUObject(
						const_cast<UK2Node_ResolveMany*>(this),
						&UK2Node_ResolveMany::AddDefaultObjectBinding
					)
				)
			);
			Section.AddMenuEntry(
				"AddStructPin",
				INVTEXT("Add Struct Dependency"),
				INVTEXT("Add a struct dependency input pin"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateUObject(
						const_cast<UK2Node_ResolveMany*>(this),
						&UK2Node_ResolveMany::AddDefaultStructBinding
					)
				)
			);
		}
	}
}

void UK2Node_ResolveMany::PostReconstructNode()
{
	Super::PostReconstructNode();

	for (UEdGraphPin* InputDependencyPin : GetAllInputDependencyPins())
	{
		UpdateOutputPinFromInputPin(InputDependencyPin);
	}
}

void UK2Node_ResolveMany::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UClass* BPFLClass = UDiBlueprintFunctionLibrary::StaticClass();
	const FName& ResolveStructFunctionName = GET_FUNCTION_NAME_CHECKED(UDiBlueprintFunctionLibrary, TryResolveStructCopy);
	const FName& ResolveObjectFunctionName = GET_FUNCTION_NAME_CHECKED(UDiBlueprintFunctionLibrary, TryResolveObject);

	UEdGraphPin* const DiContextPin = FindDiContextPin();
	UEdGraphPin* const MyExecPin = GetExecPin();
	UEdGraphPin* LastThenPin = nullptr;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction != EGPD_Input)
			continue;

		const bool bIsStructDependencyInputPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object && Pin->PinType.PinSubCategoryObject ==
			UScriptStruct::StaticClass();
		const bool bIsObjectDependencyInputPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class;

		if (!bIsObjectDependencyInputPin && !bIsStructDependencyInputPin)
			continue;

		UEdGraphPin* BindingTypeInputPin = Pin;
		int32 DependencyIndex = FindDependencyIndexByPin(BindingTypeInputPin);
		if (!Bindings.IsValidIndex(DependencyIndex))
		{
			CompilerContext.MessageLog.Error(TEXT("Invalid Dependency Data for pin @@. Please re-create the node."), BindingTypeInputPin);
			continue;
		}

		FK2Node_ResolveMany_BindingData& BindingData = Bindings[DependencyIndex];
		const bool bShouldHaveBindingName = BindingData.bHasBindingName;
		UEdGraphPin* const BindingNameInputPin = bShouldHaveBindingName ? FindBindingNamePin(BindingTypeInputPin) : nullptr;
		UEdGraphPin* const BindingOutputPin = FindOutputPin(BindingTypeInputPin);
		UEdGraphPin* const BindingIsValidPin = FindStructBindingValidPin(BindingTypeInputPin);

		if (!Pin->HasAnyConnections() && !Pin->DefaultObject)
		{
			CompilerContext.MessageLog.Error(TEXT("Invalid Class for Pin @@. Please provide a valid class."), BindingTypeInputPin);
			continue;
		}

		if ((bShouldHaveBindingName && !BindingNameInputPin) || !BindingOutputPin)
		{
			CompilerContext.MessageLog.Error(TEXT("Wrong Pin Data around pin @@. Please Refresh the node."), BindingTypeInputPin);
			continue;
		}

		if (bIsStructDependencyInputPin && !BindingIsValidPin)
		{
			CompilerContext.MessageLog.Error(TEXT("Missing valid pin for @@. Please Refresh the node."), BindingTypeInputPin);
			continue;
		}

		if (bIsObjectDependencyInputPin)
		{
			UK2Node_CallFunction* ResolveObjectFunctionNode = SourceGraph->CreateIntermediateNode<UK2Node_CallFunction>();
			ResolveObjectFunctionNode->FunctionReference.SetExternalMember(ResolveObjectFunctionName, BPFLClass);
			ResolveObjectFunctionNode->AllocateDefaultPins();
			CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ResolveObjectFunctionNode, this);

			UEdGraphPin* ObjectTypePin = ResolveObjectFunctionNode->FindPinChecked(TEXT("ObjectType"));
			UEdGraphPin* BindingNamePin = ResolveObjectFunctionNode->FindPinChecked(TEXT("BindingName"));
			UEdGraphPin* OutObjectPin = ResolveObjectFunctionNode->FindPinChecked(TEXT("ReturnValue"));
			UEdGraphPin* FuncDiContextPin = ResolveObjectFunctionNode->FindPinChecked(TEXT("DiContextInterface"));

			if (LastThenPin)
			{
				LastThenPin->MakeLinkTo(ResolveObjectFunctionNode->GetExecPin());
			}
			else
			{
				CompilerContext.MovePinLinksToIntermediate(*MyExecPin, *ResolveObjectFunctionNode->GetExecPin());
			}

			CompilerContext.CopyPinLinksToIntermediate(*BindingTypeInputPin, *ObjectTypePin);
			ResolveObjectFunctionNode->NotifyPinConnectionListChanged(ObjectTypePin);
			if (bShouldHaveBindingName)
			{
				CompilerContext.MovePinLinksToIntermediate(*BindingNameInputPin, *BindingNamePin);
			}
			CompilerContext.MovePinLinksToIntermediate(*BindingOutputPin, *OutObjectPin);
			ResolveObjectFunctionNode->NotifyPinConnectionListChanged(OutObjectPin);
			CompilerContext.CopyPinLinksToIntermediate(*DiContextPin, *FuncDiContextPin);
			LastThenPin = ResolveObjectFunctionNode->GetThenPin();
		}
		if (bIsStructDependencyInputPin)
		{
			UK2Node_CallFunction* ResolveStructFunctionNode = SourceGraph->CreateIntermediateNode<UK2Node_CallFunction>();
			ResolveStructFunctionNode->FunctionReference.SetExternalMember(ResolveStructFunctionName, BPFLClass);
			ResolveStructFunctionNode->AllocateDefaultPins();
			CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ResolveStructFunctionNode, this);

			UEdGraphPin* StructTypePin = ResolveStructFunctionNode->FindPinChecked(TEXT("StructType"));
			UEdGraphPin* BindingNamePin = ResolveStructFunctionNode->FindPinChecked(TEXT("BindingName"));
			UEdGraphPin* OutValuePin = ResolveStructFunctionNode->FindPinChecked(TEXT("OutStructData"));
			UEdGraphPin* FuncDiContextPin = ResolveStructFunctionNode->FindPinChecked(TEXT("DiContextInterface"));
			UEdGraphPin* FuncReturnValuePin = ResolveStructFunctionNode->FindPinChecked(TEXT("ReturnValue"));
			UEdGraphPin* ValidThenPin = ResolveStructFunctionNode->FindPinChecked(TEXT("Valid"));
			UEdGraphPin* NotValidThenPin = ResolveStructFunctionNode->FindPinChecked(TEXT("NotValid"));

			if (LastThenPin)
			{
				LastThenPin->MakeLinkTo(ResolveStructFunctionNode->GetExecPin());
			}
			else
			{
				CompilerContext.MovePinLinksToIntermediate(*MyExecPin, *ResolveStructFunctionNode->GetExecPin());
			}

			CompilerContext.MovePinLinksToIntermediate(*BindingTypeInputPin, *StructTypePin);
			if (bShouldHaveBindingName)
			{
				CompilerContext.MovePinLinksToIntermediate(*BindingNameInputPin, *BindingNamePin);
			}
			CompilerContext.MovePinLinksToIntermediate(*BindingOutputPin, *OutValuePin);
			ResolveStructFunctionNode->NotifyPinConnectionListChanged(OutValuePin);
			OutValuePin->PinType = BindingOutputPin->PinType;
			CompilerContext.CopyPinLinksToIntermediate(*DiContextPin, *FuncDiContextPin);
			CompilerContext.MovePinLinksToIntermediate(*BindingIsValidPin, *FuncReturnValuePin);

			UK2Node_Knot* ExecRerouteNode = SourceGraph->CreateIntermediateNode<UK2Node_Knot>();
			ExecRerouteNode->AllocateDefaultPins();
			CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ExecRerouteNode, this);
			ValidThenPin->MakeLinkTo(ExecRerouteNode->GetInputPin());
			NotValidThenPin->MakeLinkTo(ExecRerouteNode->GetInputPin());
			ExecRerouteNode->NotifyPinConnectionListChanged(ExecRerouteNode->GetInputPin());
			LastThenPin = ExecRerouteNode->GetOutputPin();
		}
	}
	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *LastThenPin);
}

void UK2Node_ResolveMany::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_ResolveMany::GetMenuCategory() const
{
	return INVTEXT("Dependency Injection");
}

void UK2Node_ResolveMany::AddObjectBinding(bool bHasName)
{
	FScopedTransaction Transaction(INVTEXT("Add Object Dependency Pin"));
	Modify();

	int32 NewPinID = GetNewPinID();
	Bindings.Emplace(false, bHasName, NewPinID);
	AddBindingPins(NewPinID, bHasName, UEdGraphSchema_K2::PC_Class, UEdGraphSchema_K2::PC_Object, UObject::StaticClass());
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

int32 UK2Node_ResolveMany::GetNewPinID() const
{
	if (Bindings.IsEmpty())
		return 0;

	return Bindings.Last().ID + 1;
}

void UK2Node_ResolveMany::AddStructBinding(bool bHasName)
{
	FScopedTransaction Transaction(INVTEXT("Add Struct Dependency Pin"));
	Modify();

	int32 NewPinID = GetNewPinID();
	Bindings.Emplace(true, bHasName, NewPinID);
	AddBindingPins(NewPinID, bHasName, UEdGraphSchema_K2::PC_Object, UEdGraphSchema_K2::PC_Struct, UScriptStruct::StaticClass());
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	UEdGraphPin* IsValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FName(PN_BindingIsValid, NewPinID));
	IsValidPin->PinFriendlyName = FText::FromString(FString::Printf(TEXT("%i IsValid"), NewPinID));
}

void UK2Node_ResolveMany::AddDefaultObjectBinding()
{
	AddObjectBinding(false);
}

void UK2Node_ResolveMany::AddDefaultStructBinding()
{
	AddStructBinding(false);
}

void UK2Node_ResolveMany::AddBindingPins(
	int32 ID,
	bool bHasName,
	FName InputPinCategory,
	FName OutputPinCategory,
	UObject* InputPinSubcategory)
{
	UEdGraphPin* InputPin = CreatePin(EGPD_Input, InputPinCategory, InputPinSubcategory, FName(PN_BindingType, ID));
	InputPin->PinFriendlyName = FText::FromString(FString::Printf(TEXT("%s %i"),PN_BindingType, ID));
	UEdGraphPin* OutputPin = CreatePin(EGPD_Output, OutputPinCategory, nullptr, FName(PN_Binding, ID));
	OutputPin->PinFriendlyName = FText::FromString(FString::Printf(TEXT("%i %s"),ID, PN_Binding));
	if (bHasName)
	{
		UEdGraphPin* BindingNameInputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, FName(PN_BindingName, ID));
		BindingNameInputPin->PinFriendlyName = FText::FromString(FString::Printf(TEXT("%s %i"),PN_BindingName, ID));
	}
}

void UK2Node_ResolveMany::AddBindingNamePin(UEdGraphPin* InputPin)
{
	FScopedTransaction Transaction(INVTEXT("Add Binding Name Pin"));
	Modify();
	int32 BindingIndex = FindDependencyIndexByPin(InputPin);
	FCreatePinParams PinParams;
	PinParams.Index = Pins.Find(InputPin) + 1;
	int32 PinID = InputPin->PinName.GetNumber();
	UEdGraphPin* BindingNameInputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, FName(PN_BindingName, PinID), PinParams);
	BindingNameInputPin->PinFriendlyName = FText::FromString(FString::Printf(TEXT("%s %i"), PN_BindingName, PinID));
	Bindings[BindingIndex].bHasBindingName = true;
	UpdateOutputPinFromInputPin(InputPin);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UK2Node_ResolveMany::RemoveBindingNamePin(UEdGraphPin* NameInputPin)
{
	check(NameInputPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name);
	FScopedTransaction Transaction(INVTEXT("Remove Binding Name Pin"));
	Modify();
	int32 BindingIndex = FindDependencyIndexByPin(NameInputPin);
	Bindings[BindingIndex].bHasBindingName = false;
	RemovePin(NameInputPin);
	UpdateOutputPinFromInputPin(FindInputPin(NameInputPin));
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UK2Node_ResolveMany::ConvertToStructDependency(UEdGraphPin* InputPin)
{
	int32 DependencyIndex = FindDependencyIndexByPin(InputPin);
	check(DependencyIndex != INDEX_NONE);

	FScopedTransaction Transaction(INVTEXT("Convert Pin to Struct Dependency"));
	InputPin->Modify();
	InputPin->PinType = FEdGraphPinType(
		UEdGraphSchema_K2::PC_Object,
		NAME_None,
		UScriptStruct::StaticClass(),
		EPinContainerType::None,
		false,
		FEdGraphTerminalType()
	);
	UEdGraphPin* OutputPin = FindOutputPin(InputPin);
	OutputPin->Modify();
	OutputPin->PinType = FEdGraphPinType(
		UEdGraphSchema_K2::PC_Struct,
		NAME_None,
		DefaultStructClass,
		EPinContainerType::None,
		false,
		FEdGraphTerminalType()
	);
	OutputPin->DefaultObject = DefaultStructClass;

	FCreatePinParams PinParams;
	PinParams.Index = Pins.Find(OutputPin) + 1;
	if (UEdGraphPin* BindingNamePin = FindBindingNamePin(InputPin))
	{
		PinParams.Index = Pins.Find(BindingNamePin) + 1;
	}
	UEdGraphPin* IsValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FName(PN_BindingIsValid, InputPin->PinName.GetNumber()), PinParams);
	IsValidPin->PinFriendlyName = FText::FromString(FString::Printf(TEXT("%i IsValid"), InputPin->PinName.GetNumber()));

	Bindings[DependencyIndex].bIsStructBinding = true;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UK2Node_ResolveMany::ConvertToObjectDependency(UEdGraphPin* InputPin)
{
	int32 DependencyIndex = FindDependencyIndexByPin(InputPin);
	check(DependencyIndex != INDEX_NONE);

	FScopedTransaction Transaction(INVTEXT("Convert Pin to Object Dependency"));
	InputPin->Modify();
	InputPin->PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Class, NAME_None, DefaultObjectClass, EPinContainerType::None, false, FEdGraphTerminalType());
	UEdGraphPin* OutputPin = FindOutputPin(InputPin);
	OutputPin->Modify();
	InputPin->PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Object, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
	Bindings[DependencyIndex].bIsStructBinding = false;

	if (UEdGraphPin* IsValidPin = FindStructBindingValidPin(InputPin))
	{
		RemovePin(IsValidPin);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

TArray<UEdGraphPin*> UK2Node_ResolveMany::GetAllInputDependencyPins() const
{
	return Pins.FilterByPredicate(
		[](const UEdGraphPin* Pin)
		{
			return Pin->Direction == EGPD_Input
				&& (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object);
		}
	);
}

UEdGraphPin* UK2Node_ResolveMany::FindInputPinByDependencyIndex(int32 DependencyIndex) const
{
	return FindPin(FName(PN_BindingType, DependencyIndex));
}

int32 UK2Node_ResolveMany::FindDependencyIndexByPin(UEdGraphPin* InputPin) const
{
	int32 PinID = InputPin->PinName.GetNumber();
	return Bindings.IndexOfByPredicate([PinID](const auto& Binding)
	{
		return Binding.ID == PinID;
	});
}

UEdGraphPin* UK2Node_ResolveMany::FindInputPin(UEdGraphPin* Pin) const
{
	return FindPin(FName(PN_BindingType, Pin->PinName.GetNumber()));
}

void UK2Node_ResolveMany::AddInputPin()
{
	AddDefaultObjectBinding();
}

void UK2Node_ResolveMany::RemoveInputPin(UEdGraphPin* Pin)
{
	FScopedTransaction Transaction(INVTEXT("Remove Dependency Pin"));
	Modify();
	int32 DependencyIndex = FindDependencyIndexByPin(Pin);
	if (ensure(Bindings.IsValidIndex(DependencyIndex)))
	{
		Bindings.RemoveAt(DependencyIndex);
	}
	UEdGraphPin* BindingNamePin = FindBindingNamePin(Pin);
	UEdGraphPin* OutputPin = FindOutputPin(Pin);
	UEdGraphPin* IsValidPin = FindStructBindingValidPin(Pin);
	RemovePin(Pin);
	if (BindingNamePin)
	{
		RemovePin(BindingNamePin);
	}
	if (OutputPin)
	{
		RemovePin(OutputPin);
	}
	if (IsValidPin)
	{
		RemovePin(IsValidPin);
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

bool UK2Node_ResolveMany::CanRemovePin(const UEdGraphPin* Pin) const
{
	return Pin
		&& Pin->Direction == EGPD_Input
		&& (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class
			|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object
			|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name);
}

UEdGraphPin* UK2Node_ResolveMany::FindOutputPin(UEdGraphPin* InputPin) const
{
	return FindPin(FName(PN_Binding, InputPin->PinName.GetNumber()));
}

UEdGraphPin* UK2Node_ResolveMany::FindBindingNamePin(UEdGraphPin* InputPin) const
{
	return FindPin(FName(PN_BindingName, InputPin->PinName.GetNumber()));
}

UEdGraphPin* UK2Node_ResolveMany::FindStructBindingValidPin(UEdGraphPin* InputPin) const
{
	return FindPin(FName(PN_BindingIsValid, InputPin->PinName.GetNumber()));
}

UEdGraphPin* UK2Node_ResolveMany::FindDiContextPin() const
{
	return *Pins.FindByPredicate(
		[](UEdGraphPin* Pin)
		{
			return Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Interface
				&& Pin->PinType.PinSubCategoryObject == UDiContextInterface::StaticClass();
		}
	);
}

bool UK2Node_ResolveMany::IsBindingInputPin(UEdGraphPin* InputPin)
{
	if (InputPin->Direction != EGPD_Input)
		return false;

	if (InputPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Class && InputPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Object)
		return false;

	return true;
}
