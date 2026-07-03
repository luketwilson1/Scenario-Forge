// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ScenarioEdMode.h
 * @brief Declares the Scenario editor mode.
 */

#pragma once

#include "EdMode.h"

/**
 * @brief Editor mode for placing and editing Scenario-authored objects in the viewport.
 */
class FScenarioEdMode : public FEdMode
{
public:
	/** Scenario editor mode identifier. */
	static const FEditorModeID EM_Scenario;

	/** Handles viewport key input for scenario placement. */
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;

	/** Scenario placement uses raw click input rather than transform widgets. */
	virtual bool UsesTransformWidget() const override;

	/** Scenario mode can coexist only with default-compatible editor state. */
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override;

private:
	/** Places an agent at the viewport cursor and assigns it to the selected squad. */
	bool PlaceAgentAtCursor(FEditorViewportClient* ViewportClient);

	/** Places an area volume at the viewport cursor and assigns it to the selected zone. */
	bool PlaceAreaVolumeAtCursor(FEditorViewportClient* ViewportClient);

	/** Places a firing position plane inside the selected area. */
	bool PlaceFiringPositionAtCursor(FEditorViewportClient* ViewportClient);
};
