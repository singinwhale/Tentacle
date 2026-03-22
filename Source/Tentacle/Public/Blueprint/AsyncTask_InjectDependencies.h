// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Contexts/DiContextInterface.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/Object.h"
#include "AsyncTask_InjectDependencies.generated.h"

/**
 * 
 */
UCLASS(meta=(HasDedicatedAsyncNode))
class TENTACLE_API UAsyncTask_InjectDependencies : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "PixelStreaming"))
	static UAsyncTask_InjectDependencies* InjectDependenciesAsync(TScriptInterface<IDiContextInterface> Target, TArray<UStruct*> ClassList, TArray<FName> Names);
};
