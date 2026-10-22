// Some copyright should be here...

using UnrealBuildTool;

public class Tripo3D : ModuleRules
{
	public Tripo3D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"TripoRuntime"
			}
			);
	}
}
