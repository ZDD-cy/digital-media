using UnrealBuildTool;
public class IndigoWhiteboxTarget : TargetRules {
    public IndigoWhiteboxTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("IndigoWhitebox");
    }
}
