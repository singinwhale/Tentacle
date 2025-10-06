using UnrealBuildTool;

public class TentacleMVVM : ModuleRules
{
    public TentacleMVVM(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "ModelViewViewModel"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Tentacle",
                "UMG",
            }
        );
    }
}