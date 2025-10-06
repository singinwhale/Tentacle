// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "TentacleTemplates.h"
#include "CoreMinimal.h"
#include "Mocks/SimpleService.h"

namespace DI
{
	static_assert(THasUClass<USimpleUService>::Value, "UClasses should have a UClass");
	static_assert(THasUStruct<USimpleUService>::Value, "UClasses should have a UStruct");
	static_assert(!TModels<CNativeMemberTypeIdProvider, USimpleUService>::Value, "UClasses should not provide member type ids");

	static_assert(!THasUClass<FSimpleNativeService>::Value, "Native classes should not have a UClass");
	static_assert(!THasUStruct<FSimpleNativeService>::Value, "Native classes should not have a UStruct");
	static_assert(TModels<CNativeMemberTypeIdProvider, FSimpleNativeService>::Value, "Native classes should provide native type ids through members");
	static_assert(!TModels<CNativeMemberTypeIdProvider, FMockEngineType>::Value, "Native foreign classes should provide native type ids through free functions");
	static_assert(sizeof(DI::GetFreeTypeId<FMockEngineType>()), "Native foreign classes should provide native type ids through free functions");

	static_assert(!THasUClass<FSimpleUStructService>::Value, "UStruct should not have a UClass");
	static_assert(THasUStruct<FSimpleUStructService>::Value, "UStruct should have a UStruct");
	static_assert(!TModels<CNativeMemberTypeIdProvider, FSimpleUStructService>::Value, "UStructs should not provide a native member type");

	// non-nullable types
	static_assert(std::is_same_v<TBindingInstRef<USimpleUService>, TObjectPtr<USimpleUService>>, "UObjects should be referenced as reference");
	static_assert(std::is_same_v<TBindingInstRef<USimpleInterfaceImplementation>, TObjectPtr<USimpleInterfaceImplementation>>, "UObjects that implement an UInterface should be referenced as references");
	static_assert(std::is_same_v<TBindingInstRef<ISimpleInterface>, TScriptInterface<ISimpleInterface>>, "UObjects that implement an UInterface should be referenced as TScriptInterface");
	static_assert(std::is_same_v<TBindingInstRef<FSimpleUStructService>, const FSimpleUStructService&>, "UStructs should be referenced as const Refs");
	static_assert(std::is_same_v<TBindingInstRef<FSimpleNativeService>, TSharedRef<FSimpleNativeService>>, "NativeTypes should be referenced as SharedRefs");

	// nullable types
	static_assert(std::is_same_v<TBindingInstPtr<USimpleUService>, TObjectPtr<USimpleUService>>, "UObjects should be optionally passed as TObjectPtr");
	static_assert(std::is_same_v<TBindingInstPtr<USimpleInterfaceImplementation>, TObjectPtr<USimpleInterfaceImplementation>>, "UObjects that implement an UInterface should be optionally passed as TObjectPtr");
	static_assert(std::is_same_v<TBindingInstPtr<ISimpleInterface>, TScriptInterface<ISimpleInterface>>, "Interfaces should be optionally passed as TScriptInterface");
	static_assert(std::is_same_v<TBindingInstPtr<FSimpleUStructService>, TOptional<const FSimpleUStructService&>>, "UStructs should be optionally passed as TOptional<ConstRef>");
	static_assert(std::is_same_v<TBindingInstPtr<FSimpleNativeService>, TSharedPtr<FSimpleNativeService>>, "Native Types should be optionally passed as SharedPtr");

	// non-nullable base types
	static_assert(std::is_same_v<TBindingInstRefBaseType<USimpleUService&>::Type, USimpleUService>, "UObjects should be referenced as reference");
	static_assert(std::is_same_v<TBindingInstRefBaseType<USimpleInterfaceImplementation&>::Type, USimpleInterfaceImplementation>, "UObjects that implement an UInterface should be referenced as references");
	static_assert(std::is_same_v<TBindingInstRefBaseType<TScriptInterface<ISimpleInterface>>::Type, ISimpleInterface>, "Interfaces should be referenced as TScriptInterface");
	static_assert(std::is_same_v<TBindingInstRefBaseType<FSimpleUStructService&>::Type, FSimpleUStructService>, "UStructs should be referenced as Refs");
	static_assert(std::is_same_v<TBindingInstRefBaseType<const FSimpleUStructService&>::Type, FSimpleUStructService>, "UStructs should be referenced as const Refs");
	static_assert(std::is_same_v<TBindingInstRefBaseType<TSharedRef<FSimpleNativeService>>::Type, FSimpleNativeService>, "NativeTypes should be referenced as SharedRefs");

	// nullable types
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TObjectPtr<USimpleUService>>::Type, USimpleUService>, "UObjects should be optionally passed as TObjectPtr");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<const TObjectPtr<USimpleUService>&>::Type, USimpleUService>, "UObjects should be optionally passed as TObjectPtr");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TObjectPtr<USimpleUService>&>::Type, USimpleUService>, "UObjects should be optionally passed as TObjectPtr");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TObjectPtr<USimpleInterfaceImplementation>>::Type, USimpleInterfaceImplementation>, "UObjects that implement an UInterface should be optionally passed as TObjectPtr");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TScriptInterface<ISimpleInterface>>::Type, ISimpleInterface>, "Interfaces should be optionally passed as TScriptInterface");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<const TScriptInterface<ISimpleInterface>&>::Type, ISimpleInterface>, "Interfaces should be optionally passed as TScriptInterface");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TScriptInterface<ISimpleInterface>&>::Type, ISimpleInterface>, "Interfaces should be optionally passed as TScriptInterface");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TOptional<FSimpleUStructService>>::Type, FSimpleUStructService>, "UStructs should be optionally passed as TOptional");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<const TOptional<FSimpleUStructService>&>::Type, FSimpleUStructService>, "UStructs should be optionally passed as TOptional");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TOptional<FSimpleUStructService>&>::Type, FSimpleUStructService>, "UStructs should be optionally passed as TOptional");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TSharedPtr<FSimpleNativeService>>::Type, FSimpleNativeService>, "Native Types should be optionally passed as SharedPtr");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<const TSharedPtr<FSimpleNativeService>&>::Type, FSimpleNativeService>, "Native Types should be optionally passed as SharedPtr");
	static_assert(std::is_same_v<TBindingInstPtrBaseType<TSharedPtr<FSimpleNativeService>&>::Type, FSimpleNativeService>, "Native Types should be optionally passed as SharedPtr");

}
