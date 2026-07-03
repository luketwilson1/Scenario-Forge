// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ScenarioForgeEditor.cpp
 * @brief Implements the ScenarioForge editor module.
 */

#include "ScenarioForgeEditor.h"

#include "EditorModeRegistry.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "ScenarioMode/ScenarioEdMode.h"
#include "ScenarioHierarchy/SScenarioHierarchyView.h"
#include "Styling/SlateIconFinder.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "ScenarioForgeEditor"

namespace ScenarioForgeEditorTabs
{
	static const FName ScenarioHierarchyTabId(TEXT("ScenarioForge.ScenarioHierarchy"));
}

/**
 * @brief Registers editor extensions.
 */
void FScenarioForgeEditorModule::StartupModule()
{
	FEditorModeRegistry::Get().RegisterMode<FScenarioEdMode>(
		FScenarioEdMode::EM_Scenario,
		LOCTEXT("ScenarioEditorModeName", "Scenario"),
		FSlateIcon(),
		true);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ScenarioForgeEditorTabs::ScenarioHierarchyTabId,
		FOnSpawnTab::CreateRaw(this, &FScenarioForgeEditorModule::SpawnScenarioHierarchyTab))
		.SetDisplayName(LOCTEXT("ScenarioTabName", "Scenario"))
		.SetTooltipText(LOCTEXT("ScenarioTabTooltip", "Open the Scenario editor."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FScenarioForgeEditorModule::RegisterMenus));
}

/**
 * @brief Unregisters editor extensions.
 */
void FScenarioForgeEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ScenarioForgeEditorTabs::ScenarioHierarchyTabId);
	FEditorModeRegistry::Get().UnregisterMode(FScenarioEdMode::EM_Scenario);
}

/**
 * @brief Registers the Window menu entry used to open the hierarchy.
 */
void FScenarioForgeEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window"));
	FToolMenuSection& Section = WindowMenu->FindOrAddSection(TEXT("ScenarioForge"));
	Section.Label = LOCTEXT("ScenarioForgeMenuSection", "Scenario Forge");
	Section.AddMenuEntry(
		TEXT("OpenScenarioHierarchy"),
		LOCTEXT("OpenScenarioLabel", "Scenario"),
		LOCTEXT("OpenScenarioTooltip", "Open the Scenario editor."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(ScenarioForgeEditorTabs::ScenarioHierarchyTabId);
		})));
}

/**
 * @brief Creates the scenario hierarchy tab.
 *
 * @param SpawnTabArgs Tab spawn arguments supplied by the global tab manager.
 * @return Spawned dock tab.
 */
TSharedRef<SDockTab> FScenarioForgeEditorModule::SpawnScenarioHierarchyTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("ScenarioTabLabel", "Scenario"))
		[
			SNew(SScenarioHierarchyView)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FScenarioForgeEditorModule, ScenarioForgeEditor)
