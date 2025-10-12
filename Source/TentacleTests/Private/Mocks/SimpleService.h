// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "TypeId.h"
#include "UObject/Object.h"
#include "SimpleService.generated.h"

UCLASS()
class USimpleUService : public UObject
{
	GENERATED_BODY()

public:
	int32 A;
};

USTRUCT()
struct FSimpleUStructService
{
	GENERATED_BODY()

	int32 A = 0;

	FSimpleUStructService()
	{
	}

	FSimpleUStructService(int32 InA):A(InA)
	{
	}

	bool operator==(const FSimpleUStructService& Other) const
	{
		return A == Other.A;
	}
};

class FSimpleNativeService
{
public:
	DI_DEFINE_NATIVE_TYPEID_MEMBER(FSimpleNativeService)

	int32 A;
};

class FMockEngineType
{
public:

	int32 A;
};

DI_DECLARE_FREE_NATIVE_TYPEID(TENTACLETESTS_API, FMockEngineType)


UINTERFACE(NotBlueprintable, NotBlueprintType)
class USimpleInterface : public UInterface
{
	GENERATED_BODY()
};

class ISimpleInterface
{
	GENERATED_BODY()

public:
	virtual int32 GetA() const = 0;
};

UCLASS()
class USimpleInterfaceImplementation : public UObject, public ISimpleInterface
{
	GENERATED_BODY()

public:
	//  - ISimpleInterface
	virtual int32 GetA() const override;
	// -- 

	int32 A;
};

namespace DI
{
	namespace InjectTest
	{
		TObjectPtr<USimpleUService> InjectDependencies(TObjectPtr<USimpleUService> SimpleUService);;
	}
}
