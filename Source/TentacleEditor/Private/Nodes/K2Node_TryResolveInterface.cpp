// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Nodes/K2Node_TryResolveInterface.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Contexts/DiBlueprintFunctionLibrary.h"

void UK2Node_TryResolveInterface::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);
	
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic([](UEdGraphNode* NewNode, bool)
		{			
			UK2Node_TryResolveInterface* Node = CastChecked<UK2Node_TryResolveInterface>(NewNode);
			UFunction* Function = UDiBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UDiBlueprintFunctionLibrary, TryResolveInterface));
			check(Function);
			Node->SetFromFunction(Function);
		});
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}
