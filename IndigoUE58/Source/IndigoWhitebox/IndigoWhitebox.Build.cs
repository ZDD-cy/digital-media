using UnrealBuildTool;
public class IndigoWhitebox : ModuleRules {
    public IndigoWhitebox(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]{"Core", "CoreUObject", "Engine", "InputCore"});
    }
}
