// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once
#include "TypeId.h"

namespace DI
{
	/**
	 * ID for a binding.
	 * Consists of a TypeID and a Name.
	 */
	class TENTACLE_API FBindingId
	{
	public:
		FBindingId() = default;

		explicit FBindingId(FTypeId InBoundTypeId)
			: BoundTypeId(MoveTemp(InBoundTypeId)), BindingName(NAME_None)
		{
		}

		explicit FBindingId(FTypeId InBoundTypeId, FName InBindingName)
			: BoundTypeId(MoveTemp(InBoundTypeId)), BindingName(MoveTemp(InBindingName))
		{
		}

		~FBindingId() = default;

		FORCEINLINE const FTypeId& GetBoundTypeId() const
		{
			return BoundTypeId;
		}

		FORCEINLINE void AddReferencedObjects(FReferenceCollector& Collector)
		{
			return BoundTypeId.AddReferencedObjects(Collector);
		}

		FORCEINLINE const FName& GetBindingName() const
		{
			return BindingName;
		}

		FORCEINLINE FString ToString() const
		{
			return FString::Printf(TEXT("%s:%s"), *BoundTypeId.GetName().ToString(), *BindingName.ToString());
		}

	private:
		FTypeId BoundTypeId = {};
		FName BindingName = NAME_None;
	};

	FORCEINLINE bool operator==(const FBindingId& A, const FBindingId& B)
	{
		return A.GetBoundTypeId() == B.GetBoundTypeId()
			&& A.GetBindingName() == B.GetBindingName();
	}

	template <class T>
	static FBindingId MakeBindingId()
	{
		return FBindingId(DI::GetTypeId<T>());
	}

	template <class T>
	static FBindingId MakeBindingId(FName BindingName)
	{
		return FBindingId(DI::GetTypeId<T>(), MoveTemp(BindingName));
	}

	
	FORCEINLINE uint32 GetTypeHash(const DI::FBindingId& Binding)
	{
		return HashCombine(::GetTypeHash(Binding.GetBoundTypeId()), GetTypeHash(Binding.GetBindingName()));
	}
}

FORCEINLINE uint32 GetTypeHash(const DI::FBindingId& Binding)
{
	return DI::GetTypeHash(Binding);
}
