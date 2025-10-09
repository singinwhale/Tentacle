// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Contexts/DiBlueprintFunctionLibrary.h"

#include "Blueprint/BlueprintExceptionInfo.h"
#include "Container/ChainedDiContainer.h"
#include "Contexts/AutoInjectableInterface.h"
#include "Contexts/DIContextInterface.h"


TScriptInterface<IDiContextInterface> UDiBlueprintFunctionLibrary::FindDiContextForObject(UObject* ContextObject)
{
	return DI::TryFindDiContext(ContextObject);
}

bool UDiBlueprintFunctionLibrary::RequestAutoInject(TScriptInterface<IAutoInjectableInterface> AutoInjectableObject, bool& bResult)
{
	if (!AutoInjectableObject.GetObject())
	{
		if (FFrame* StackFrame = FFrame::GetThreadLocalTopStackFrame())
		{
			FBlueprintExceptionInfo ExceptionInfo(
				EBlueprintExceptionType::AccessViolation,
				INVTEXT("Accessed None attempting to call RequestAutoInject.")
				);
			FBlueprintCoreDelegates::ThrowScriptException(StackFrame->Object, *StackFrame, ExceptionInfo);
		}
		return false;
	}

	bResult = DI::RequestAutoInject(AutoInjectableObject);
	return bResult;
}

UObject* UDiBlueprintFunctionLibrary::TryResolveObject(TScriptInterface<IDiContextInterface> DiContextInterface, UClass* ObjectType, FName BindingName)
{
	return DiContextInterface->DiResolve().TryGetUObjectByClass(ObjectType, BindingName, DI::EResolveErrorBehavior::LogError);
}

TScriptInterface<IInterface> UDiBlueprintFunctionLibrary::TryResolveInterface(TScriptInterface<IDiContextInterface> DiContextInterface, TSubclassOf<UInterface> InterfaceType, FName BindingName)
{
	checkNoEntry();
	return TScriptInterface<IInterface>();
}

bool UDiBlueprintFunctionLibrary::TryResolveStruct(
	TScriptInterface<IDiContextInterface> DiContextInterface,
	FName BindingName,
	int32& OutStructData,
	EStructUtilsResult& Result)
{
	checkNoEntry();
	return false;
}

bool UDiBlueprintFunctionLibrary::TryResolveStructCopy(
	TScriptInterface<IDiContextInterface> DiContextInterface,
	UScriptStruct* StructType,
	FName BindingName,
	int32& OutStructData,
	EStructUtilsResult& Result)
{
	checkNoEntry();
	return false;
}

void UDiBlueprintFunctionLibrary::BindObject(
	TScriptInterface<IDiContextInterface> DiContextInterface,
	UObject* Object,
	FName BindingName)
{
	if (!DiContextInterface)
	{
		if (FFrame* StackFrame = FFrame::GetThreadLocalTopStackFrame())
		{
			FBlueprintExceptionInfo ExceptionInfo(
				EBlueprintExceptionType::AccessViolation,
				INVTEXT("Accessed None DiContextInterface attempting to call BindObject.")
			);
			FBlueprintCoreDelegates::ThrowScriptException(StackFrame->Object, *StackFrame, ExceptionInfo);
		}
		return;
	}
	BindObjectAsType(DiContextInterface, Object, Object->GetClass(), BindingName);
}

void UDiBlueprintFunctionLibrary::BindStruct(TScriptInterface<IDiContextInterface> DiContextInterface, FName BindingName, int32 StructData)
{
	checkNoEntry();
}

void UDiBlueprintFunctionLibrary::BindObjectAsType(
	TScriptInterface<IDiContextInterface> DiContextInterface,
	UObject* Object,
	UClass* ObjectBindingType,
	FName BindingName)
{
	FFrame* Stack = FFrame::GetThreadLocalTopStackFrame();
	if (!DiContextInterface)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			INVTEXT("DiContextInterface is invalid")
		);

		FBlueprintCoreDelegates::ThrowScriptException(Stack->Object, *Stack, ExceptionInfo);
		return;
	}

	if (!Object)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			INVTEXT("Object is invalid")
		);

		FBlueprintCoreDelegates::ThrowScriptException(Stack->Object, *Stack, ExceptionInfo);
		return;
	}

	if (!ObjectBindingType)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			INVTEXT("ObjectBindingType is invalid")
		);

		FBlueprintCoreDelegates::ThrowScriptException(Stack->Object, *Stack, ExceptionInfo);
		return;
	}

	if (!Object->IsA(ObjectBindingType) && !Object->GetClass()->ImplementsInterface(ObjectBindingType))
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Object is not derived from ObjectBindingType")
		);

		FBlueprintCoreDelegates::ThrowScriptException(Stack->Object, *Stack, ExceptionInfo);
		return;
	}

	DI::FChainedDiContainer& DiContainer = DiContextInterface->GetDiContainer();
	TSharedPtr<DI::FBinding> Binding;
	if (ObjectBindingType->HasAnyClassFlags(CLASS_Interface))
	{
		Binding = MakeShared<DI::FUInterfaceBinding>(
			DI::FBindingId(DI::FTypeId(ObjectBindingType), BindingName),
			FScriptInterface(Object, Object->GetNativeInterfaceAddress(ObjectBindingType))
		);
	}
	else
	{
		Binding = MakeShared<DI::TUObjectBinding<UObject>>(
			DI::FBindingId(DI::FTypeId(ObjectBindingType), BindingName),
			Object
		);
	}
	DI::EBindResult Result = DiContainer.BindSpecific(Binding.ToSharedRef(), DI::EBindConflictBehavior::BlueprintException);
}

DEFINE_FUNCTION(UDiBlueprintFunctionLibrary::execTryResolveStruct)
{
	P_GET_TINTERFACE(IDiContextInterface, DiContextInterface);
	P_GET_PROPERTY(FNameProperty, BindingName);

	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);

	const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_GET_ENUM_REF(EStructUtilsResult, Result);

	P_FINISH;

	if (!ValueProp || !ValuePtr || !DiContextInterface)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Failed to resolve the Value for Struct")
		);

		FBlueprintCoreDelegates::ThrowScriptException(Stack.Object, Stack, ExceptionInfo);

		P_NATIVE_BEGIN;
			Result = EStructUtilsResult::NotValid;
			(*static_cast<bool*>(RESULT_PARAM)) = false;
		P_NATIVE_END;
	}
	else
	{
		P_NATIVE_BEGIN;
			bool bResult = DiContextInterface->DiResolve().TryGetUStruct(
				ValueProp->Struct,
				ValuePtr,
				BindingName
			);
			Result = bResult ? EStructUtilsResult::Valid : EStructUtilsResult::NotValid;
			(*static_cast<bool*>(RESULT_PARAM)) = bResult;
		P_NATIVE_END;
	}
}

DEFINE_FUNCTION(UDiBlueprintFunctionLibrary::execTryResolveStructCopy)
{
	P_GET_TINTERFACE(IDiContextInterface, DiContextInterface);
	P_GET_OBJECTPTR(UScriptStruct, StructType);
	P_GET_PROPERTY(FNameProperty, BindingName);

	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);

	const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_GET_ENUM_REF(EStructUtilsResult, Result);

	P_FINISH;

	if (!ValueProp || !ValuePtr || !DiContextInterface || !StructType)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Failed to resolve the Value for Struct")
		);

		FBlueprintCoreDelegates::ThrowScriptException(Stack.Object, Stack, ExceptionInfo);

		P_NATIVE_BEGIN;
			Result = EStructUtilsResult::NotValid;
			(*static_cast<bool*>(RESULT_PARAM)) = false;
		P_NATIVE_END;
	}
	else
	{
		P_NATIVE_BEGIN;
			bool bResult = DiContextInterface->GetDiContainer()
			                                 .Resolve()
			                                 .TryGetUStruct(
				                                 StructType,
				                                 ValuePtr,
				                                 BindingName
			                                 );
			Result = bResult ? EStructUtilsResult::Valid : EStructUtilsResult::NotValid;
			(*static_cast<bool*>(RESULT_PARAM)) = bResult;
		P_NATIVE_END;
	}
}

DEFINE_FUNCTION(UDiBlueprintFunctionLibrary::execTryResolveInterface)
{
	P_GET_TINTERFACE(IDiContextInterface, DiContextInterface);
	P_GET_OBJECTPTR(UClass, InterfaceType);
	P_GET_PROPERTY(FNameProperty, BindingNameProperty);

	P_FINISH;

	if (!InterfaceType || !DiContextInterface)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Failed to resolve the Interface")
			);

		FBlueprintCoreDelegates::ThrowScriptException(Stack.Object, Stack, ExceptionInfo);

		P_NATIVE_BEGIN;
			(*static_cast<UObject**>(RESULT_PARAM)) = nullptr;
		P_NATIVE_END;
	}
	else
	{
		FName BindingName = BindingNameProperty;
		P_NATIVE_BEGIN;
			DI::FBindingId BindingId(DI::FTypeId(InterfaceType.Get()), BindingName);
			TSharedPtr<DI::FBinding> Binding = DiContextInterface->GetDiContainer().FindBinding(BindingId);
			if (!Binding)
			{
				UE_LOG(LogDependencyInjection, Error, TEXT("Failed to resolve Interface Binding %s"), *BindingId.ToString());
			}
			TSharedPtr<DI::FUInterfaceBinding> InterfaceBinding = StaticCastSharedPtr<DI::FUInterfaceBinding>(Binding);
			(*static_cast<UObject**>(RESULT_PARAM)) = InterfaceBinding ? InterfaceBinding->Resolve().GetObject() : nullptr;
		P_NATIVE_END;
	}
}

DEFINE_FUNCTION(UDiBlueprintFunctionLibrary::execBindStruct)
{
	P_GET_TINTERFACE(IDiContextInterface, DiContextInterface);
	P_GET_PROPERTY(FNameProperty, BindingName);

	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);

	const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	uint8* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	if (!ValueProp || !ValuePtr || !DiContextInterface)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Failed to resolve the Value for Struct")
		);

		FBlueprintCoreDelegates::ThrowScriptException(Stack.Object, Stack, ExceptionInfo);
	}
	else
	{
		P_NATIVE_BEGIN;
			{
				TSharedRef<DI::FRawDataBinding> StructDataBinding = MakeShared<DI::FUStructBinding>(ValueProp->Struct, BindingName, ValuePtr);
				DiContextInterface->GetDiContainer().BindSpecific(StructDataBinding, DI::EBindConflictBehavior::BlueprintException);
			}
		P_NATIVE_END;
	}
}
