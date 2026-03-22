// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#include "Tentacle.h"

#define LOCTEXT_NAMESPACE "FTentacleModule"

DEFINE_LOG_CATEGORY(LogDependencyInjection);

void FTentacleModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FTentacleModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTentacleModule, Tentacle)