// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "AutoInjectableInterface.h"
#include "DiContextInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintInstancedStructLibrary.h"
#include "Templates/SubclassOf.h"

#include "DiBlueprintFunctionLibrary.generated.h"


/**
 * Exposes UObject related DI Functions to blueprint.
 * @see UK2Node_StructBinding
 * @see UK2Node_ResolveMany
 */
UCLASS()
class TENTACLE_API UDiBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Dependency Injection", meta=(DefaultToSelf="ContextObject"))
	static TScriptInterface<IDiContextInterface> FindDiContextForObject(UObject* ContextObject);

	/**
	 * Request auto injetion on a AutoInjectable.
	 * @param AutoInjectableObject the object to be initialized via IAutoInjectable::AutoInject
	 * @param bResult True if initialization has been requested. False if no IAutoInjector has been found.
	 * @return True if initialization has been requested. False if no IAutoInjector has been found.
	 */
	UFUNCTION(BlueprintCallable, Category="Dependency Injection", meta = (DefaultToSelf = "AutoInjectableObject", ExpandBoolAsExecs="bResult"))
	static bool RequestAutoInject(TScriptInterface<IAutoInjectableInterface> AutoInjectableObject, bool& bResult);


	/**
	 * Resolve a single object from the context.
	 * @param DiContextInterface The context that has the bindings
	 * @param ObjectType the UClass of the object to be resolved
	 * @param BindingName Name of the binding or None for a type binding
	 * @return The resolved object or None if the object is not bound.
	 */
	UFUNCTION(BlueprintCallable, Category="Dependency Injection", meta = (DeterminesOutputType = "ObjectType", DefaultToSelf = "DiContextInterface"))
	static UObject* TryResolveObject(TScriptInterface<IDiContextInterface> DiContextInterface, UClass* ObjectType, FName BindingName);

	/**
	 * Resolve a single interface instance from the context.
	 * @param DiContextInterface The context that has the bindings
	 * @param ObjectType the UClass of the object to be resolved
	 * @param BindingName Name of the binding or None for a type binding
	 * @return The resolved object or None if the object is not bound.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Dependency Injection", meta = (DeterminesOutputType = "InterfaceType", DefaultToSelf = "DiContextInterface", BlueprintInternalUseOnly=true))
	static TScriptInterface<IInterface> TryResolveInterface(TScriptInterface<IDiContextInterface> DiContextInterface, TSubclassOf<UInterface> InterfaceType, FName BindingName);


	/**
	 * Resolve a single UStruct instance from the context.
	 * @param DiContextInterface The context that has the bindings
	 * @param OutStructData the Struct Ref pin that the data will be written to.
	 * @param BindingName Name of the binding or None for a type binding
	 * @return The resolved object or None if the object is not bound.
	 */
	UFUNCTION(
		BlueprintCallable,
		CustomThunk,
		Category="Dependency Injection",
		meta=( BlueprintInternalUseOnly=true, CustomStructureParam="OutStructData", DefaultToSelf = "DiContextInterface", ReturnDisplayName="Is Valid",
			ExpandEnumAsExecs="Result" )
	)
	static bool TryResolveStruct(
		TScriptInterface<IDiContextInterface> DiContextInterface,
		FName BindingName,
		UPARAM(ref) int32& OutStructData,
		EStructUtilsResult& Result);


	/**
	 * Resolve StructData by providing a concrete script struct.
	 * @param DiContextInterface The context that has the bindings
	 * @param StructType UScriptStruct of the binding.
	 * @param BindingName Name of the binding or None for a type binding
	 * @param OutStructData Copy of the struct data that is bound for the binding ID.
	 * @param Result Valid if the binding was found, NotValid if the binding was not found.
	 * @return True if the binding was found, False if the binding was not found.
	 */
	UFUNCTION(
		BlueprintCallable,
		CustomThunk,
		meta=( BlueprintInternalUseOnly=true, CustomStructureParam="OutStructData", DefaultToSelf = "DiContextInterface", ReturnDisplayName="Is Valid",
			ExpandEnumAsExecs="Result" )
	)
	static bool TryResolveStructCopy(TScriptInterface<IDiContextInterface> DiContextInterface, UScriptStruct* StructType, FName BindingName, int32& OutStructData, EStructUtilsResult& Result);


	/**
	 * Bind an object into the DI Context.
	 * @param DiContextInterface The context that has the bindings
	 * @param Object The object to bind into the context.
	 * @param BindingName The name of the binding.
	 */
	UFUNCTION(BlueprintCallable, Category="Dependency Injection", meta = (DefaultToSelf = "DiContextInterface"))
	static void BindObject(TScriptInterface<IDiContextInterface> DiContextInterface, UObject* Object, FName BindingName);


	/**
	 * Bind a struct into the DI Context.
	 * @param DiContextInterface The context that has the bindings
	 * @param StructData The struct data to bind into the context.
	 * @param BindingName The name of the binding.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, meta=( BlueprintInternalUseOnly=true, CustomStructureParam="StructData", DefaultToSelf = "DiContextInterface"))
	static void BindStruct(TScriptInterface<IDiContextInterface> DiContextInterface, FName BindingName, int32 StructData);

	/**
	 * Bind an object into the DI Context by specific type.
	 * @param DiContextInterface The context that has the bindings
	 * @param Object The object to bind into the context.
	 * @param ObjectBindingType The class that should be used for the bindings. Must be a parent class of the Object parameter.
	 * @param BindingName The name of the binding.
	 */
	UFUNCTION(BlueprintCallable, Category="Dependency Injection", meta = (BlueprintInternalUseOnly=true, DefaultToSelf = "DiContextInterface"))
	static void BindObjectAsType(TScriptInterface<IDiContextInterface> DiContextInterface, UObject* Object, UClass* ObjectBindingType, FName BindingName);

private:
	DECLARE_FUNCTION(execTryResolveStruct);
	DECLARE_FUNCTION(execTryResolveStructCopy);
	DECLARE_FUNCTION(execTryResolveInterface);
	DECLARE_FUNCTION(execBindStruct);
};
