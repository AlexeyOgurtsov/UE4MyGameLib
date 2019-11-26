#pragma once

#include "UObject/WeakObjectPtrTemplates.h"
#include "SplineMovementConfig.generated.h"

/** ESplineMovementAttachTransformMode*/
UENUM(BlueprintType, Category=SplineMovement)
enum class ESplineMovementAttachTransformMode : uint8
{
	/**
	* Calculate the new spline transform from the current world transform.
	* But use rotation component of the spline.
	*/
	KeepWorldLocationOnly             = 0           UMETA(DisplayName="Keep World Location Only"),

	/**
	* Calculate the new spline transform from the current world transform.
	*/
	KeepWorld                         = 1           UMETA(DisplayName="Keep World"),

	/**
	* Move the updated component to transform according to the current Spline Transform.
	*/
	KeepSplineTransform               = 2           UMETA(DisplayName="Keep spline transform")
};

/**
* Rules of how to attach and detach to/from spline.
*/
USTRUCT(BlueprintType, Category=SplineMovement)
struct FSplineMovementAttachRules
{
	GENERATED_BODY()

	/**
	* If true, then we should attach to spline automatically on BeginPlay.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoAttach = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESplineMovementAttachTransformMode AttachTransformMode = ESplineMovementAttachTransformMode::KeepSplineTransform;

	/**
	* How fast should we blend between old and new transforms when attaching.
	* If ZERO (exacyly ZERO !) then no blend at all.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin = 0.0F, ClampMax=1000.0F))
	float AttachBlendTime = 0.0F;

	/**
	* Should we sweep when attaching.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAttachSweep = false;

	/**
	* Are we allowed to control while attaching.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowAttachingControl = true;

	/**
	* Should we automatically detach from the spline when we received a blocking hit
	* in the attached state.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDetachOnBlockingHit = false;

	/**
	* Initial transform of the local space to the move space when attached the first time.
	* @warn: X is the initial location along the spline.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform InitialLocalToMoveSpace = FTransform::Identity;
};


USTRUCT(BlueprintType, Category=SplineMovement)
struct FSplineMovementPhysTrackingConfig
{
	GENERATED_BODY()

	/** 
	 * Initial speed of moving forward along the track 
	 * If negative, then moving backward.
	 *
	 * Acceleration does not influence on this part of the movement speed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=-10000.0F, ClampMax=10000.0F))
	float InitialSpeed = 0.0F;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=0.0F, ClampMax=5000.0F))
	float Acceleration = 10.0F;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=0.0F, ClampMax=5000.0F))
	float Deceleration = 30.0F;
};


/**
* Config, that is common both for actor and pawn spline movement components.
* Physics state.
*/
USTRUCT(BlueprintType, Category=SplineMovement)
struct FSplineMovementPhysConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSplineMovementPhysTrackingConfig Tracking;

	/** Forward acceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=0.0F, ClampMax=5000.0F))
	float ForwardAcceleration = 10.0F;

	/** Forward deceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=0.0F, ClampMax=5000.0F))
	float ForwardDeceleration = 20.0F;

	/** Strafe acceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=0.0F, ClampMax=5000.0F))
	float StrafeAcceleration = 10.0F;

	/** Strafe deceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=0.0F, ClampMax=5000.0F))
	float StrafeDeceleration = 20.0F;

	/** Lift acceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=0.0F, ClampMax=5000.0F))
	float LiftAcceleration = 40.0F;

	/** Strafe deceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin=0.0F, ClampMax=5000.0F))
	float LiftDeceleration = 70.0F;
};

/**
* Config, that is common both for actor and pawn spline movement components.
*/
USTRUCT(BlueprintType, Category=SplineMovement)
struct FSplineMovementConfig
{
	GENERATED_BODY()

	/** Provider to the spline component*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> SplineProvider = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSplineMovementPhysConfig Phys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSplineMovementAttachRules AttachRules;
};
