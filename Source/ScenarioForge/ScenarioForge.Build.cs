// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file ScenarioForge.Build.cs
 * @brief Defines Unreal Build Tool module dependencies for ScenarioForge.
 */

using UnrealBuildTool;

/**
 * @brief Unreal Build Tool rules for the ScenarioForge runtime module.
 */
public class ScenarioForge : ModuleRules
{
	/**
	 * @brief Configures precompiled header mode and public module dependencies.
	 *
	 * @param Target Build target supplied by Unreal Build Tool.
	 */
	public ScenarioForge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"NavigationSystem",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
