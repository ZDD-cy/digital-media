using UnrealBuildTool;
public class IndigoWhiteboxEditorTarget : TargetRules {
    public IndigoWhiteboxEditorTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("IndigoWhitebox");
    }
}
