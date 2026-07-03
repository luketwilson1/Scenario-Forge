// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file SScenarioHierarchyView.cpp
 * @brief Implements the Scenario hierarchy editor view.
 */

#include "SScenarioHierarchyView.h"

#include "Agent.h"
#include "../../ScenarioForge/Area.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "../../ScenarioForge/FiringPosition.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "../../ScenarioForge/Objective.h"
#include "PropertyEditorModule.h"
#include "Scenario.h"
#include "../ScenarioMode/ScenarioEdMode.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Styling/AppStyle.h"
#include "Squad.h"
#include "../../ScenarioForge/SquadGroup.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"
#include "../../ScenarioForge/Zone.h"

#define LOCTEXT_NAMESPACE "ScenarioHierarchyView"

TWeakObjectPtr<ASquad> SScenarioHierarchyView::ActivePlacementSquad;
TWeakObjectPtr<AZone> SScenarioHierarchyView::ActivePlacementZone;
TWeakObjectPtr<AArea> SScenarioHierarchyView::ActivePlacementArea;
FSimpleMulticastDelegate SScenarioHierarchyView::ScenarioDataChangedDelegate;

/**
 * @brief Cleans up editor delegate bindings.
 */
SScenarioHierarchyView::~SScenarioHierarchyView()
{
	if (MapChangeDelegateHandle.IsValid())
	{
		FEditorDelegates::MapChange.Remove(MapChangeDelegateHandle);
		MapChangeDelegateHandle.Reset();
	}

	if (ScenarioDataChangedDelegateHandle.IsValid())
	{
		ScenarioDataChangedDelegate.Remove(ScenarioDataChangedDelegateHandle);
		ScenarioDataChangedDelegateHandle.Reset();
	}

	if (SelectedNode.IsValid() && SelectedNode->Squad.IsValid() && ActivePlacementSquad.Get() == SelectedNode->Squad.Get())
	{
		SetActivePlacementSquad(nullptr);
	}

	if (SelectedNode.IsValid() && SelectedNode->Object.Get() == ActivePlacementZone.Get())
	{
		SetActivePlacementZone(nullptr);
	}

	if (SelectedNode.IsValid() && SelectedNode->Object.Get() == ActivePlacementArea.Get())
	{
		SetActivePlacementArea(nullptr);
	}

}

/**
 * @brief Builds the hierarchy view widget.
 *
 * @param InArgs Slate construction arguments.
 */
void SScenarioHierarchyView::Construct(const FArguments& InArgs)
{
	MapChangeDelegateHandle = FEditorDelegates::MapChange.AddSP(this, &SScenarioHierarchyView::OnEditorMapChanged);
	ScenarioDataChangedDelegateHandle = ScenarioDataChangedDelegate.AddSP(this, &SScenarioHierarchyView::OnScenarioDataChanged);

	ActiveScenario = FindScenarioInEditorWorld();
	RebuildContent();
}

/**
 * @brief Gets the squad currently targeted by viewport placement actions.
 *
 * @return Active placement squad, or nullptr.
 */
ASquad* SScenarioHierarchyView::GetActivePlacementSquad()
{
	return ActivePlacementSquad.Get();
}

/**
 * @brief Gets the zone currently targeted by viewport placement actions.
 *
 * @return Active placement zone, or nullptr.
 */
AZone* SScenarioHierarchyView::GetActivePlacementZone()
{
	return ActivePlacementZone.Get();
}

/**
 * @brief Gets the area currently targeted by viewport placement actions.
 *
 * @return Active placement area, or nullptr.
 */
AArea* SScenarioHierarchyView::GetActivePlacementArea()
{
	return ActivePlacementArea.Get();
}

/**
 * @brief Broadcasts that scenario hierarchy data changed outside this widget.
 */
void SScenarioHierarchyView::BroadcastScenarioDataChanged()
{
	ScenarioDataChangedDelegate.Broadcast();
}

/**
 * @brief Sets the squad currently targeted by viewport placement actions.
 *
 * @param Squad Squad to target, or nullptr to clear placement.
 */
void SScenarioHierarchyView::SetActivePlacementSquad(ASquad* Squad)
{
	ActivePlacementSquad = Squad;
	if (Squad)
	{
		ActivePlacementZone = nullptr;
		ActivePlacementArea = nullptr;
	}

	if (Squad)
	{
		if (!GLevelEditorModeTools().IsModeActive(FScenarioEdMode::EM_Scenario))
		{
			GLevelEditorModeTools().ActivateMode(FScenarioEdMode::EM_Scenario);
		}
	}
	else if (!ActivePlacementZone.IsValid() && !ActivePlacementArea.IsValid() && GLevelEditorModeTools().IsModeActive(FScenarioEdMode::EM_Scenario))
	{
		GLevelEditorModeTools().DeactivateMode(FScenarioEdMode::EM_Scenario);
	}
}

/**
 * @brief Sets the zone currently targeted by viewport placement actions.
 *
 * @param Zone Zone to target, or nullptr to clear placement.
 */
void SScenarioHierarchyView::SetActivePlacementZone(AZone* Zone)
{
	ActivePlacementZone = Zone;
	if (Zone)
	{
		ActivePlacementSquad = nullptr;
		ActivePlacementArea = nullptr;
	}

	if (Zone)
	{
		if (!GLevelEditorModeTools().IsModeActive(FScenarioEdMode::EM_Scenario))
		{
			GLevelEditorModeTools().ActivateMode(FScenarioEdMode::EM_Scenario);
		}
	}
	else if (!ActivePlacementSquad.IsValid() && !ActivePlacementArea.IsValid() && GLevelEditorModeTools().IsModeActive(FScenarioEdMode::EM_Scenario))
	{
		GLevelEditorModeTools().DeactivateMode(FScenarioEdMode::EM_Scenario);
	}
}

/**
 * @brief Sets the area currently targeted by viewport placement actions.
 *
 * @param Area Area to target, or nullptr to clear placement.
 */
void SScenarioHierarchyView::SetActivePlacementArea(AArea* Area)
{
	ActivePlacementArea = Area;
	if (Area)
	{
		ActivePlacementSquad = nullptr;
		ActivePlacementZone = nullptr;
	}

	if (Area)
	{
		if (!GLevelEditorModeTools().IsModeActive(FScenarioEdMode::EM_Scenario))
		{
			GLevelEditorModeTools().ActivateMode(FScenarioEdMode::EM_Scenario);
		}
	}
	else if (!ActivePlacementSquad.IsValid() && !ActivePlacementZone.IsValid() && GLevelEditorModeTools().IsModeActive(FScenarioEdMode::EM_Scenario))
	{
		GLevelEditorModeTools().DeactivateMode(FScenarioEdMode::EM_Scenario);
	}
}

/**
 * @brief Rebuilds the full widget for the current scenario state.
 */
void SScenarioHierarchyView::RebuildContent()
{
	if (!ActiveScenario.IsValid())
	{
		ActiveScenario = FindScenarioInEditorWorld();
	}

	HierarchyTree.Reset();
	PropertiesPanel.Reset();

	ChildSlot
	[
		ActiveScenario.IsValid()
			? BuildHierarchyView()
			: BuildEmptyState()
	];
}

/**
 * @brief Builds the empty state shown when no scenario actor exists.
 *
 * @return Empty state widget.
 */
TSharedRef<SWidget> SScenarioHierarchyView::BuildEmptyState()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
		.Padding(24.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSpacer)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoScenarioTitle", "No Scenario in this level"))
				.Font(FAppStyle::GetFontStyle(TEXT("HeadingMedium")))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateScenarioButton", "Create Scenario"))
				.OnClicked(this, &SScenarioHierarchyView::OnCreateScenarioClicked)
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSpacer)
			]
		];
}

/**
 * @brief Builds the scenario hierarchy editing view.
 *
 * @return Hierarchy editing widget.
 */
TSharedRef<SWidget> SScenarioHierarchyView::BuildHierarchyView()
{
	RebuildHierarchy();

	TSharedRef<SWidget> HierarchyView =
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			.Padding(FMargin(6.0f, 4.0f))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SBox)
					.MinDesiredWidth(96.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("AddButton", "Add"))
						.OnClicked(this, &SScenarioHierarchyView::OnAddClicked)
						.IsEnabled(this, &SScenarioHierarchyView::CanAddToSelection)
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(96.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("DeleteButton", "Delete"))
						.OnClicked(this, &SScenarioHierarchyView::OnDeleteClicked)
						.IsEnabled(this, &SScenarioHierarchyView::CanDeleteSelection)
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(16.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked(this, &SScenarioHierarchyView::GetDebugCheckState)
					.OnCheckStateChanged(this, &SScenarioHierarchyView::OnDebugCheckStateChanged)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DebugToggleLabel", "Debug"))
					]
				]
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)

			+ SSplitter::Slot()
			.Value(0.62f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
				.Padding(0.0f)
				[
					SAssignNew(HierarchyTree, STreeView<FScenarioHierarchyNodePtr>)
					.TreeItemsSource(&RootNodes)
					.OnGenerateRow(this, &SScenarioHierarchyView::GenerateHierarchyRow)
					.OnGetChildren(this, &SScenarioHierarchyView::GetHierarchyChildren)
					.OnSelectionChanged(this, &SScenarioHierarchyView::OnHierarchySelectionChanged)
					.SelectionMode(ESelectionMode::Multi)
				]
			]

			+ SSplitter::Slot()
			.Value(0.38f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
				.Padding(FMargin(10.0f, 8.0f))
				[
					SAssignNew(PropertiesPanel, SBorder)
					.BorderImage(FStyleDefaults::GetNoBrush())
					.Padding(0.0f)
					[
						BuildPropertiesPanelContent()
					]
				]
			]
		]
	;

	if (HierarchyTree.IsValid())
	{
		TArray<FScenarioHierarchyNodePtr> NodesToExpand = RootNodes;
		while (NodesToExpand.Num() > 0)
		{
			const FScenarioHierarchyNodePtr Node = NodesToExpand.Pop(EAllowShrinking::No);
			if (!Node.IsValid())
			{
				continue;
			}

			HierarchyTree->SetItemExpansion(Node, true);
			NodesToExpand.Append(Node->Children);
		}
	}

	SyncSelectionToEditor();

	return HierarchyView;
}

/**
 * @brief Builds the right-side custom properties panel content for the current selection.
 *
 * @return Custom properties panel content.
 */
TSharedRef<SWidget> SScenarioHierarchyView::BuildPropertiesPanelContent()
{
	FText Header = LOCTEXT("PropertiesHeader", "Properties");
	FText SubHeader = LOCTEXT("NoPropertiesSelection", "Select a scenario object");
	UObject* DetailsObject = GetSelectedDetailsObject();

	if (SelectedNode.IsValid())
	{
		if (SelectedNode->Type == EScenarioHierarchyNodeType::Scenario && ActiveScenario.IsValid())
		{
			Header = FText::FromString(ActiveScenario->GetActorLabel());
			SubHeader = LOCTEXT("ScenarioPropertiesSubHeader", "Scenario");
		}
		else if (SelectedNode->Type == EScenarioHierarchyNodeType::Squad && SelectedNode->Squad.IsValid())
		{
			Header = FText::FromString(SelectedNode->Squad->GetActorLabel());
			SubHeader = LOCTEXT("SquadPropertiesSubHeader", "Squad");
		}
		else if (SelectedNode->Type == EScenarioHierarchyNodeType::Agent && SelectedNode->Agent.IsValid())
		{
			Header = FText::FromString(SelectedNode->Agent->GetActorLabel());
			SubHeader = LOCTEXT("AgentPropertiesSubHeader", "Agent");
		}
		else if (AActor* SelectedActor = Cast<AActor>(SelectedNode->Object.Get()))
		{
			Header = FText::FromString(SelectedActor->GetActorLabel());
			SubHeader = FText::FromName(SelectedActor->GetClass()->GetFName());
		}
	}

	if (!DetailsView.IsValid())
	{
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bShowObjectLabel = false;
		DetailsViewArgs.bShowPropertyMatrixButton = false;
		DetailsViewArgs.bUpdatesFromSelection = false;

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
		DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SScenarioHierarchyView::ShouldShowPropertyInDetails));
		DetailsView->OnFinishedChangingProperties().AddSP(this, &SScenarioHierarchyView::HandleDetailsPropertyChanged);
	}

	DetailsView->SetObject(DetailsObject);
	const bool bSelectedArea = Cast<AArea>(DetailsObject) != nullptr;

	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(Header)
			.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(SubHeader)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility(bSelectedArea ? EVisibility::Visible : EVisibility::Collapsed)
			[
				SNew(SButton)
				.Text(LOCTEXT("GenerateFiringPositionsButton", "Generate Firing Positions"))
				.OnClicked(this, &SScenarioHierarchyView::OnGenerateFiringPositionsClicked)
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			DetailsView.ToSharedRef()
		];
}

/**
 * @brief Gets the UObject represented by the current hierarchy selection.
 *
 * @return Selected scenario object, or nullptr.
 */
UObject* SScenarioHierarchyView::GetSelectedDetailsObject() const
{
	if (!SelectedNode.IsValid())
	{
		return nullptr;
	}

	if (SelectedNode->Object.IsValid())
	{
		return SelectedNode->Object.Get();
	}

	if (SelectedNode->Type == EScenarioHierarchyNodeType::Scenario && ActiveScenario.IsValid())
	{
		return ActiveScenario.Get();
	}

	if (SelectedNode->Type == EScenarioHierarchyNodeType::Squad && SelectedNode->Squad.IsValid())
	{
		return SelectedNode->Squad.Get();
	}

	if (SelectedNode->Type == EScenarioHierarchyNodeType::Agent && SelectedNode->Agent.IsValid())
	{
		return SelectedNode->Agent.Get();
	}

	return nullptr;
}

/**
 * @brief Filters properties shown in the embedded details panel.
 *
 * @param PropertyAndParent Property under consideration.
 * @return True when the property should be visible.
 */
bool SScenarioHierarchyView::ShouldShowPropertyInDetails(const FPropertyAndParent& PropertyAndParent) const
{
	const UObject* DetailsObject = GetSelectedDetailsObject();
	if (!DetailsObject)
	{
		return false;
	}

	const FProperty* RootProperty = PropertyAndParent.ParentProperties.Num() > 0
		? PropertyAndParent.ParentProperties.Last()
		: &PropertyAndParent.Property;
	if (!RootProperty)
	{
		return false;
	}

	const UClass* DetailsClass = DetailsObject->GetClass();
	if (RootProperty->GetOwnerClass() != DetailsClass)
	{
		return false;
	}

	const FName RootPropertyName = RootProperty->GetFName();
	static const TSet<FName> HiddenScenarioPropertyNames =
	{
		GET_MEMBER_NAME_CHECKED(ASquad, Agents),
		GET_MEMBER_NAME_CHECKED(AZone, Areas),
		GET_MEMBER_NAME_CHECKED(AArea, AreaMeshComponent),
		GET_MEMBER_NAME_CHECKED(AArea, AreaBaseColor),
		GET_MEMBER_NAME_CHECKED(AArea, Opacity),
		GET_MEMBER_NAME_CHECKED(AArea, FiringPositions),
		GET_MEMBER_NAME_CHECKED(AFiringPosition, FiringPositionMeshComponent),
		GET_MEMBER_NAME_CHECKED(AFiringPosition, FiringPositionBaseColor),
		GET_MEMBER_NAME_CHECKED(AFiringPosition, Opacity),
		TEXT("AbilitySystemComponent"),
		TEXT("AgentAttributeSet"),
		TEXT("EquippedWeapon"),
		TEXT("StowedWeapon")
	};

	if (HiddenScenarioPropertyNames.Contains(RootPropertyName))
	{
		return false;
	}

	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(RootProperty))
	{
		if (ObjectProperty->PropertyClass && ObjectProperty->PropertyClass->IsChildOf<UActorComponent>())
		{
			return false;
		}
	}

	return RootProperty->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible);
}

/**
 * @brief Handles generating firing positions for the selected area.
 *
 * @return Reply for the handled click.
 */
FReply SScenarioHierarchyView::OnGenerateFiringPositionsClicked()
{
	AArea* SelectedArea = Cast<AArea>(GetSelectedDetailsObject());
	if (!SelectedArea)
	{
		return FReply::Handled();
	}

	SelectedArea->GenerateFiringPositions();
	RebuildHierarchy();
	RefreshHierarchyTree();
	SScenarioHierarchyView::BroadcastScenarioDataChanged();

	return FReply::Handled();
}

/**
 * @brief Refreshes hierarchy state after details panel edits.
 *
 * @param PropertyChangedEvent Details panel property change event.
 */
void SScenarioHierarchyView::HandleDetailsPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	if (SyncSelectedObjectLabelFromNameProperty(PropertyChangedEvent) && SelectedNode.IsValid())
	{
		if (AActor* SelectedActor = Cast<AActor>(GetSelectedDetailsObject()))
		{
			SelectedNode->Label = FText::FromString(SelectedActor->GetActorLabel());
		}

		RefreshHierarchyTree();
		RefreshPropertiesPanel();
	}
}

/**
 * @brief Mirrors the selected object's edited name field to its actor label.
 *
 * @param PropertyChangedEvent Details panel property change event.
 * @return True if a hierarchy-facing label changed.
 */
bool SScenarioHierarchyView::SyncSelectedObjectLabelFromNameProperty(const FPropertyChangedEvent& PropertyChangedEvent)
{
	static const TSet<FName> HierarchyNameProperties =
	{
		TEXT("SquadGroupName"),
		TEXT("SquadName"),
		TEXT("AgentName"),
		TEXT("ZoneName"),
		TEXT("AreaName"),
		TEXT("ObjectiveName")
	};

	if (!PropertyChangedEvent.Property || !HierarchyNameProperties.Contains(PropertyChangedEvent.Property->GetFName()))
	{
		return false;
	}

	AActor* SelectedActor = Cast<AActor>(GetSelectedDetailsObject());
	if (!SelectedActor)
	{
		return false;
	}

	const void* ValueAddress = PropertyChangedEvent.Property->ContainerPtrToValuePtr<void>(SelectedActor);
	FString NewLabel;

	if (const FStrProperty* StringProperty = CastField<FStrProperty>(PropertyChangedEvent.Property))
	{
		NewLabel = StringProperty->GetPropertyValue(ValueAddress);
	}
	else if (const FNameProperty* NameProperty = CastField<FNameProperty>(PropertyChangedEvent.Property))
	{
		NewLabel = NameProperty->GetPropertyValue(ValueAddress).ToString();
	}

	if (NewLabel.IsEmpty())
	{
		return false;
	}

	SelectedActor->SetActorLabel(NewLabel);
	return true;
}

/**
 * @brief Refreshes the custom properties panel.
 */
void SScenarioHierarchyView::RefreshPropertiesPanel()
{
	if (PropertiesPanel.IsValid())
	{
		PropertiesPanel->SetContent(BuildPropertiesPanelContent());
	}
}

/**
 * @brief Applies runtime debug visibility for area volumes and firing positions.
 */
void SScenarioHierarchyView::ApplyDebugVisualVisibility() const
{
	if (!ActiveScenario.IsValid())
	{
		return;
	}

	const bool bHideInGame = !bDebugVisualsEnabled;
	for (AZone* Zone : ActiveScenario->Zones)
	{
		if (!IsValid(Zone))
		{
			continue;
		}

		for (AArea* Area : Zone->Areas)
		{
			if (!IsValid(Area))
			{
				continue;
			}

			Area->SetActorHiddenInGame(bHideInGame);
			if (Area->AreaMeshComponent)
			{
				Area->AreaMeshComponent->SetHiddenInGame(bHideInGame);
				Area->AreaMeshComponent->SetVisibility(true, true);
			}

			for (AFiringPosition* FiringPosition : Area->FiringPositions)
			{
				if (IsValid(FiringPosition))
				{
					FiringPosition->SetActorHiddenInGame(bHideInGame);
					if (FiringPosition->FiringPositionMeshComponent)
					{
						FiringPosition->FiringPositionMeshComponent->SetHiddenInGame(bHideInGame);
						FiringPosition->FiringPositionMeshComponent->SetVisibility(true, true);
					}
				}
			}
		}
	}

	if (GEditor)
	{
		GEditor->RedrawAllViewports();
	}
}

/**
 * @brief Gets the current debug checkbox state.
 *
 * @return Checked when debug visuals are enabled.
 */
ECheckBoxState SScenarioHierarchyView::GetDebugCheckState() const
{
	return bDebugVisualsEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

/**
 * @brief Handles changing the debug visualization toggle.
 *
 * @param NewState New checkbox state.
 */
void SScenarioHierarchyView::OnDebugCheckStateChanged(ECheckBoxState NewState)
{
	bDebugVisualsEnabled = NewState == ECheckBoxState::Checked;
	ApplyDebugVisualVisibility();
}

/**
 * @brief Finds the first scenario actor in the current editor world.
 *
 * @return Scenario actor if one exists; otherwise nullptr.
 */
AScenario* SScenarioHierarchyView::FindScenarioInEditorWorld() const
{
	if (!GEditor)
	{
		return nullptr;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return nullptr;
	}

	for (TActorIterator<AScenario> It(EditorWorld); It; ++It)
	{
		if (!It->IsPendingKillPending())
		{
			return *It;
		}
	}

	return nullptr;
}

/**
 * @brief Handles clicking the create scenario button.
 *
 * @return Reply for the handled click.
 */
FReply SScenarioHierarchyView::OnCreateScenarioClicked()
{
	if (!GEditor)
	{
		return FReply::Handled();
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return FReply::Handled();
	}

	if (AScenario* ExistingScenario = FindScenarioInEditorWorld())
	{
		ActiveScenario = ExistingScenario;
		RebuildContent();
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateScenarioTransaction", "Create Scenario"));
	EditorWorld->Modify();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = TEXT("Scenario");
	SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	SpawnParameters.ObjectFlags = RF_Transactional;

	AScenario* NewScenario = EditorWorld->SpawnActor<AScenario>(AScenario::StaticClass(), FTransform::Identity, SpawnParameters);
	if (NewScenario)
	{
		NewScenario->SetActorLabel(TEXT("Scenario"));
		NewScenario->Modify();

		GEditor->SelectNone(false, true);
		GEditor->SelectActor(NewScenario, true, true, true);

		ActiveScenario = NewScenario;
		RebuildContent();
	}

	return FReply::Handled();
}

/**
 * @brief Handles clicking the add button.
 *
 * @return Reply for the handled click.
 */
FReply SScenarioHierarchyView::OnAddClicked()
{
	if (!CanAddToSelection() || !ActiveScenario.IsValid() || !GEditor)
	{
		return FReply::Handled();
	}

	if (SelectedNode->Type == EScenarioHierarchyNodeType::ZonesFolder)
	{
		return AddZoneToScenario();
	}

	if (SelectedNode->Type == EScenarioHierarchyNodeType::Squad)
	{
		return AddAgentToSelectedSquad();
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateSquadTransaction", "Create Squad"));
	EditorWorld->Modify();
	ActiveScenario->Modify();

	const int32 NewSquadNumber = ActiveScenario->Squads.Num() + 1;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transactional;

	ASquad* NewSquad = EditorWorld->SpawnActor<ASquad>(ASquad::StaticClass(), FTransform::Identity, SpawnParameters);
	if (NewSquad)
	{
		const FString SquadLabel = FString::Printf(TEXT("Squad %d"), NewSquadNumber);
		NewSquad->SetActorLabel(SquadLabel);
		NewSquad->Modify();

		ActiveScenario->Squads.Add(NewSquad);

		GEditor->SelectNone(false, true);
		GEditor->SelectActor(NewSquad, true, true, true);

		RebuildHierarchy();
		RefreshHierarchyTree();
	}

	return FReply::Handled();
}

/**
 * @brief Returns whether the add button can create an item for the current selection.
 *
 * @return True if add can create an item for the selected node.
 */
bool SScenarioHierarchyView::CanAddToSelection() const
{
	return SelectedNode.IsValid()
		&& (SelectedNode->Type == EScenarioHierarchyNodeType::SquadsFolder
			|| SelectedNode->Type == EScenarioHierarchyNodeType::Squad
			|| SelectedNode->Type == EScenarioHierarchyNodeType::ZonesFolder);
}

/**
 * @brief Creates a new zone in the active scenario.
 *
 * @return Reply for the handled click.
 */
FReply SScenarioHierarchyView::AddZoneToScenario()
{
	if (!ActiveScenario.IsValid() || !GEditor)
	{
		return FReply::Handled();
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateZoneTransaction", "Create Zone"));
	EditorWorld->Modify();
	ActiveScenario->Modify();

	const int32 NewZoneNumber = ActiveScenario->Zones.Num() + 1;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transactional;

	AZone* NewZone = EditorWorld->SpawnActor<AZone>(AZone::StaticClass(), FTransform::Identity, SpawnParameters);
	if (NewZone)
	{
		const FString ZoneLabel = FString::Printf(TEXT("Zone %d"), NewZoneNumber);
		NewZone->ZoneName = ZoneLabel;
		NewZone->SetActorLabel(ZoneLabel);
		NewZone->Modify();

		ActiveScenario->Zones.Add(NewZone);

		GEditor->SelectNone(false, true);
		GEditor->SelectActor(NewZone, true, true, true);

		RebuildHierarchy();
		RefreshHierarchyTree();
		SScenarioHierarchyView::BroadcastScenarioDataChanged();
	}

	return FReply::Handled();
}

/**
 * @brief Creates a new agent in the selected squad.
 *
 * @return Reply for the handled click.
 */
FReply SScenarioHierarchyView::AddAgentToSelectedSquad()
{
	ASquad* SelectedSquad = SelectedNode.IsValid() ? SelectedNode->Squad.Get() : nullptr;
	if (!IsValid(SelectedSquad) || !GEditor)
	{
		return FReply::Handled();
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateAgentTransaction", "Create Agent"));
	EditorWorld->Modify();
	SelectedSquad->Modify();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transactional;

	const FVector SpawnLocation = SelectedSquad->GetActorLocation();
	AAgent* NewAgent = EditorWorld->SpawnActor<AAgent>(AAgent::StaticClass(), SpawnLocation, SelectedSquad->GetActorRotation(), SpawnParameters);
	if (!NewAgent)
	{
		return FReply::Handled();
	}

	const FString AgentLabel = FString::Printf(TEXT("%s Agent %d"), *SelectedSquad->GetActorLabel(), SelectedSquad->Agents.Num() + 1);
	NewAgent->SetAgentName(AgentLabel);
	NewAgent->Modify();

	SelectedSquad->Agents.Add(NewAgent);

	GEditor->SelectNone(false, true);
	GEditor->SelectActor(NewAgent, true, true, true);

	RebuildHierarchy();
	RefreshHierarchyTree();
	SScenarioHierarchyView::BroadcastScenarioDataChanged();

	return FReply::Handled();
}

/**
 * @brief Handles clicking the delete button.
 *
 * @return Reply for the handled click.
 */
FReply SScenarioHierarchyView::OnDeleteClicked()
{
	if (!CanDeleteSelection() || !ActiveScenario.IsValid() || !GEditor)
	{
		return FReply::Handled();
	}

	DeleteSelectedObjects();

	return FReply::Handled();
}

/**
 * @brief Returns whether the delete button can delete the current selection.
 *
 * @return True if delete can remove the selected node.
 */
bool SScenarioHierarchyView::CanDeleteSelection() const
{
	if (HierarchyTree.IsValid())
	{
		for (const FScenarioHierarchyNodePtr& Node : HierarchyTree->GetSelectedItems())
		{
			if (Node.IsValid() && Node->Object.IsValid())
			{
				return true;
			}
		}
	}

	return SelectedNode.IsValid() && SelectedNode->Object.IsValid();
}

/**
 * @brief Deletes selected objects and removes them from their owning scenario data arrays.
 */
void SScenarioHierarchyView::DeleteSelectedObjects()
{
	TArray<FScenarioHierarchyNodePtr> NodesToDelete = HierarchyTree.IsValid()
		? HierarchyTree->GetSelectedItems()
		: TArray<FScenarioHierarchyNodePtr>();

	if (NodesToDelete.IsEmpty() && SelectedNode.IsValid())
	{
		NodesToDelete.Add(SelectedNode);
	}

	NodesToDelete.RemoveAll([](const FScenarioHierarchyNodePtr& Node)
	{
		return !Node.IsValid() || !Node->Object.IsValid();
	});

	if (NodesToDelete.IsEmpty())
	{
		return;
	}

	auto IsNodeCoveredByAnotherSelection = [&NodesToDelete](const FScenarioHierarchyNodePtr& Node)
	{
		if (!Node.IsValid())
		{
			return true;
		}

		for (const FScenarioHierarchyNodePtr& CandidateParent : NodesToDelete)
		{
			if (!CandidateParent.IsValid() || CandidateParent == Node)
			{
				continue;
			}

			if (CandidateParent->Type == EScenarioHierarchyNodeType::Scenario)
			{
				return true;
			}

			if (Node->Type == EScenarioHierarchyNodeType::Agent
				&& CandidateParent->Type == EScenarioHierarchyNodeType::Squad
				&& Node->ParentSquad.Get() == CandidateParent->Object.Get())
			{
				return true;
			}

			if (Node->Type == EScenarioHierarchyNodeType::Area
				&& CandidateParent->Type == EScenarioHierarchyNodeType::Zone
				&& Node->ParentZone.Get() == CandidateParent->Object.Get())
			{
				return true;
			}

			if (Node->Type == EScenarioHierarchyNodeType::FiringPosition)
			{
				if (CandidateParent->Type == EScenarioHierarchyNodeType::Area
					&& Node->ParentArea.Get() == CandidateParent->Object.Get())
				{
					return true;
				}

				if (CandidateParent->Type == EScenarioHierarchyNodeType::Zone)
				{
					const AZone* CandidateZone = Cast<AZone>(CandidateParent->Object.Get());
					if (CandidateZone && CandidateZone->Areas.Contains(Node->ParentArea.Get()))
					{
						return true;
					}
				}
			}
		}

		return false;
	};

	NodesToDelete.RemoveAll(IsNodeCoveredByAnotherSelection);

	if (NodesToDelete.IsEmpty())
	{
		return;
	}

	UObject* FirstObjectToDelete = NodesToDelete[0]->Object.Get();
	AActor* FirstActorToDelete = Cast<AActor>(FirstObjectToDelete);
	UWorld* EditorWorld = FirstActorToDelete ? FirstActorToDelete->GetWorld() : GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteHierarchyObjectTransaction", "Delete Scenario Object"));
	EditorWorld->Modify();

	if (ActiveScenario.IsValid())
	{
		ActiveScenario->Modify();
	}

	auto DestroyActor = [EditorWorld](AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return;
		}

		Actor->Modify();
		if (GEditor)
		{
			GEditor->SelectActor(Actor, false, true, true);
		}
		EditorWorld->EditorDestroyActor(Actor, true);
	};

	bool bDeletingScenario = false;

	auto DeleteOneNode = [&](const FScenarioHierarchyNodePtr& Node)
	{
		if (!Node.IsValid())
		{
			return;
		}

		UObject* ObjectToDelete = Node->Object.Get();
		AActor* ActorToDelete = Cast<AActor>(ObjectToDelete);

		switch (Node->Type)
		{
	case EScenarioHierarchyNodeType::Scenario:
		if (ActorToDelete == ActiveScenario.Get())
		{
			bDeletingScenario = true;
			SetActivePlacementSquad(nullptr);
			SetActivePlacementZone(nullptr);
			SetActivePlacementArea(nullptr);
			DestroyActor(ActorToDelete);
			ActiveScenario.Reset();
		}
		break;

	case EScenarioHierarchyNodeType::SquadGroup:
		ActiveScenario->SquadGroups.Remove(Cast<ASquadGroup>(ObjectToDelete));
		DestroyActor(ActorToDelete);
		break;

	case EScenarioHierarchyNodeType::Squad:
		if (ASquad* SquadToDelete = Cast<ASquad>(ObjectToDelete))
		{
			ActiveScenario->Squads.Remove(SquadToDelete);
			for (AAgent* Agent : SquadToDelete->Agents)
			{
				DestroyActor(Agent);
			}
			SquadToDelete->Agents.Reset();
			DestroyActor(SquadToDelete);
		}
		break;

	case EScenarioHierarchyNodeType::Agent:
		if (ASquad* OwningSquad = Node->ParentSquad.Get())
		{
			OwningSquad->Modify();
			OwningSquad->Agents.Remove(Cast<AAgent>(ObjectToDelete));
		}
		DestroyActor(ActorToDelete);
		break;

	case EScenarioHierarchyNodeType::Zone:
		if (AZone* ZoneToDelete = Cast<AZone>(ObjectToDelete))
		{
			SetActivePlacementZone(nullptr);
			ActiveScenario->Zones.Remove(ZoneToDelete);
			for (AArea* Area : ZoneToDelete->Areas)
			{
				if (IsValid(Area))
				{
					for (AFiringPosition* FiringPosition : Area->FiringPositions)
					{
						DestroyActor(FiringPosition);
					}
					Area->FiringPositions.Reset();
				}
				DestroyActor(Area);
			}
			ZoneToDelete->Areas.Reset();
			DestroyActor(ZoneToDelete);
		}
		break;

	case EScenarioHierarchyNodeType::Area:
		if (AArea* AreaToDelete = Cast<AArea>(ObjectToDelete))
		{
			if (AZone* OwningZone = Node->ParentZone.Get())
			{
				OwningZone->Modify();
				OwningZone->Areas.Remove(AreaToDelete);
			}
			for (AFiringPosition* FiringPosition : AreaToDelete->FiringPositions)
			{
				DestroyActor(FiringPosition);
			}
			AreaToDelete->FiringPositions.Reset();
		}
		DestroyActor(ActorToDelete);
		break;

	case EScenarioHierarchyNodeType::FiringPosition:
		if (AArea* OwningArea = Node->ParentArea.Get())
		{
			OwningArea->Modify();
			OwningArea->FiringPositions.Remove(Cast<AFiringPosition>(ObjectToDelete));
		}
		DestroyActor(ActorToDelete);
		break;

	case EScenarioHierarchyNodeType::Objective:
		ActiveScenario->Objectives.Remove(Cast<AObjective>(ObjectToDelete));
		DestroyActor(ActorToDelete);
		break;

	default:
		if (ActorToDelete)
		{
			DestroyActor(ActorToDelete);
		}
		break;
	}
	};

	for (const FScenarioHierarchyNodePtr& Node : NodesToDelete)
	{
		DeleteOneNode(Node);
	}

	SelectedNode.Reset();
	if (bDeletingScenario)
	{
		RebuildContent();
	}
	else
	{
		RebuildHierarchy();
		RefreshHierarchyTree();
		RefreshPropertiesPanel();
	}
	SScenarioHierarchyView::BroadcastScenarioDataChanged();
}

/**
 * @brief Refreshes the hierarchy when the editor level changes.
 *
 * @param MapChangeFlags Flags describing the editor map change.
 */
void SScenarioHierarchyView::OnEditorMapChanged(uint32 MapChangeFlags)
{
	ActiveScenario.Reset();
	SelectedNode.Reset();
	SetActivePlacementSquad(nullptr);
	SetActivePlacementZone(nullptr);
	SetActivePlacementArea(nullptr);
	RebuildContent();
}

/**
 * @brief Refreshes the view after scenario data changes outside the widget.
 */
void SScenarioHierarchyView::OnScenarioDataChanged()
{
	UObject* PreviouslySelectedObject = nullptr;
	if (SelectedNode.IsValid())
	{
		if (SelectedNode->Type == EScenarioHierarchyNodeType::Squad)
		{
			PreviouslySelectedObject = SelectedNode->Squad.Get();
		}
		else if (SelectedNode->Type == EScenarioHierarchyNodeType::Agent)
		{
			PreviouslySelectedObject = SelectedNode->Agent.Get();
		}
		else
		{
			PreviouslySelectedObject = SelectedNode->Object.Get();
		}
	}

	RebuildHierarchy();
	RefreshHierarchyTree();
	ApplyDebugVisualVisibility();

	if (PreviouslySelectedObject && HierarchyTree.IsValid())
	{
		TArray<FScenarioHierarchyNodePtr> NodesToVisit = RootNodes;
		while (NodesToVisit.Num() > 0)
		{
			const FScenarioHierarchyNodePtr Candidate = NodesToVisit.Pop(EAllowShrinking::No);
			if (!Candidate.IsValid())
			{
				continue;
			}

			if (Candidate->Object.Get() == PreviouslySelectedObject
				|| Candidate->Squad.Get() == PreviouslySelectedObject
				|| Candidate->Agent.Get() == PreviouslySelectedObject)
			{
				SelectedNode = Candidate;
				HierarchyTree->SetSelection(Candidate);
				break;
			}

			NodesToVisit.Append(Candidate->Children);
		}
	}

	SyncSelectionToEditor();
}

/**
 * @brief Handles hierarchy selection changes.
 *
 * @param Item Selected hierarchy item.
 * @param SelectInfo Selection source.
 */
void SScenarioHierarchyView::OnHierarchySelectionChanged(FScenarioHierarchyNodePtr Item, ESelectInfo::Type SelectInfo)
{
	SelectedNode = Item;
	SyncSelectionToEditor();
}

/**
 * @brief Applies the current hierarchy selection to the details panel and editor selection.
 */
void SScenarioHierarchyView::SyncSelectionToEditor()
{
	if (SelectedNode.IsValid() && SelectedNode->Type == EScenarioHierarchyNodeType::Squad)
	{
		ASquad* Squad = SelectedNode->Squad.Get();
		SetActivePlacementSquad(Squad);

		if (GEditor && IsValid(Squad))
		{
			GEditor->SelectNone(false, true);
			GEditor->SelectActor(Squad, true, true, true);
		}
	}
	else if (SelectedNode.IsValid() && SelectedNode->Type == EScenarioHierarchyNodeType::Agent)
	{
		AAgent* Agent = SelectedNode->Agent.Get();
		SetActivePlacementSquad(nullptr);
		SetActivePlacementZone(nullptr);
		SetActivePlacementArea(nullptr);

		if (GEditor && IsValid(Agent))
		{
			GEditor->SelectNone(false, true);
			GEditor->SelectActor(Agent, true, true, true);
		}
	}
	else if (SelectedNode.IsValid())
	{
		AActor* SelectedActor = Cast<AActor>(SelectedNode->Object.Get());
		if (SelectedNode->Type == EScenarioHierarchyNodeType::Zone)
		{
			SetActivePlacementZone(Cast<AZone>(SelectedActor));
		}
		else if (SelectedNode->Type == EScenarioHierarchyNodeType::Area)
		{
			SetActivePlacementArea(Cast<AArea>(SelectedActor));
		}
		else
		{
			SetActivePlacementSquad(nullptr);
			SetActivePlacementZone(nullptr);
			SetActivePlacementArea(nullptr);
		}

		if (GEditor && IsValid(SelectedActor))
		{
			GEditor->SelectNone(false, true);
			GEditor->SelectActor(SelectedActor, true, true, true);
		}
	}
	else
	{
		SetActivePlacementSquad(nullptr);
		SetActivePlacementZone(nullptr);
		SetActivePlacementArea(nullptr);
	}

	RefreshPropertiesPanel();
}

/**
 * @brief Builds the static first-pass hierarchy.
 */
void SScenarioHierarchyView::RebuildHierarchy()
{
	RootNodes.Reset();
	SelectedNode.Reset();

	FScenarioHierarchyNodePtr ScenarioNode = MakeShared<FScenarioHierarchyNode>();
	ScenarioNode->Label = LOCTEXT("ScenarioNode", "Scenario");
	ScenarioNode->Type = EScenarioHierarchyNodeType::Scenario;
	ScenarioNode->Object = ActiveScenario.Get();

	FScenarioHierarchyNodePtr AINode = MakeShared<FScenarioHierarchyNode>();
	AINode->Label = LOCTEXT("AINode", "AI");
	AINode->Type = EScenarioHierarchyNodeType::Folder;

	{
		FScenarioHierarchyNodePtr FolderNode = MakeShared<FScenarioHierarchyNode>();
		FolderNode->Label = LOCTEXT("SquadGroupsNode", "Squad Groups");
		FolderNode->Type = EScenarioHierarchyNodeType::Folder;

		if (ActiveScenario.IsValid())
		{
			for (ASquadGroup* SquadGroup : ActiveScenario->SquadGroups)
			{
				if (!IsValid(SquadGroup))
				{
					continue;
				}

				FScenarioHierarchyNodePtr SquadGroupNode = MakeShared<FScenarioHierarchyNode>();
				SquadGroupNode->Label = FText::FromString(SquadGroup->GetActorLabel());
				SquadGroupNode->Type = EScenarioHierarchyNodeType::SquadGroup;
				SquadGroupNode->Object = SquadGroup;
				FolderNode->Children.Add(SquadGroupNode);
			}
		}

		AINode->Children.Add(FolderNode);
	}

	{
		FScenarioHierarchyNodePtr FolderNode = MakeShared<FScenarioHierarchyNode>();
		FolderNode->Label = LOCTEXT("SquadsNode", "Squads");
		FolderNode->Type = EScenarioHierarchyNodeType::SquadsFolder;

		if (ActiveScenario.IsValid())
		{
			for (ASquad* Squad : ActiveScenario->Squads)
			{
				if (!IsValid(Squad))
				{
					continue;
				}

				FScenarioHierarchyNodePtr SquadNode = MakeShared<FScenarioHierarchyNode>();
				SquadNode->Label = FText::FromString(Squad->GetActorLabel());
				SquadNode->Type = EScenarioHierarchyNodeType::Squad;
				SquadNode->Object = Squad;
				SquadNode->Squad = Squad;

				for (AAgent* Agent : Squad->Agents)
				{
					if (!IsValid(Agent))
					{
						continue;
					}

					FScenarioHierarchyNodePtr AgentNode = MakeShared<FScenarioHierarchyNode>();
					AgentNode->Label = FText::FromString(Agent->GetActorLabel());
					AgentNode->Type = EScenarioHierarchyNodeType::Agent;
					AgentNode->Object = Agent;
					AgentNode->Agent = Agent;
					AgentNode->ParentSquad = Squad;
					SquadNode->Children.Add(AgentNode);
				}

				FolderNode->Children.Add(SquadNode);
			}
		}

		AINode->Children.Add(FolderNode);
	}

	{
		FScenarioHierarchyNodePtr FolderNode = MakeShared<FScenarioHierarchyNode>();
		FolderNode->Label = LOCTEXT("ZonesNode", "Zones");
		FolderNode->Type = EScenarioHierarchyNodeType::ZonesFolder;

		if (ActiveScenario.IsValid())
		{
			for (AZone* Zone : ActiveScenario->Zones)
			{
				if (!IsValid(Zone))
				{
					continue;
				}

				FScenarioHierarchyNodePtr ZoneNode = MakeShared<FScenarioHierarchyNode>();
				ZoneNode->Label = FText::FromString(Zone->GetActorLabel());
				ZoneNode->Type = EScenarioHierarchyNodeType::Zone;
				ZoneNode->Object = Zone;

				for (AArea* Area : Zone->Areas)
				{
					if (!IsValid(Area))
					{
						continue;
					}

					FScenarioHierarchyNodePtr AreaNode = MakeShared<FScenarioHierarchyNode>();
					AreaNode->Label = FText::FromString(Area->GetActorLabel());
					AreaNode->Type = EScenarioHierarchyNodeType::Area;
					AreaNode->Object = Area;
					AreaNode->ParentZone = Zone;

					for (AFiringPosition* FiringPosition : Area->FiringPositions)
					{
						if (!IsValid(FiringPosition))
						{
							continue;
						}

						FScenarioHierarchyNodePtr FiringPositionNode = MakeShared<FScenarioHierarchyNode>();
						FiringPositionNode->Label = FText::FromString(FiringPosition->GetActorLabel());
						FiringPositionNode->Type = EScenarioHierarchyNodeType::FiringPosition;
						FiringPositionNode->Object = FiringPosition;
						FiringPositionNode->ParentArea = Area;
						AreaNode->Children.Add(FiringPositionNode);
					}

					ZoneNode->Children.Add(AreaNode);
				}

				FolderNode->Children.Add(ZoneNode);
			}
		}

		AINode->Children.Add(FolderNode);
	}

	{
		FScenarioHierarchyNodePtr FolderNode = MakeShared<FScenarioHierarchyNode>();
		FolderNode->Label = LOCTEXT("ObjectivesNode", "Objectives");
		FolderNode->Type = EScenarioHierarchyNodeType::Folder;

		if (ActiveScenario.IsValid())
		{
			for (AObjective* Objective : ActiveScenario->Objectives)
			{
				if (!IsValid(Objective))
				{
					continue;
				}

				FScenarioHierarchyNodePtr ObjectiveNode = MakeShared<FScenarioHierarchyNode>();
				ObjectiveNode->Label = FText::FromString(Objective->GetActorLabel());
				ObjectiveNode->Type = EScenarioHierarchyNodeType::Objective;
				ObjectiveNode->Object = Objective;
				FolderNode->Children.Add(ObjectiveNode);
			}
		}

		AINode->Children.Add(FolderNode);
	}

	ScenarioNode->Children.Add(AINode);
	RootNodes.Add(ScenarioNode);
	ApplyDebugVisualVisibility();
}

/**
 * @brief Refreshes the tree widget after hierarchy data changes.
 */
void SScenarioHierarchyView::RefreshHierarchyTree()
{
	if (!HierarchyTree.IsValid())
	{
		return;
	}

	HierarchyTree->RequestTreeRefresh();

	TArray<FScenarioHierarchyNodePtr> NodesToExpand = RootNodes;
	while (NodesToExpand.Num() > 0)
	{
		const FScenarioHierarchyNodePtr Node = NodesToExpand.Pop(EAllowShrinking::No);
		if (!Node.IsValid())
		{
			continue;
		}

		HierarchyTree->SetItemExpansion(Node, true);
		NodesToExpand.Append(Node->Children);
	}
}

/**
 * @brief Generates one hierarchy row.
 *
 * @param Item Hierarchy item to display.
 * @param OwnerTable Table view that owns the row.
 * @return Generated row widget.
 */
TSharedRef<ITableRow> SScenarioHierarchyView::GenerateHierarchyRow(FScenarioHierarchyNodePtr Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	FName IconName = TEXT("ContentBrowser.AssetTreeFolderOpen");
	if (Item.IsValid() && Item->Type == EScenarioHierarchyNodeType::Squad)
	{
		IconName = TEXT("Icons.GroupActors");
	}
	else if (Item.IsValid() && Item->Type == EScenarioHierarchyNodeType::Agent)
	{
		IconName = TEXT("ClassIcon.Character");
	}
	else if (Item.IsValid() && Item->Type == EScenarioHierarchyNodeType::Objective)
	{
		IconName = TEXT("Icons.Bullseye");
	}
	else if (Item.IsValid() && Item->Type == EScenarioHierarchyNodeType::Area)
	{
		IconName = TEXT("ClassIcon.Cube");
	}
	else if (Item.IsValid() && Item->Type == EScenarioHierarchyNodeType::FiringPosition)
	{
		IconName = TEXT("Icons.Pinned");
	}

	return SNew(STableRow<FScenarioHierarchyNodePtr>, OwnerTable)
		.Padding(FMargin(2.0f, 1.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(IconName))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Item.IsValid() ? Item->Label : FText::GetEmpty())
			]
		];
}

/**
 * @brief Gets child nodes for a hierarchy item.
 *
 * @param Item Parent item.
 * @param OutChildren Children displayed under the parent.
 */
void SScenarioHierarchyView::GetHierarchyChildren(FScenarioHierarchyNodePtr Item, TArray<FScenarioHierarchyNodePtr>& OutChildren) const
{
	if (Item.IsValid())
	{
		OutChildren.Append(Item->Children);
	}
}

#undef LOCTEXT_NAMESPACE
