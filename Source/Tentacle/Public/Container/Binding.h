#pragma once
#include "BindingId.h"
#include "TypeId.h"
#include "StructUtils/InstancedStruct.h"

namespace DI
{
	/**
	 * Common parent for all bindings.
	 * This binding has no resolve-capabilities of its own but can do tracking for the Garbage Collector.
	 */
	class FBinding
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
	class TUObjectBinding final : public FBinding
	{
	public:
		using Super = FBinding;

		TObjectPtr<T> UObjectDependency;

		TUObjectBinding(FBindingId BindingId, TObjectPtr<T> InObject)
			: Super(BindingId), UObjectDependency(MoveTemp(InObject))
		{
			static_assert(TIsDerivedFrom<T, UObject>::IsDerived);
			checkf(
				InObject.GetClass()->IsChildOf(static_cast<UClass*>(BindingId.GetBoundTypeId().TryGetUType())),
				TEXT("%s is not derived from %s"),
				*InObject.GetClass()->GetName(),
				*BindingId.GetBoundTypeId().TryGetUType()->GetName()
			);
		}

		virtual bool IsValid() const override
		{
			return ::IsValid(UObjectDependency);
		}

		TObjectPtr<T> Resolve() const
		{
			check(UObjectDependency);
			return UObjectDependency;
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Super::AddReferencedObjects(Collector);
			Collector.AddReferencedObject(UObjectDependency);
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

		const FScriptInterface& Resolve() const
		{
			check(InterfaceDependency.GetObject())
			return InterfaceDependency;
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Super::AddReferencedObjects(Collector);
			InterfaceDependency.AddReferencedObjects(Collector);
		}
	};

	template <class T>
	class TUInterfaceDependencyBinding final : public FUInterfaceBinding
	{
	public:
		using Super = FUInterfaceBinding;


		TUInterfaceDependencyBinding(FBindingId BindingId, const TScriptInterface<T>& InInterface)
			: Super(BindingId, InInterface)
		{
		}

		TScriptInterface<T> Resolve() const
		{
			return TScriptInterface<T>(InterfaceDependency.GetObject());
		}
	};

	template <class T>
	class TSharedNativeDependencyBinding final : public FBinding
	{
	public:
		using Super = FBinding;

		TSharedRef<T> SharedNativeDependency;

		TSharedNativeDependencyBinding(FBindingId BindingId, TSharedRef<T> InSharedInstance)
			: Super(BindingId), SharedNativeDependency(InSharedInstance)
		{
		}

		TSharedRef<T> Resolve() const
		{
			return SharedNativeDependency;
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

		virtual void CopyRawData(void* OutData, int32 OutDataSize) override
		{
			const UScriptStruct* StructClass = GetStruct();
			check(StructClass->GetStructureSize() <= OutDataSize);
			StructClass->CopyScriptStruct(OutData, StructData.GetMemory(), 1);
		};

	protected:
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
	class TTypedStructBinding final : public FUStructBinding
	{
	public:
		using Super = FUStructBinding;

		TTypedStructBinding(FBindingId BindingId, const T& InInstance)
			: Super(T::StaticStruct(), BindingId.GetBindingName(), reinterpret_cast<const uint8*>(&InInstance))
		{
			checkf(T::StaticStruct() == BindingId.GetBoundTypeId().TryGetUType(), TEXT("Inherited struct types are not supported at this moment"));
		}

		const T& Resolve() const
		{
			return StructData.Get<T>();
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
