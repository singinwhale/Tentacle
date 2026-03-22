// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Nodes/K2Node_BindObjectAsType.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "KismetCompiler.h"
#include "Contexts/DiBlueprintFunctionLibrary.h"

void UK2Node_BindObjectAsType::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);
	
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic([](UEdGraphNode* NewNode, bool)
		{			
			UK2Node_BindObjectAsType* Node = CastChecked<UK2Node_BindObjectAsType>(NewNode);
			UFunction* Function = UDiBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UDiBlueprintFunctionLibrary, BindObjectAsType));
			check(Function);
			Node->SetFromFunction(Function);
		});
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UK2Node_BindObjectAsType::PostReconstructNode()
{
	Super::PostReconstructNode();
	ConformPinTypes();
}

void UK2Node_BindObjectAsType::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	ConformPinTypes();
}

void UK2Node_BindObjectAsType::ConformPinTypes()
{
	UEdGraphPin* TypePin = FindPin(FName("ObjectBindingType"), EGPD_Input);
	UEdGraphPin* ObjectPin = FindPin(FName("Object"), EGPD_Input);
	if (TypePin->HasAnyConnections())
	{
		ObjectPin->PinType.PinSubCategoryObject = TypePin->LinkedTo[0]->PinType.PinSubCategoryObject;
	}
	else
	{
		ObjectPin->PinType.PinSubCategoryObject = TypePin->DefaultObject;
	}
}

void UK2Node_BindObjectAsType::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);
	ConformPinTypes();
}

void UK2Node_BindObjectAsType::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{	
	UEdGraphPin* TypePin = FindPin(FName("ObjectBindingType"), EGPD_Input);
	UEdGraphPin* ObjectPin = FindPin(FName("Object"), EGPD_Input);
	UClass* MinimalClass = Cast<UClass>(TypePin->HasAnyConnections() ? TypePin->LinkedTo[0]->PinType.PinSubCategoryObject : TypePin->DefaultObject);
	if (!MinimalClass)
	{
		CompilerContext.MessageLog.Error(TEXT("@@ must not be None"), TypePin);
		return;
	}
	
	if (!ObjectPin->HasAnyConnections())
	{
		CompilerContext.MessageLog.Error(TEXT("@@ must not be None"), ObjectPin);
		return;
	}

	TWeakObjectPtr<UObject> ConnectedObjectPinType = ObjectPin->LinkedTo[0]->PinType.PinSubCategoryObject;
	if (ConnectedObjectPinType.IsValid() && !Cast<UClass>(ConnectedObjectPinType.Get())->IsChildOf(MinimalClass))
	{
		CompilerContext.MessageLog.Error(TEXT("@@ is not compatible with %s"), ObjectPin, *MinimalClass->GetName());
		return;
	}
	
	ObjectPin->PinType.PinSubCategoryObject = UObject::StaticClass();
	
	Super::ExpandNode(CompilerContext, SourceGraph);
}

