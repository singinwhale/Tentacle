// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#pragma once

#include <type_traits>

namespace DI
{
	template <bool Predicate, typename TypeA = void, typename TypeB = void>
	struct TConditional;

	template <typename TypeA, typename TypeB>
	struct TConditional<true, TypeA, TypeB>
	{
		using Type = TypeA;
	};

	template <typename TypeA, typename TypeB>
	struct TConditional<false, TypeA, TypeB>
	{
		using Type = TypeB;
	};


	template <class T>
	typename TEnableIf<TIsIInterface<T>::Value, UClass*>::Type
	GetStaticClass()
	{
		return T::UClassType::StaticClass();
	}

	template <class T>
	typename TEnableIf<TIsDerivedFrom<T, UObject>::Value, UClass*>::Type
	GetStaticClass()
	{
		return T::StaticClass();
	}

	template <class T>
	typename TEnableIf<TModels<CStaticStructProvider, T>::Value, UScriptStruct*>::Type
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
	using TBindingInstanceTypeSwitch = typename TConditional<
		THasUStruct<typename TDecay<T>::Type>::Value,
		typename TConditional<THasUClass<typename TDecay<T>::Type>::Value,
		                      typename TConditional<TIsIInterface<typename TDecay<T>::Type>::Value,
		                                            TUInterfaceType,
		                                            TUObjectType>::Type,
		                      TUStructType>::Type,
		TNativeType>::Type;

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
		using Type = typename TBindingInstBaseType<typename TDecay<T>::Type>::Type;
	};

	template <class T>
	struct TBindingInstBaseType<T&>
	{
		using Type = typename TBindingInstBaseType<typename TDecay<T>::Type>::Type;
	};

	template <class T>
	struct TBindingInstBaseType<T*>
	{
		using Type = typename TBindingInstBaseType<T>::Type;
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

	template<typename T>
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
	 * Creates a TTuple<T, T, ..., T> of the requested length with every element initialised to Value.
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

	template<class T>
	using TVoid = decltype(static_cast<void>(DeclVal<T>()));


	template<class T>
	concept CConvertibleToBool = requires
	{
		static_cast<bool>(DeclVal<T>());
	};
	
	template<class T>
	struct TIsBindingPtrValid
	{
		static constexpr bool Check(const T& Value) { return true; }
	};
	
	template<CConvertibleToBool T>
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

	template <class... TBindingInstPtrs>
	auto TryDerefAllInstances(TTuple<TBindingInstPtrs...> InstancePointers) // -> TOptional<TTuple<TBindingInstRef<T>...>>
	{
		using TOptionalResultType = TOptional<TTuple<TBindingInstRef<typename TBindingInstBaseType<TBindingInstPtrs>::Type>...>>;
		if (AreBindingInstsValid(InstancePointers))
		{
			return TOptionalResultType(TransformTuple(InstancePointers, [](const auto& InstancePointer) {
				return ToRefType(InstancePointer);
			}));
		}
		return TOptionalResultType();
	}
	
	
}
