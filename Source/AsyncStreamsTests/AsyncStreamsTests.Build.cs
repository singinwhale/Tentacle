using UnrealBuildTool;

public class AsyncStreamsTests : ModuleRules
{
    public AsyncStreamsTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Engine",
                "AsyncStreams"
            }
        );
    }
}