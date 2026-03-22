// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

using UnrealBuildTool;

public class Tentacle : ModuleRules
{
	public Tentacle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"AsyncStreams",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Slate",
				"SlateCore",
				"UMG",
			}
		);
	}
}