#include "ExampleNative.h"

TSharedRef<FSimpleNativeService> FExampleNative::Initialize(TSharedRef<FSimpleNativeService> SimpleNative)
{
	SimpleNativeService = SimpleNative;
	return SimpleNative;
}

TSharedRef<FSimpleNativeService> FExampleNative::InitializeWithExtraArgs(TSharedRef<FSimpleNativeService> SimpleNative, FString InExtraString)
{
	ExtraString = InExtraString;
	return Initialize(SimpleNative);
}

const FSimpleUStructService& FExampleNative::InitializeWithUStruct(const FSimpleUStructService& SimpleNative)
{
	return SimpleNative;
}
