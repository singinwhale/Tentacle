// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once
#include "BindingId.h"
#include "TypeId.h"
#include "WeakFuture.h"
#include "StructUtils/InstancedStruct.h"

namespace DI
{
	/**
	 * Common parent for all bindings.
	 * This binding has no resolve-capabilities of its own but can do tracking for the Garbage Collector.
	 *
	 * Bindings are always owned by the DiContainer via TSharedRef<FBinding>, so deriving from
	 * TSharedFromThis lets any binding hand out weak self-refs (used e.g. by TResolverBinding to
	 * keep an async-resolver continuation safe against the binding being destroyed mid-flight).
	 */
	class FBinding : public TSharedFromThis<FBinding, ESPMode::ThreadSafe>
	{
	public:
		FBinding(FBindingId BindingId)
			: Id(MoveTemp(BindingId))
		{
		}

		virtual ~FBinding() = default;

		FORCEINLINE FBindingId GetId() const
		{
			return Id;
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector)
		{
			GetId().AddReferencedObjects(Collector);
		}

		virtual bool IsValid() const
		{
			return true;
		}

	private:
		FBindingId Id;
	};


	/**
	 *
	 * @tparam T
	 */
	template <class T>
	class TUObjectBinding : public FBinding
	{
	public:
		using Super = FBinding;

		TObjectPtr<T> UObjectDependency;

		TUObjectBinding(FBindingId BindingId, TObjectPtr<T> InObject)
			: Super(BindingId), UObjectDependency(MoveTemp(InObject))
		{
			static_assert(TIsDerivedFrom<T, UObject>::IsDerived);
			if (UObjectDependency)
			{
				checkf(
					UObjectDependency.GetClass()->IsChildOf(static_cast<UClass*>(BindingId.GetBoundTypeId().TryGetUType())),
					TEXT("%s is not derived from %s"),
					*UObjectDependency.GetClass()->GetName(),
					*BindingId.GetBoundTypeId().TryGetUType()->GetName()
				);
			}
		}

		virtual bool IsValid() const override
		{
			return ::IsValid(UObjectDependency);
		}

		virtual TWeakFuture<TObjectPtr<T>> Resolve() const
		{
			check(UObjectDependency);
			return MakeFulfilledWeakPromise<TObjectPtr<T>>(UObjectDependency).GetWeakFuture();
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Super::AddReferencedObjects(Collector);
			Collector.AddReferencedObject(UObjectDependency);
		}

	protected:
		/** Construct without an instance. Only subclasses (e.g. factory bindings) may use this. */
		explicit TUObjectBinding(FBindingId BindingId)
			: TUObjectBinding(MoveTemp(BindingId), nullptr)
		{
		}
	};


	class FUInterfaceBinding : public FBinding
	{
	public:
		using Super = FBinding;

		FScriptInterface InterfaceDependency;

		FUInterfaceBinding(FBindingId BindingId, const FScriptInterface& InInterface)
			: Super(BindingId), InterfaceDependency(InInterface)
		{
		}

		virtual bool IsValid() const override
		{
			return ::IsValid(InterfaceDependency.GetObject());
		}

		TWeakFuture<FScriptInterface> Resolve() const
		{
			check(InterfaceDependency.GetObject())
			return MakeFulfilledWeakPromise<FScriptInterface>(InterfaceDependency).GetWeakFuture();
		}

		/**
		 * Untyped, polymorphic resolve. Required by Blueprint paths that don't have compile-time
		 * type information. Derived (typed/factory) bindings override this to produce an FScriptInterface
		 * from their own state.
		 */
		virtual TWeakFuture<FScriptInterface> ResolveAsScriptInterface() const
		{
			return Resolve();
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Super::AddReferencedObjects(Collector);
			InterfaceDependency.AddReferencedObjects(Collector);
		}

	protected:
		/** Construct without an instance. Only subclasses (e.g. factory bindings) may use this. */
		explicit FUInterfaceBinding(FBindingId BindingId)
			: Super(BindingId), InterfaceDependency()
		{
		}
	};

	template <class T>
	class TUInterfaceDependencyBinding : public FUInterfaceBinding
	{
	public:
		using Super = FUInterfaceBinding;


		TUInterfaceDependencyBinding(FBindingId BindingId, const TScriptInterface<T>& InInterface)
			: Super(BindingId, InInterface)
		{
		}

		virtual TWeakFuture<TScriptInterface<T>> Resolve() const
		{
			check(InterfaceDependency.GetObject())
			return MakeFulfilledWeakPromise<TScriptInterface<T>>(InterfaceDependency.GetObject()).GetWeakFuture();
		}

	protected:
		/** Construct without an instance. Only subclasses (e.g. factory bindings) may use this. */
		explicit TUInterfaceDependencyBinding(FBindingId BindingId)
			: Super(MoveTemp(BindingId))
		{
		}
	};

	template <class T>
	class TSharedNativeDependencyBinding : public FBinding
	{
	public:
		using Super = FBinding;

		TSharedPtr<T> SharedNativeDependency;

		TSharedNativeDependencyBinding(FBindingId BindingId, TSharedRef<T> InSharedInstance)
			: Super(BindingId), SharedNativeDependency(InSharedInstance)
		{
		}

		virtual bool IsValid() const override
		{
			return SharedNativeDependency.IsValid();
		}

		virtual TWeakFuture<TSharedRef<T>> Resolve() const
		{
			check(SharedNativeDependency.IsValid());
			return MakeFulfilledWeakPromise<TSharedRef<T>>(SharedNativeDependency.ToSharedRef()).GetWeakFuture();
		}

	protected:
		/** Construct without an instance. Only subclasses (e.g. factory bindings) may use this. */
		explicit TSharedNativeDependencyBinding(FBindingId BindingId)
			: Super(BindingId), SharedNativeDependency()
		{
		}
	};

	/**
	 * Binding that owns arbitrary data that can be copied out.
	 */
	class FRawDataBinding : public FBinding
	{
	public:
		using Super = FBinding;

		FRawDataBinding(FBindingId BindingId)
			: Super(BindingId)
		{
		}

		virtual void CopyRawData(void* OutData, int32 SizeOfOutData) = 0;
	};


	/**
	 * Binding that holds UStruct data.
	 * Native types have to be referenced through shared pointers.
	 */
	class FUStructBinding : public FRawDataBinding
	{
	public:
		using Super = FRawDataBinding;

		FUStructBinding(UScriptStruct* StructType, FName BindingName, const uint8* StructMemoryToCopy)
			: Super(FBindingId(FTypeId(StructType), BindingName))
		{
			StructData.InitializeAs(StructType, StructMemoryToCopy);
		}

		const UScriptStruct* GetStruct()
		{
			return StructData.GetScriptStruct();
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Super::AddReferencedObjects(Collector);
			StructData.AddStructReferencedObjects(Collector);
		}

		virtual void CopyRawData(void* OutData, int32 InSizeOfOutData) override
		{
			// Use StructData.IsValid() instead of this->IsValid() because the binding should not get into other's hands in an invalid state.
			if (!StructData.IsValid())
				return;

			const UScriptStruct* StructClass = GetStruct();
			check(StructClass->GetStructureSize() <= InSizeOfOutData);
			StructClass->CopyScriptStruct(OutData, StructData.GetMemory(), 1);
		}

		virtual bool IsValid() const override
		{
			return StructData.IsValid();
		}

	protected:
		/**
		 * Construct with type only, no initial data. Only subclasses (e.g. factory bindings) may use this.
		 */
		FUStructBinding(UScriptStruct* StructType, FName BindingName)
			: Super(FBindingId(FTypeId(StructType), BindingName))
		{
			// do not default initialize so we can use IsValid to track init state.
		}

		/**
		 * Manages the inner struct data.
		 * We could inline the data to avoid the secondary allocation, but we would basically reimplement FInstancedStruct.
		 */
		FInstancedStruct StructData;
	};

	/**
	 * Binding that holds typed UStruct data.
	 * Native types have to be referenced through shared pointers.
	 */
	template <class T>
	class TTypedStructBinding : public FUStructBinding
	{
	public:
		using Super = FUStructBinding;

		TTypedStructBinding(FBindingId BindingId, const T& InInstance)
			: Super(T::StaticStruct(), BindingId.GetBindingName(), reinterpret_cast<const uint8*>(&InInstance))
		{
			checkf(T::StaticStruct() == BindingId.GetBoundTypeId().TryGetUType(), TEXT("Inherited struct types are not supported at this moment"));
		}

		virtual TWeakFuture<const T&> Resolve() const
		{
			if (!IsValid())
				return {};

			return MakeFulfilledWeakPromise<const T&>(StructData.Get<T>()).GetWeakFuture();
		}

	protected:
		/** Construct without an instance. Only subclasses (e.g. factory bindings) may use this. */
		explicit TTypedStructBinding(FBindingId BindingId)
			: Super(T::StaticStruct(), BindingId.GetBindingName())
		{
			checkf(T::StaticStruct() == BindingId.GetBoundTypeId().TryGetUType(), TEXT("Inherited struct types are not supported at this moment"));
		}
	};


	template <class T>
	using TBindingType = DI::TBindingInstanceTypeSwitch<
		T,
		TUObjectBinding<T>, // UObject
		TUInterfaceDependencyBinding<T>, // IInterface
		TTypedStructBinding<T>, // UStruct
		TSharedNativeDependencyBinding<T>>; // Native
}
