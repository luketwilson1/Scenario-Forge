// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file SScenarioHierarchyView.h
 * @brief Declares the Scenario hierarchy editor view.
 */

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SCompoundWidget.h"

class AScenario;
class AAgent;
class ASquad;
class ASquadGroup;
class AZone;
class AArea;
class AObjective;
class AFiringPosition;
class IDetailsView;
struct FScenarioHierarchyNode;
struct FPropertyAndParent;
struct FPropertyChangedEvent;

/** Shared pointer type for hierarchy nodes. */
typedef TSharedPtr<FScenarioHierarchyNode> FScenarioHierarchyNodePtr;

/** Types of rows shown by the scenario hierarchy. */
enum class EScenarioHierarchyNodeType : uint8
{
	Scenario,
	Folder,
	SquadsFolder,
	ZonesFolder,
	SquadGroup,
	Squad,
	Agent,
	Zone,
	Area,
	FiringPosition,
	Objective
};

/**
 * @brief Static hierarchy node used by the first-pass Scenario editor UI.
 */
struct FScenarioHierarchyNode
{
	/** Display text for this node. */
	FText Label;

	/** Type-specific behavior for this node. */
	EScenarioHierarchyNodeType Type = EScenarioHierarchyNodeType::Folder;

	/** UObject represented by this node, if any. */
	TWeakObjectPtr<UObject> Object;

	/** Squad actor represented by this node, if any. */
	TWeakObjectPtr<ASquad> Squad;

	/** Agent actor represented by this node, if any. */
	TWeakObjectPtr<AAgent> Agent;

	/** Squad that owns this node's agent, if any. */
	TWeakObjectPtr<ASquad> ParentSquad;

	/** Zone that owns this node's area, if any. */
	TWeakObjectPtr<AZone> ParentZone;

	/** Area that owns this node's firing position, if any. */
	TWeakObjectPtr<AArea> ParentArea;

	/** Child nodes displayed under this node. */
	TArray<FScenarioHierarchyNodePtr> Children;
};

/**
 * @brief Two-pane Scenario hierarchy editor shell.
 */
class SScenarioHierarchyView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioHierarchyView) {}
	SLATE_END_ARGS()

	/** Cleans up editor delegate bindings. */
	virtual ~SScenarioHierarchyView() override;

	/**
	 * @brief Builds the hierarchy view widget.
	 *
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs);

	/** Gets the squad currently targeted by viewport placement actions. */
	static ASquad* GetActivePlacementSquad();

	/** Gets the zone currently targeted by viewport placement actions. */
	static AZone* GetActivePlacementZone();

	/** Gets the area currently targeted by viewport placement actions. */
	static AArea* GetActivePlacementArea();

	/** Broadcasts that scenario hierarchy data changed outside this widget. */
	static void BroadcastScenarioDataChanged();

	/** Sets the area currently targeted by viewport firing-position placement actions. */
	static void SetActivePlacementArea(AArea* Area);

private:
	/** Sets the squad currently targeted by viewport placement actions. */
	static void SetActivePlacementSquad(ASquad* Squad);

	/** Sets the zone currently targeted by viewport placement actions. */
	static void SetActivePlacementZone(AZone* Zone);

	/** Rebuilds the full widget for the current scenario state. */
	void RebuildContent();

	/** Builds the empty state shown when no scenario actor exists. */
	TSharedRef<SWidget> BuildEmptyState();

	/** Builds the scenario hierarchy editing view. */
	TSharedRef<SWidget> BuildHierarchyView();

	/** Builds the right-side custom properties panel content for the current selection. */
	TSharedRef<SWidget> BuildPropertiesPanelContent();

	/** Returns the UObject represented by the current hierarchy selection. */
	UObject* GetSelectedDetailsObject() const;

	/** Filters properties displayed in the embedded details panel. */
	bool ShouldShowPropertyInDetails(const FPropertyAndParent& PropertyAndParent) const;

	/** Refreshes hierarchy state after details panel edits. */
	void HandleDetailsPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent);

	/** Mirrors the selected object's edited name field to its actor label. */
	bool SyncSelectedObjectLabelFromNameProperty(const FPropertyChangedEvent& PropertyChangedEvent);

	/** Refreshes the custom properties panel. */
	void RefreshPropertiesPanel();

	/** Applies PIE visibility for debug-only area and firing-position actors. */
	void ApplyDebugVisualVisibility() const;

	/** Gets the current debug checkbox state. */
	ECheckBoxState GetDebugCheckState() const;

	/** Handles changing the debug visualization toggle. */
	void OnDebugCheckStateChanged(ECheckBoxState NewState);

	/** Handles generating firing positions for the selected area. */
	FReply OnGenerateFiringPositionsClicked();

	/** Finds the first scenario actor in the current editor world. */
	AScenario* FindScenarioInEditorWorld() const;

	/** Handles clicking the create scenario button. */
	FReply OnCreateScenarioClicked();

	/** Handles clicking the add button. */
	FReply OnAddClicked();

	/** Creates a new agent in the selected squad. */
	FReply AddAgentToSelectedSquad();

	/** Creates a new zone in the active scenario. */
	FReply AddZoneToScenario();

	/** Returns whether the add button can create an item for the current selection. */
	bool CanAddToSelection() const;

	/** Handles clicking the delete button. */
	FReply OnDeleteClicked();

	/** Returns whether the delete button can delete the current selection. */
	bool CanDeleteSelection() const;

	/** Deletes selected objects and removes them from their owning scenario data arrays. */
	void DeleteSelectedObjects();

	/** Refreshes the hierarchy when the editor level changes. */
	void OnEditorMapChanged(uint32 MapChangeFlags);

	/** Refreshes the view after scenario data changes outside the widget. */
	void OnScenarioDataChanged();

	/** Handles hierarchy selection changes. */
	void OnHierarchySelectionChanged(FScenarioHierarchyNodePtr Item, ESelectInfo::Type SelectInfo);

	/** Builds the static first-pass hierarchy. */
	void RebuildHierarchy();

	/** Refreshes the tree widget after hierarchy data changes. */
	void RefreshHierarchyTree();

	/** Applies the current hierarchy selection to the details panel and editor selection. */
	void SyncSelectionToEditor();

	/**
	 * @brief Generates one hierarchy row.
	 *
	 * @param Item Hierarchy item to display.
	 * @param OwnerTable Table view that owns the row.
	 * @return Generated row widget.
	 */
	TSharedRef<class ITableRow> GenerateHierarchyRow(FScenarioHierarchyNodePtr Item, const TSharedRef<class STableViewBase>& OwnerTable) const;

	/**
	 * @brief Gets child nodes for a hierarchy item.
	 *
	 * @param Item Parent item.
	 * @param OutChildren Children displayed under the parent.
	 */
	void GetHierarchyChildren(FScenarioHierarchyNodePtr Item, TArray<FScenarioHierarchyNodePtr>& OutChildren) const;

	/** Root items displayed by the hierarchy tree. */
	TArray<FScenarioHierarchyNodePtr> RootNodes;

	/** Tree widget displaying the scenario folder hierarchy. */
	TSharedPtr<class STreeView<FScenarioHierarchyNodePtr>> HierarchyTree;

	/** Right-side panel reserved for custom properties for the selected hierarchy item. */
	TSharedPtr<class SBorder> PropertiesPanel;

	/** Details view displaying the selected scenario object's editable properties. */
	TSharedPtr<IDetailsView> DetailsView;

	/** Currently selected hierarchy node. */
	FScenarioHierarchyNodePtr SelectedNode;

	/** Scenario actor currently being edited by the hierarchy. */
	TWeakObjectPtr<AScenario> ActiveScenario;

	/** True when area volumes and firing-position debug markers should be visible in PIE. */
	bool bDebugVisualsEnabled = false;

	/** Delegate handle for editor map change refreshes. */
	FDelegateHandle MapChangeDelegateHandle;

	/** Delegate handle for scenario data refreshes. */
	FDelegateHandle ScenarioDataChangedDelegateHandle;

	/** Squad currently targeted by viewport placement actions. */
	static TWeakObjectPtr<ASquad> ActivePlacementSquad;

	/** Zone currently targeted by viewport placement actions. */
	static TWeakObjectPtr<AZone> ActivePlacementZone;

	/** Area currently targeted by viewport placement actions. */
	static TWeakObjectPtr<AArea> ActivePlacementArea;

	/** Broadcast when external scenario data changes should refresh open hierarchy widgets. */
	static FSimpleMulticastDelegate ScenarioDataChangedDelegate;
};
