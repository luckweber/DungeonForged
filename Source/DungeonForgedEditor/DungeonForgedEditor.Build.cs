// Copyright DungeonForged. Editor-only module: widgets / tools sem puxar Blutility pelo include público do runtime.

using UnrealBuildTool;

public class DungeonForgedEditor : ModuleRules
{
	public DungeonForgedEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DungeonForged",
			"UMG",
			"Slate",
			"SlateCore",
			"Blutility",
			"EditorSubsystem",
			"DeveloperSettings",
			"UnrealEd",
			"UMGEditor",
		});

	}
}
