using UnrealBuildTool;
using System.IO;

public class TripoEditor : ModuleRules
{
    public TripoEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // 设置为Editor模块
        Type = ModuleType.CPlusPlus;

        // 只在编辑器目标中可用
        if (Target.Type == TargetType.Editor)
        {
            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "TripoRuntime"  // 依赖运行时模块
                }
            );

            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "UnrealEd",
                    "AssetTools",
                    "DesktopPlatform",
                    "Slate",
                    "SlateCore",
                    "ToolMenus",
                    // Referenced by name for the Tools menu extension and the
                    // tab-spawner teardown check in FTripoEditorModule.
                    "LevelEditor",
                    "Projects",
                    "ImageWrapper",
                    "ImageCore"
                }
            );

            // 添加版本检测模块
            PublicIncludePaths.AddRange(
                new string[] {
                    Path.Combine(ModuleDirectory, "../TripoRuntime/Public")
                }
            );
        }
    }
}
