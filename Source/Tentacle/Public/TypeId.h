// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "TentacleTemplates.h"
#include "UObject/Object.h"


namespace DI
{
	class FTypeId;
}

uint32 GetTypeHash(const DI::FTypeId& TypeId);

namespace DI
{
	namespace Private
	{
		FTypeId MakeNativeTypeId(const TCHAR* TypeName);

		//@see https://stackoverflow.com/a/38637849
		template <typename... Ts>
		struct TAlwaysFalse : std::false_type
		{
		};
	}

/**
 * RTTI-like type ID that uses either UObject information or manually defined type names for native types.
 * You can make a typeID for a type by using DI::GetTypeId<T>();
 * Example for native types (UTypes don't need this):
 * @code
 *	class MyType {
 *		DI_DEFINE_NATIVE_TYPEID(MyType)
 *	}
 *
 *	void Main(){
 *		FTypeId MyTypeId = DI::GetTypeId<MyType>();
 *	}
 * @endcode
 */
class TENTACLE_API FTypeId
{
	friend FTypeId Private::MakeNativeTypeId(const TCHAR* TypeName);
	friend auto ::GetTypeHash(const FTypeId&) -> uint32;

private:
	enum class EIdType : uint8
	{
		Invalid = 255,
		UType = 0,
		NativeType = 1,
	};

	EIdType Type = EIdType::Invalid;

	union
	{
		TObjectPtr<UStruct> UType;
		const TCHAR* NativeClassId;
	};

	explicit FTypeId(const TCHAR* ClassId)
		: Type(EIdType::NativeType), NativeClassId(ClassId)
	{
	}

public:
	FTypeId()
		: Type(EIdType::Invalid),
		  UType(nullptr)
	{
	}

	FTypeId(const FTypeId& Other)
	{
		*this = Other;
	}

	FTypeId(FTypeId&& Other)
	{
		*this = MoveTemp(Other);
	}

	FTypeId& operator=(const FTypeId& Other)
	{
		Type = Other.Type;
		switch (Other.Type)
		{
		case EIdType::Invalid:
			break;
		case EIdType::UType:
			UType = Other.UType;
			break;
		case EIdType::NativeType:
			NativeClassId = Other.NativeClassId;
			break;
		}
		return *this;
	}

	FTypeId& operator=(FTypeId&& Other)
	{
		Type = Other.Type;
		switch (Other.Type)
		{
		case EIdType::Invalid:
			break;
		case EIdType::UType:
			UType = Other.UType;
			break;
		case EIdType::NativeType:
			NativeClassId = Other.NativeClassId;
			break;
		}
		Other.Type = EIdType::Invalid;
		Other.UType = nullptr;
		return *this;
	}

	explicit FTypeId(UStruct* TypeClass)
		: Type(EIdType::UType), UType(TypeClass)
	{
	}

	static const FTypeId InvalidId; 

	FName GetName() const;

	UStruct* TryGetUType() const
	{
		if (Type == EIdType::UType)
		{
			return UType;
		}
		return nullptr;
	}

	void AddReferencedObjects(FReferenceCollector& Collector)
	{
		if (Type == EIdType::UType)
		{
			Collector.AddReferencedObject(UType);
		}
	}

	const void* GetTypeIdAddress() const
	{
		switch (Type)
		{
		case EIdType::Invalid:
		default:
			return nullptr;
		case EIdType::UType:
			return UType;
		case EIdType::NativeType:
			return NativeClassId;
		}
	}

	FORCEINLINE bool operator==(const FTypeId& Other) const
	{
		return
			Type == Other.Type
			&& GetTypeIdAddress() == Other.GetTypeIdAddress();
	}
};

}

FORCEINLINE uint32 GetTypeHash(const DI::FTypeId& TypeId)
{
	return HashCombine(GetTypeHash(TypeId.Type), GetTypeHash(TypeId.GetTypeIdAddress()));
}

#define DI_TYPEID_BODY(TypeName)\
	static_assert(sizeof(TypeName) != 0, #TypeName " does not name a type.");\
	static const TCHAR* TypeName ## TypeName = TEXT(PREPROCESSOR_TO_STRING(TypeName)); \
	static const ::DI::FTypeId TypeName ## TypeId = ::DI::Private::MakeNativeTypeId(TypeName ## TypeName);\
	return TypeName ## TypeId;

/**
 * Use this to define a typeID inside a type.
 * Use for types that you wrote yourself and where you can edit the source files.
 * @param TypeName Type of the class. Can include namespace declarations.
 */
#define DI_DEFINE_NATIVE_TYPEID_MEMBER(TypeName)\
		FORCEINLINE static const ::DI::FTypeId& GetTypeId()\
		{ \
			DI_TYPEID_BODY(TypeName)\
		}


#define DI_DECLARE_FREE_NATIVE_TYPEID(API_MACRO, TypeName)\
	namespace DI {\
		template<>\
		const FTypeId& GetFreeTypeId<TypeName>();\
	}

/**
 * Use this to define a typeID outside the type.
 * Use for types that are defined in foreign code where you can't define the typeID as a member function.
 * @param TypeName Type of the class. Can include namespace declarations.
 */
#define DI_DEFINE_FREE_NATIVE_TYPEID(TypeName)\
	namespace DI {\
		template<>\
		const FTypeId& GetFreeTypeId<TypeName>()\
		{ \
			DI_TYPEID_BODY(TypeName)\
		}\
	}


namespace DI
{
	/**
	 * Concept for types that have a member-style typeID provider
	 * i.e. the ones that use DI_DEFINE_NATIVE_TYPEID_MEMBERa
	 */
	struct CNativeMemberTypeIdProvider
	{
		template <typename T>
		auto Requires() -> decltype(
			FTypeId(T::GetTypeId())
		);
	};

	template <class T>
	const FTypeId& GetFreeTypeId()
	{
		static_assert(
			Private::TAlwaysFalse<T>::value,
			"Your type does not provide a Type Use `DI_DEFINE_NATIVE_TYPEID_MEMBER(YourClass)` in your class"
			" or use `DI_DEFINE_FREE_NATIVE_TYPEID(SomeClass)` to define the type ID for a foreign type."
		);
		return FTypeId::InvalidId;
	}

	template <class T>
	typename TEnableIf<THasUStruct<T>::Value, const FTypeId&>::Type
	GetTypeId() /* -> FTypeId */
	{
		static const FTypeId StaticUStructId = FTypeId(GetStaticClass<T>());
		return StaticUStructId;
	}

	template <class T>
	typename TEnableIf<TModels<CNativeMemberTypeIdProvider, T>::Value, const FTypeId&>::Type
	GetTypeId() /* -> FTypeId */
	{
		return T::GetTypeId();
	}

	template <class T>
	typename TEnableIf<!TOr<THasUStruct<T>, TModels<CNativeMemberTypeIdProvider, T>>::Value, const FTypeId&>::Type
	GetTypeId() /* -> FTypeId */
	{
		return ::DI::GetFreeTypeId<T>();
	}

	namespace Private
	{
		FORCEINLINE FTypeId MakeNativeTypeId(const TCHAR* TypeName)
		{
			return FTypeId(TypeName);
		}
	}
}
