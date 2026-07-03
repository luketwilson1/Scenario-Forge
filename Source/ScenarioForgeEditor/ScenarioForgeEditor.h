// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ScenarioForgeEditor.h
 * @brief Declares the ScenarioForge editor module.
 */

#pragma once

#include "Modules/ModuleInterface.h"

/**
 * @brief Registers ScenarioForge editor tabs and menu entries.
 */
class FScenarioForgeEditorModule : public IModuleInterface
{
public:
	/** Registers editor extensions. */
	virtual void StartupModule() override;

	/** Unregisters editor extensions. */
	virtual void ShutdownModule() override;

private:
	/** Registers the menu entry that opens the scenario hierarchy. */
	void RegisterMenus();

	/**
	 * @brief Creates the scenario hierarchy tab.
	 *
	 * @param SpawnTabArgs Tab spawn arguments supplied by the global tab manager.
	 * @return Spawned dock tab.
	 */
	TSharedRef<class SDockTab> SpawnScenarioHierarchyTab(const class FSpawnTabArgs& SpawnTabArgs);
};
