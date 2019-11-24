#pragma once

#include "SplineMovementConfig.h"
#include "SplineMovementTypes.generated.h"

/**
* Publicly available types for all types of the Spline Movement components.
*/

class USplineMovementComponentImpl;

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

// ~Events Begin
/**
* @param 1: Attachment state right before
* Called right before the state is changed and phys variables (transform, velocities, tracking speed) are updated.
*/
DECLARE_EVENT_OneParam(USplineMovementComponentImpl, FBeforeMovementAttachedToSplineEvent, ESplineMovementAttachState);

/**
* @param 1: Attachment state right before
* Called right before the state is changed and phys variables (transform, velocities, tracking speed) are updated.
* @warn Not called, when attaching is immediate (@see the corresponding event for the attached state instead!)
*/
DECLARE_EVENT_OneParam(USplineMovementComponentImpl, FBeforeMovementBeginAttachingToSplineEvent, ESplineMovementAttachState);

/**
* @param 1: Attachment state right before
* Called right before the state is changed and phys variables (transform, velocities, tracking speed) are updated.
*/
DECLARE_EVENT_OneParam(USplineMovementComponentImpl, FBeforeMovementDetachedFromSplineEvent, ESplineMovementAttachState);
DECLARE_EVENT(USplineMovementComponentImpl, FMovementAttachedToSplineEvent);

/**
* @warn Not called, when attaching is immediate (@see the corresponding event for the attached state instead!)
*/
DECLARE_EVENT(USplineMovementComponentImpl, FMovementBeginAttachingToSplineEvent);
DECLARE_EVENT(USplineMovementComponentImpl, FMovementDetachedFromSplineEvent);
// ~Events End
