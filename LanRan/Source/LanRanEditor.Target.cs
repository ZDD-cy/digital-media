using UnrealBuildTool;
public class LanRanEditorTarget : TargetRules { public LanRanEditorTarget(TargetInfo Target) : base(Target) { Type=TargetType.Editor; DefaultBuildSettings=BuildSettingsVersion.Latest; IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("LanRan"); } }
