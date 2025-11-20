// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "TypeId.h"

const DI::FTypeId DI::FTypeId::InvalidId = FTypeId();

FName DI::FTypeId::GetName() const
{
	switch (Type)
	{
	case EIdType::Invalid:
	default:
		return TEXT("Invalid");
	case EIdType::UType:
		return UType->GetFName();
		break;
	case EIdType::NativeType:
		return NativeClassId;
	}
}
