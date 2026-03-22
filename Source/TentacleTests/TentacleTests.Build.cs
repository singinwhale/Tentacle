// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

using UnrealBuildTool;

public class TentacleTests : ModuleRules
{
	public TentacleTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Tentacle",
				"Core",
				"CoreUObject",
				"Engine",
			}
		);
	}
}