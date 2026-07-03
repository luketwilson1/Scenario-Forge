// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Zone.cpp
 * @brief Implements authored tactical zones.
 */

#include "Zone.h"

/**
 * @brief Initializes this data-only tactical zone.
 */
AZone::AZone()
{
	PrimaryActorTick.bCanEverTick = false;
}
