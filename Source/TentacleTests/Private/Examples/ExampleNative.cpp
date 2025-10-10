#include "ExampleNative.h"

TSharedPtr<FSimpleNativeService> FExampleNative::Initialize(TSharedPtr<FSimpleNativeService> SimpleNative)
{
	SimpleNativeService = SimpleNative;
	return SimpleNativeService;
}

TSharedPtr<FSimpleNativeService> FExampleNative::InitializeWithExtraArgs(TSharedPtr<FSimpleNativeService> SimpleNative, FString InExtraString)
{
	ExtraString = InExtraString;
	return Initialize(SimpleNative);
}
