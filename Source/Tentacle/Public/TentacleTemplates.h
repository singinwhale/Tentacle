// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#pragma once

#include <type_traits>
#include "UObject/WeakInterfacePtr.h"
#include "Engine/GameInstance.h"
#include "Templates/Casts.h"
#include "UObject/Interface.h"

namespace DI
{
#ifdef __JETBRAINS_IDE__
#define TENTACLE_REQUIRES_CUSTOM_TIsInterface
#else
#ifdef __RESHARPER__
#define TENTACLE_REQUIRES_CUSTOM_TIsInterface
#endif
#endif

#ifdef TENTACLE_REQUIRES_CUSTOM_TIsInterface
	/**
	 * Metafunction which detects whether or not a class is an IInterface.  Rules:
	 *
	 * 1. A UObject is not an IInterface.
	 * 2. A type without a UClassType typedef member is not an IInterface.
	 * 3. A type whose UClassType is derived from UInterface.
	 *
	 * Otherwise, assume it's an IInterface.
	 *
	 * This shadows the implementation in Templates/Casts.h and
	 * is required for some IDEs because they can't resolve the StaticClassFlags at lint time.
	 */
	template <typename T, bool bIsAUObject_IMPL = std::is_convertible_v<T*, const volatile UObject*>>
	struct TIsIInterface
	{
		enum { Value = false };
	};

	template <typename T>
	struct TIsIInterface<T, false>
	{
		template <typename U> static char (&Resolve(typename U::UClassType*))[(std::derived_from<typename U::UClassType, UInterface>) ? 2 : 1];
		template <typename U> static char (&Resolve(...))[1];

		enum { Value = sizeof(Resolve<T>(0)) - 1 };
	};
#endif

#undef TENTACLE_REQUIRES_CUSTOM_TIsInterface

	template <class T>
	TEnableIf<TIsIInterface<T>::Value, UClass*>::Type
	GetStaticClass()
	{
		return T::UClassType::StaticClass();
	}

	template <class T>
	TEnableIf<TIsDerivedFrom<T, UObject>::Value, UClass*>::Type
	GetStaticClass()
	{
		return T::StaticClass();
	}

	template <class T>
	TEnableIf<TModels<CStaticStructProvider, T>::Value, UScriptStruct*>::Type
	GetStaticClass()
	{
		return T::StaticStruct();
	}

	template <class T>
	using THasUClass = TOr<TIsIInterface<T>, TIsDerivedFrom<T, UObject>>;

	template <class T>
	using THasUStruct = TOr<THasUClass<T>, TModels<CStaticStructProvider, T>>;

	template <class T>
	constexpr bool HasUStruct()
	{
		return THasUStruct<T>::Value;
	}

	namespace BindingInstanceTypes
	{
		template <class T>
		using Native = TSharedRef<T>;

		template <class T>
		using UStruct = TOptional<T>;

		template <class T>
		using UObject = TObjectPtr<T>;
	}

	template <class T, typename TUObjectType, typename TUInterfaceType, typename TUStructType, typename TNativeType>
	using TBindingInstanceTypeSwitch = std::conditional_t<
		THasUStruct<std::decay_t<T>>::Value,
		std::conditional_t<THasUClass<std::decay_t<T>>::Value,
		                   std::conditional_t<TIsIInterface<std::decay_t<T>>::Value,
		                                      TUInterfaceType,
		                                      TUObjectType>,
		                   TUStructType>,
		TNativeType>;

	// Binding Instance Reference Type (Non-nullable)
	template <class T>
	using TBindingInstRef = TBindingInstanceTypeSwitch<
		T,
		/* TUObjectType */ TObjectPtr<T>, //using TObjectPtr<T> and not T& because converting from T& to TObjectPtr throws a warning. 
		/* TUInterfaceType */ TScriptInterface<T>,
		/* TUStructType */ const T&,
		/* TNativeType */ TSharedRef<T>>;

	// Binding Instance Optional Type (Nullable)
	template <class T>
	using TBindingInstPtr = TBindingInstanceTypeSwitch<
		T,
		/* TUObjectType */ TObjectPtr<T>,
		/* TUInterfaceType */ TScriptInterface<T>,
		/* TUStructType */ TOptional<const T&>,
		/* TNativeType */ TSharedPtr<T>>;


	// Binding Instance Weak Ptr
	template <class T>
	using TBindingInstWeakPtr = TBindingInstanceTypeSwitch<
		T,
		/* TUObjectType */ TWeakObjectPtr<T>,
		/* TUInterfaceType */ TWeakInterfacePtr<T>,
		/* TUStructType */ T,
		/* TNativeType */ TWeakPtr<T>>;


	/* Gets the base/inner/raw type from a TBindingInstPtr or TBindingInstRef */
	template <class T>
	struct TBindingInstBaseType
	{
		using Type = T;
	};

	template <class T>
	struct TBindingInstBaseType<const T&>
	{
		using Type = TBindingInstBaseType<typename TDecay<T>::Type>::Type;
	};

	template <class T>
	struct TBindingInstBaseType<T&>
	{
		using Type = TBindingInstBaseType<typename TDecay<T>::Type>::Type;
	};

	template <class T>
	struct TBindingInstBaseType<T*>
	{
		using Type = TBindingInstBaseType<T>::Type;
	};

	template <class T>
	struct TBindingInstBaseType<TScriptInterface<T>>
	{
		using Type = T;
	};

	template <class T>
	struct TBindingInstBaseType<TScriptInterface<T>&>
	{
		using Type = T;
	};

	template <class T>
	struct TBindingInstBaseType<const TScriptInterface<T>&>
	{
		using Type = T;
	};

	template <class T>
	struct TBindingInstBaseType<TSharedPtr<T>>
	{
		using Type = T;
	};

	template <class T>
	struct TBindingInstBaseType<TSharedRef<T>>
	{
		using Type = T;
	};

	template <class T>
	struct TBindingInstBaseType<TOptional<T>>
	{
		using Type = T;
	};

	template <class T>
	struct TBindingInstBaseType<TObjectPtr<T>>
	{
		using Type = T;
	};

	template <class T>
	struct TBindingInstBaseType<const TObjectPtr<T>>
	{
		using Type = T;
	};

	template <typename T>
	concept CHasType = requires
	{
		typename T::Type;
	};

	/**
	 * Converts a binding instance optional (TBindingInstPtr)to a binding instance reference (TBindingInstRef)
	 * Asserts if that is not possible.
	 */
	template <class T>
	TScriptInterface<T> ToRefType(TScriptInterface<T> Nullable)
	{
		static_assert(std::is_same_v<TBindingInstPtr<T>, TScriptInterface<T>>);
		static_assert(std::is_same_v<TBindingInstRef<T>, TScriptInterface<T>>);
		check(Nullable.GetObjectRef());
		return Nullable;
	}

	template <class T>
	TSharedRef<T> ToRefType(TSharedPtr<T> Nullable)
	{
		static_assert(std::is_same_v<TBindingInstPtr<T>, TSharedPtr<T>>);
		static_assert(std::is_same_v<TBindingInstRef<T>, TSharedRef<T>>);
		return Nullable.ToSharedRef();
	}

	template <class T>
	T& ToRefType(TOptional<T>& Nullable)
	{
		static_assert(std::is_same_v<TBindingInstPtr<T>, TOptional<T>>);
		static_assert(std::is_same_v<TBindingInstRef<T>, T&>);
		return Nullable.GetValue();
	}

	template <class T>
	const T& ToRefType(const TOptional<T>& Nullable)
	{
		static_assert(std::is_same_v<TBindingInstPtr<T>, TOptional<T>>);
		static_assert(std::is_same_v<TBindingInstRef<T>, T&>);
		return Nullable.GetValue();
	}

	template <class T>
	const TObjectPtr<T>& ToRefType(const TObjectPtr<T>& Nullable)
	{
		static_assert(std::is_same_v<TBindingInstPtr<T>, TObjectPtr<T>>);
		static_assert(std::is_same_v<TBindingInstRef<T>, TObjectPtr<T>>);
		check(Nullable);
		return Nullable;
	}

	/** Converts the binding instance optional type to a matching reference type. */
	template <class T>
	struct TToRefType
	{
		using Type = TBindingInstRef<typename TBindingInstBaseType<T>::Type>;
	};

	template <class T>
	TOptional<TObjectPtr<T>> ResolveWeakBindingInstPtr(const TWeakObjectPtr<T>& WeakPtr)
	{
		TObjectPtr<T> Object = WeakPtr.Get();
		return Object ? Object : TOptional<TObjectPtr<T>>();
	}

	template <class T>
	TOptional<TScriptInterface<T>> ResolveWeakBindingInstPtr(const TWeakInterfacePtr<T>& WeakPtr)
	{
		return WeakPtr.IsValid() ? WeakPtr.ToScriptInterface() : TOptional<T>{};
	}

	template <class T>
	TOptional<T> ResolveWeakBindingInstPtr(const T& WeakPtr)
	{
		return WeakPtr;
	}

	template <class T>
	TOptional<TSharedRef<T>> ResolveWeakBindingInstPtr(const TWeakPtr<T>& WeakPtr)
	{
		TSharedPtr<T> SharedPtr = WeakPtr.Pin();
		return SharedPtr ? SharedPtr.ToSharedRef() : TOptional<TSharedRef<T>>{};
	}

	template <class T>
	TWeakObjectPtr<T> MakeWeakBindingInstPtr(TObjectPtr<T> Instance)
	{
		return MakeWeakObjectPtr(Instance);
	}

	template <class T>
	TWeakInterfacePtr<T> MakeWeakBindingInstPtr(TScriptInterface<T> Instance)
	{
		return TWeakInterfacePtr<T>(Instance);
	}

	template <class T>
	T MakeWeakBindingInstPtr(T Instance)
	{
		return Instance;
	}

	template <class T>
	TWeakPtr<T> MakeWeakBindingInstPtr(TSharedRef<T> Instance)
	{
		return Instance.ToWeakPtr();
	}

	/**
	 * Creates a TTuple<T, T, ..., T> of the requested length with every element initialized to Value.
	 * Example:
	 *   MakeUniformTuple<FName, 3>(NAME_None);   // -> TTuple<FName, FName, FName> { NAME_None, NAME_None, NAME_None }
	 */
	template <typename T, uint32... Indices>
	auto MakeUniformTupleImpl(const T& Value, TIntegerSequence<uint32, Indices...>)
	{
		// Expand the tuple with N copies of Value
		return MakeTuple((static_cast<void>(Indices), Value)...);
	}

	template <typename T, uint32 N>
	auto MakeUniformTuple(const T& Value)
	{
		return MakeUniformTupleImpl<T>(Value, TMakeIntegerSequence<uint32, N>{});
	}

	template <class T>
	using TVoid = decltype(static_cast<void>(DeclVal<T>()));


	template <class T>
	concept CConvertibleToBool = requires
	{
		static_cast<bool>(DeclVal<T>());
	};

	template <class T>
	struct TIsBindingPtrValid
	{
		static constexpr bool Check(const T& Value) { return true; }
	};

	template <CConvertibleToBool T>
	struct TIsBindingPtrValid<T>
	{
		static bool Check(const T& Value) { return static_cast<bool>(Value); }
	};

	template <class... TBindingInsts>
	bool AreBindingInstsValid(const TTuple<TBindingInsts...>& Tuple)
	{
		bool bIsValid = true;
		VisitTupleElements([&]<class T>(const T& Value)
		{
			bIsValid = bIsValid && TIsBindingPtrValid<T>::Check(Value);
		}, Tuple);
		return bIsValid;
	}

	template <class TResultTuple, class TSourceTuple, uint32... Indices>
	TResultTuple MakeDerefedTuple(TSourceTuple& InstancePointers, TIntegerSequence<uint32, Indices...>)
	{
		return TResultTuple(ToRefType(InstancePointers.template Get<Indices>())...);
	}

	template <class... TBindingInstPtrs>
	auto TryDerefAllInstances(TTuple<TBindingInstPtrs...> InstancePointers) // -> TOptional<TTuple<TBindingInstRef<T>...>>
	{
		using TResultTuple = TTuple<TBindingInstRef<typename TBindingInstBaseType<TBindingInstPtrs>::Type>...>;
		using TOptionalResultType = TOptional<TResultTuple>;
		if (AreBindingInstsValid(InstancePointers))
		{
			// TransformTuple/MakeTuple decays reference types (const T& -> T), causing structs to be copied
			// rather than forwarded by reference. Directly constructing TResultTuple with an index sequence
			// preserves reference element types (TTuple<const T&>) and avoids the copy.
			return TOptionalResultType(MakeDerefedTuple<TResultTuple>(InstancePointers, TMakeIntegerSequence<uint32, sizeof...(TBindingInstPtrs)>{}));
		}
		return TOptionalResultType();
	}
}
