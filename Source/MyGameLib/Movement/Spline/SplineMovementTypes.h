#pragma once

#include "SplineMovementConfig.h"
#include "SplineMovementTypes.generated.h"

/**
* Publicly available types for all types of the Spline Movement components.
*/

UENUM(BlueprintType, Category=SplineMovement)
enum class ESplineMovementAttachState : uint8
{
	/**
	* Detached from the spline.
	* The movement is NOT constrained to the spline at all.
	*/
	Detached             = 0           UMETA(DisplayName="Detached"),

	/**
	* We are attaching to the spline, with blending
	*/
	Attaching            = 1           UMETA(DisplayName = "Attaching"),
	
	/**
	* We are already attached to the spline.
	* Moving is performed with the spline constraint.
	*/
	Attached             = 2           UMETA(DisplayName = "Attached")
};
