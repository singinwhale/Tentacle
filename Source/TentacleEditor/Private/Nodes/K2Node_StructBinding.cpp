// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.
#include "Nodes/K2Node_StructBinding.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Contexts/DiBlueprintFunctionLibrary.h"
#include "Kismet/BlueprintInstancedStructLibrary.h"


#define LOCTEXT_NAMESPACE "TentacleEditor"

void UK2Node_StructBinding::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);
	UClass* Action = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(Action))
	{
		auto CustomizeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, const FName FunctionName)
		{
			UK2Node_StructBinding* Node = CastChecked<UK2Node_StructBinding>(NewNode);
			UFunction* Function = UDiBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(FunctionName);
			check(Function);
			Node->SetFromFunction(Function);
		};
		
		// TryResolveStruct()
		UBlueprintNodeSpawner* TryResolveStructSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(TryResolveStructSpawner != nullptr);
		TryResolveStructSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UDiBlueprintFunctionLibrary, TryResolveStruct));
		ActionRegistrar.AddBlueprintAction(Action, TryResolveStructSpawner);

		// TryResolveStructCopy()
		UBlueprintNodeSpawner* TryResolveStructCopySpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(TryResolveStructCopySpawner != nullptr);
		TryResolveStructCopySpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UDiBlueprintFunctionLibrary, TryResolveStructCopy));
		ActionRegistrar.AddBlueprintAction(Action, TryResolveStructCopySpawner);

		// BindStruct()
		UBlueprintNodeSpawner* BindStructSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(BindStructSpawner != nullptr);
		BindStructSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UDiBlueprintFunctionLibrary, BindStruct));
		ActionRegistrar.AddBlueprintAction(Action, BindStructSpawner);
	}
}

bool UK2Node_StructBinding::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	const UEdGraphPin* StructPin = FindPin(FName(TEXT("OutStructData")));
	if (!StructPin)
	{
		StructPin = FindPin(FName(TEXT("StructData")));
	}
	checkf(StructPin, TEXT("Expected all functions that are used with this node to have a struct data pin."))

	if (MyPin == StructPin && MyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		if (OtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Struct)
		{
			OutReason = TEXT("OutStructData must be a struct.");
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE