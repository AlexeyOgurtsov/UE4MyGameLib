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

// ~Delegates Begin
/**
* Called right before the state is changed and phys variables (transform, velocities, tracking speed) are updated.
*
* @param InStateBefore  Attachment state right before
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBeforeMovementAttachedToSplineSignature, ESplineMovementAttachState, InStateBefore);

/**
* Called right before the state is changed and phys variables (transform, velocities, tracking speed) are updated.
* @warn Not called, when attaching is immediate (@see the corresponding event for the attached state instead!)
*
* @param InStateBefore  Attachment state right before
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBeforeMovementBeginAttachingToSplineSignature, ESplineMovementAttachState, InStateBefore);

/**
* Called right before the state is changed and phys variables (transform, velocities, tracking speed) are updated.
*
* @param InStateBefore  Attachment state right before
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBeforeMovementDetachedFromSplineSignature, ESplineMovementAttachState, InStateBefore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMovementAttachedToSplineSignature);

/**
* @warn Not called, when attaching is immediate (@see the corresponding event for the attached state instead!)
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMovementBeginAttachingToSplineSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMovementDetachedFromSplineSignature);


USTRUCT(BlueprintType, Category=SplineMovement)
struct FSplineMovementDelegates
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintAssignable, Category=Delegates)
	FBeforeMovementAttachedToSplineSignature BeforeMovementAttachedToSpline;

	UPROPERTY(BlueprintAssignable, Category=Delegates)
	FBeforeMovementBeginAttachingToSplineSignature BeforeMovementBeginAttachingToSpline;

	UPROPERTY(BlueprintAssignable, Category=Delegates)
	FBeforeMovementDetachedFromSplineSignature BeforeMovementDetachedFromSpline;

	UPROPERTY(BlueprintAssignable, Category=Delegates)
	FMovementAttachedToSplineSignature OnMovementAttachedToSpline;

	UPROPERTY(BlueprintAssignable, Category=Delegates)
	FMovementBeginAttachingToSplineSignature OnMovementBeginAttachingToSpline;

	UPROPERTY(BlueprintAssignable, Category=Delegates)
	FMovementDetachedFromSplineSignature OnMovementDetachedFromSpline;
};

// ~Delegates End
