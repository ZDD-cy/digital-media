using UnrealBuildTool;
public class LanRanTarget : TargetRules { public LanRanTarget(TargetInfo Target) : base(Target) { Type=TargetType.Game; DefaultBuildSettings=BuildSettingsVersion.Latest; IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("LanRan"); } }
