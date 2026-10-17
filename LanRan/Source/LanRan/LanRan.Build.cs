using UnrealBuildTool;
public class LanRan : ModuleRules { public LanRan(ReadOnlyTargetRules Target) : base(Target) { PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","IndigoInteraction"}); } }
