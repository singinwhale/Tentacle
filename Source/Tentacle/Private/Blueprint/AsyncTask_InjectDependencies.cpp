// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Blueprint/AsyncTask_InjectDependencies.h"

UAsyncTask_InjectDependencies* UAsyncTask_InjectDependencies::InjectDependenciesAsync(TScriptInterface<IDiContextInterface> Target, TArray<UStruct*> ClassList, TArray<FName> Names)
{
	UAsyncTask_InjectDependencies* Task = NewObject<UAsyncTask_InjectDependencies>();
	return Task;
}
