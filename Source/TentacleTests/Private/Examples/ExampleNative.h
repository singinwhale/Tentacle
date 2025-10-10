#pragma once

class FSimpleNativeService;

class TENTACLETESTS_API FExampleNative
{
public:
	TSharedPtr<FSimpleNativeService> Initialize(TSharedPtr<FSimpleNativeService> SimpleNative);
	TSharedPtr<FSimpleNativeService> InitializeWithExtraArgs(TSharedPtr<FSimpleNativeService> SimpleNative, FString InExtraString);

	TSharedPtr<FSimpleNativeService> SimpleNativeService;
	FString ExtraString;
};
