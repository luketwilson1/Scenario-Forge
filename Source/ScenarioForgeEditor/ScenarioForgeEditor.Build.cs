// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

/**
 * @brief Unreal Build Tool rules for the ScenarioForge editor module.
 */
public class ScenarioForgeEditor : ModuleRules
{
	/**
	 * @brief Configures editor module dependencies.
	 *
	 * @param Target Build target supplied by Unreal Build Tool.
	 */
	public ScenarioForgeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ScenarioForge"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EditorFramework",
			"EditorStyle",
			"LevelEditor",
			"PropertyEditor",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
