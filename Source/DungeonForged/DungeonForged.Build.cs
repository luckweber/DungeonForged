// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

// Source/DungeonForged/DungeonForged.Build.cs
// Editor dependency gating: use `if (Target.bBuildEditor)` here — C++ `WITH_EDITOR` is not available in UBT.
// Enable the CommonUI plugin in DungeonForged.uproject (CommonUI / CommonInput ship as a plugin, not always default-on).

public class DungeonForged : ModuleRules
{
	public DungeonForged(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			// Core engine
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"NetCore",

			// GAS
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",

			// Input
			"EnhancedInput",

			// UI
			"UMG",
			"Slate",
			"SlateCore",
			"CommonUI",
			"CommonInput",

			// AI & navigation
			"AIModule",
			"NavigationSystem",

			// PCG
			"PCG",

			// FX
			"Niagara",
		});

		// UBT: editor-only link dependencies (equivalent to shipping without editor modules).
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"PropertyEditor",
				"ToolMenus",
				"EditorStyle",
				"UMGEditor",
			});
		}
	}
}
