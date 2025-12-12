using UnrealBuildTool;

public class BPDT_Runtime : ModuleRules
{
    public BPDT_Runtime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Projects"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
