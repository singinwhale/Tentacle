#pragma once

struct FSimpleUStructService;
class FSimpleNativeService;

class TENTACLETESTS_API FExampleNative
{
public:
	TSharedRef<FSimpleNativeService> Initialize(TSharedRef<FSimpleNativeService> SimpleNative);
	TSharedRef<FSimpleNativeService> InitializeWithExtraArgs(TSharedRef<FSimpleNativeService> SimpleNative, FString InExtraString);
	const FSimpleUStructService& InitializeWithUStruct(const FSimpleUStructService& SimpleNative);

	TSharedPtr<FSimpleNativeService> SimpleNativeService;
	FString ExtraString;
};
