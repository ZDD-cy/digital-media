using UnrealBuildTool;
using System.IO;

public class TripoRuntime : ModuleRules
{
    public TripoRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // 设置为Runtime模块
        Type = ModuleType.CPlusPlus;

        // 运行时必需的模块
        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "HTTP",
                "Json"
            }
        );

        // 添加版本检测模块
        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Public")
            }
        );
    }
} 