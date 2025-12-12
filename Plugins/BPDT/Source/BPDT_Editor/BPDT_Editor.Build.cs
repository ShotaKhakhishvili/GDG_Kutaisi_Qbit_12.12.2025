using UnrealBuildTool;

public class BPDT_Editor : ModuleRules
{
    public BPDT_Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "Blutility",
            "EditorScriptingUtilities",
            "AssetRegistry",
            "ToolMenus"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
