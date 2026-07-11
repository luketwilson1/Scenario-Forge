// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ScenarioForgeEditor.cpp
 * @brief Implements the ScenarioForge editor module.
 */

#include "ScenarioForgeEditor.h"

#include "EditorModeRegistry.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/ActorComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "ScenarioMode/ScenarioEdMode.h"
#include "ScenarioHierarchy/SScenarioHierarchyView.h"
#include "Engine/SkeletalMesh.h"
#include "Styling/SlateIconFinder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
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
	RegisterAssetClassIcons();

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

	UnregisterAssetClassIcons();
}

/**
 * @brief Registers Scenario Forge class icons used by the Content Browser.
 */
void FScenarioForgeEditorModule::RegisterAssetClassIcons()
{
	EditorStyleSet = MakeShared<FSlateStyleSet>(TEXT("ScenarioForgeEditorStyle"));

	const auto RegisterClassIconAlias = [this](const FName TargetClassName, const UClass* SourceClass)
	{
		const FSlateBrush* IconBrush = FSlateIconFinder::FindIconBrushForClass(SourceClass);
		const FSlateBrush* ThumbnailBrush = FSlateIconFinder::FindCustomIconBrushForClass(SourceClass, TEXT("ClassThumbnail"));
		if (IconBrush)
		{
			EditorStyleSet->Set(*FString::Printf(TEXT("ClassIcon.%s"), *TargetClassName.ToString()), new FSlateBrush(*IconBrush));
		}
		if (ThumbnailBrush)
		{
			EditorStyleSet->Set(*FString::Printf(TEXT("ClassThumbnail.%s"), *TargetClassName.ToString()), new FSlateBrush(*ThumbnailBrush));
		}
	};

	RegisterClassIconAlias(TEXT("PawnCustomization"), APawn::StaticClass());
	RegisterClassIconAlias(TEXT("AgentCustomization"), ACharacter::StaticClass());
	RegisterClassIconAlias(TEXT("ProjectileCustomization"), UProjectileMovementComponent::StaticClass());
	RegisterClassIconAlias(TEXT("WeaponCustomization"), USkeletalMesh::StaticClass());
	RegisterClassIconAlias(TEXT("EquipmentCustomization"), UActorComponent::StaticClass());
	RegisterClassIconAlias(TEXT("DamageEffectCustomization"), URadialForceComponent::StaticClass());

	FSlateStyleRegistry::RegisterSlateStyle(*EditorStyleSet);
}

/**
 * @brief Unregisters Scenario Forge class icons used by the Content Browser.
 */
void FScenarioForgeEditorModule::UnregisterAssetClassIcons()
{
	if (EditorStyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*EditorStyleSet);
		EditorStyleSet.Reset();
	}
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
