#pragma once

#include "Movement/Spline/SplineMovementTypes.h"
#include "SplineMovementComponentImpl.generated.h"

class UMovementComponent;
class USplineComponent;

USTRUCT()
struct FSplineMovementTrackState
{
	GENERATED_BODY()

	float Speed = 0.0F;
	float TargetSpeed = 0.0F;
};

USTRUCT()
struct FSplineMovementPhysState
{
	GENERATED_BODY()

	/** 
	* In Attached State: Velocity vector in the space of the spline.
	* In Attaching State: Not Used
	* In Detached State: Velocity vector in local coordinate system of the Updated Component.
	*
	* @warn: Does NOT account the tracking-along-spline component!
	*/
	FVector MoveSpaceVelocity = FVector::ZeroVector;

	FSplineMovementTrackState Tracking;
};

USTRUCT()
struct FSplineMovementAttachmentState
{
	GENERATED_BODY()

	ESplineMovementAttachState State = ESplineMovementAttachState::Detached;
	float AttachingTime = 0.0F;

	/**
	* Transform in world space in the last detached state.
	*/
	FTransform MoveSpaceToWorld_BeforeAttaching;

	void SetAttached();
	void SetAttaching(const FTransform& InMoveSpaceToWorldBeforeAttaching);
	void SetDetached();
};

/**
* Implementation of spline movement common both for Actor and Pawn spline movement components.
*/
UCLASS()
class USplineMovementComponentImpl : public UObject
{
	GENERATED_BODY()

public:
	USplineMovementComponentImpl();

	static USplineMovementComponentImpl* CreateSplineMovementComponentImpl(FName InObjectName, UMovementComponent* InMovementComponent, FSplineMovementConfig* pInConfig);

	// ~Config Begin
	/** GetConfig*/
	const FSplineMovementConfig& GetConfig() const { return *pConfig; }

	/**
	* Updates the entire state, when the config is changed.
	* To be called explicitly when some part of the config is updated after the PostInitProperties of the Movement Component.
	*/
	void UpdateFromConfig();

	/**
	* To be called each time the spline provider config variable is changed or 
	* when the new spline component is to be used in the spline provider.
	*
	* @warn: Automatically detaches from the spline and does NOT reattach to it again!
	*/
	void UpdateSplineProvider();
	// ~Config End
	
	// ~Environment Begin
	/** GetMovementComponent*/
	UMovementComponent* GetMovementComponent() const { return MovementComponent; }
	AActor* GetOwner() const;
	USceneComponent* GetUpdatedComponent() const;
	UPrimitiveComponent* GetUpdatedPrimitive() const;
	bool IsActive() const;
	// ~Environment End

	/** GetMovementComponent*/
	// ~Spline Begin
	/** GetSpline*/
	USplineComponent* GetSplineComponent() const { return SplineComponent; }

	ESplineMovementAttachState GetAttachState() const { return AttachState.State; }

	FBeforeMovementAttachedToSplineEvent& OnBeforeMovementAttachedToSpline() { return BeforeMovementAttachedToSpline; }
	FBeforeMovementBeginAttachingToSplineEvent& OnBeforeMovementBeginAttachingToSpline() { return BeforeMovementBeginAttachingToSpline; }
	FBeforeMovementDetachedFromSplineEvent& OnBeforeMovementDetachedFromSpline() { return BeforeMovementDetachedFromSpline; }
	FMovementAttachedToSplineEvent& OnMovementAttachedToSpline() { return MovementAttachedToSpline; }
	FMovementBeginAttachingToSplineEvent& OnMovementBeginAttachingToSpline() { return MovementBeginAttachingToSpline; }
	FMovementDetachedFromSplineEvent& OnMovementDetachedFromSpline() { return MovementDetachedFromSpline; }

	/**
	* Free movement is movement when we are detached from the spline.
	*/
	bool IsFreeMovement() const { return GetAttachState() == ESplineMovementAttachState::Detached; }
	bool IsMovementAttachedToSpline() const { return GetAttachState() == ESplineMovementAttachState::Attached; }
	bool IsAttachingMovementToSplineNow() const { return GetAttachState() == ESplineMovementAttachState::Attaching; }
	bool IsMovementAttachedOrAttachingToSpline() const { return IsMovementAttachedToSpline() || IsAttachingMovementToSplineNow(); }

	/**
	* Time of attaching: always zero in detached or attached states.
	*/
	float GetAttachingTime() const { return AttachState.AttachingTime; }

	/**
	* In attached state: Transform of the local coordinate system in spline space
	* In attaching state: The goal transform of the local coordinate system we are attaching to in spline space
	* In detached state: Only rotation is used
	*
	* Along-spline offset (X) is always ZERO.
	* Scale is NEVER accounted.
	*/
	const FTransform& GetLocalToMoveSpace() const { return LocalToMoveSpace; }

	/**
	* In attached state: Location along the spline.
	* In attaching state: The goal location along the spline.
	* In detached state: not used.
	*/
	float GetLocationAlongSpline() const { return LocationAlongSpline; }

	/**
	* Returns the velocity in the space simulation is calculated now.
	* @param: bAddTrackSpeed if true, we should consider the tracking speed (if it's enabled at all).
	*/
	FVector GetMoveSpaceVelocity(bool bInAddTrackSpeed = true) const;

	/** Current tracking speed */
	float GetTrackingSpeed() const;

	/** Updates tracking speed, but NOT instantenously*/
	void SetTrackingSpeed(float InTargetSpeed);

	/** Target tracking speed */
	float GetTargetTrackingSpeed() const;

	/**
	* Transform from the spline space to the world space at current location along spline.
	*/
	FTransform GetSplineToWorld() const;


	/**
	* Detaches from spline.
	*
	* @returns: true if successfuly went to attached or attaching state (irrespective of whether due to this call or already was in that state).
	*/
	bool AttachToSpline();

	/**
	* Detaches from spline.
	*
	* @returns: true if successfully detached (irrespective or whether due to this call or already was in that state)
	*/
	bool DetachFromSpline();

	/**
	* Toggles spline attachment state: attached/detached.
	*
	* @returns: true if the state is successfully changed
	*/
	bool ToggleAttachToSpline();

	/**
	* Sets new location along spline.
	* Safe to call ever if spline is not linked.
	* If the spline is linked then the new location will be automatically fixed according to the spline length.
	*/
	void SetLocationAlongSpline(float NewLocationAlongSpline);
	// ~Spline End
	
	// ~From Movement Component Begin
	// (To be delegated from the owning movement component)
	/**
	* Must be called from the Component's BeginPlay()
	*/
	void MyBeginPlay();

	/**
	* To be called when the PostInitProperties of the owning MovementComponent is called.
	*/
	void MovementComponentPostInitProperties();

	/**
	* Typically to be called right before the Super call.
	* Always must be called before the MoveTick.
	* Returns true: if tick should be skipped, false, if should be done.
	*/
	bool TickBeforeSuper_ReturnShouldSkipUpdate(float DeltaTime);

	/**
	* Performs all actions that are to be performed at the end of the tick.
	* Zeroes pending input vector.
	*/
	void FinalizeTick();

	/**
	* Calculates all physics parameters.
	* And moves the updated component.
	*/
	void MoveTick(float DeltaTime);

	/**
	* Set input vector at this time.
	* Typically called from TickComponent function.
	*
	* @param InInputVector: Input vector in world space
	* 	Magnitue is always clamped to range 0.0F...1.0F
	* 	for UPawnMovementComponent: GetPendingInputVector() is typically passed.
	*/
	void SetPendingInputVector(const FVector& InInputVector);

	/**
	* To be called from the OnTeleported of the Movement Component.
	*/
	void OnComponentTeleported();

	/**
	* To be called from the StopMovementImmediately of the Movement Component.
	*/
	void StopMovementImmediately();
	// ~From Movement Component End
	
	// ~UObject Begin
	virtual UWorld* GetWorld() const override;
	// ~UObject End
	
private:
	// ~Spline attachment Begin
	void ReLinkToSpline();

	AActor* SplineProvider = nullptr;
	USplineComponent* SplineComponent = nullptr;

	FBeforeMovementAttachedToSplineEvent BeforeMovementAttachedToSpline;
	FBeforeMovementBeginAttachingToSplineEvent BeforeMovementBeginAttachingToSpline;
	FBeforeMovementDetachedFromSplineEvent BeforeMovementDetachedFromSpline;

	FMovementAttachedToSplineEvent MovementAttachedToSpline;
	FMovementBeginAttachingToSplineEvent MovementBeginAttachingToSpline;
	FMovementDetachedFromSplineEvent MovementDetachedFromSpline;

	bool GotoState_Attaching();
	bool GotoState_Attached(bool bInSignalBeforeEvent = true);
	bool GotoState_Detached();

	FSplineMovementAttachmentState AttachState;

	// ~Spline attachnment End

	void FixLocationAlongSpline();
	void FixLocalToMoveSpace();
	void ResetToInitialTransformAndLocation();

	FVector UpdateMoveSpaceVelocity_AndReturnMoveDelta(float DeltaTime, const FVector& InAcceleration);
	void RecalculateTrackingSpeed(float DeltaTime);

	FTransform GetMoveSpaceToWorld_ForFreeMovement() const;

	FTransform LocalToMoveSpace;
	float LocationAlongSpline = 0.0F;

	FSplineMovementPhysState Phys;

	FVector CalculateAccelerationFromInputVector(const FVector& InInputVector, const FVector& InOldVelocity) const;

	FVector InputVector = FVector::ZeroVector;

	// ~Environment Begin
	/**
	* Current pending input vector from the MovementComponent
	*/
	/** MovementComponent*/
	UMovementComponent* MovementComponent = nullptr;

	/** Config*/
	FSplineMovementConfig* pConfig = nullptr;
	// ~Environment End
	
};
