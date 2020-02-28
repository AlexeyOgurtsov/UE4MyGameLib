#pragma once

#include "Movement/Spline/SplineMovementTypes.h"
#include "SplineMovementComponentImpl.generated.h"

class UMovementComponent;
class USplineComponent;


enum class ESplineMovementSimulationResetFlags : uint8
{
	/** None*/
	None = 0
	
	/** Keeping location in world space when resetting*/
	, KeepWorldSpaceLocation = 1 << 0

	/** Keeping rotation in world space when resetting*/
	, KeepWorldSpaceRotation = 1 << 1

	/** Keeping velocity in world space when resetting*/
	, KeepWorldSpaceVelocity = 1 << 2
};
ENUM_CLASS_FLAGS(ESplineMovementSimulationResetFlags);

USTRUCT()
struct FSplineMovementTrackState
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.0F;

	UPROPERTY()
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
	UPROPERTY()
	FVector MoveSpaceVelocity = FVector::ZeroVector;

	UPROPERTY()
	FSplineMovementTrackState Tracking;
};

USTRUCT()
struct FSplineMovementMoveSpace
{
	GENERATED_BODY()

	/**
	* Transform from space, in which the movement is calculated now to the world space
	*/
	UPROPERTY()
	FTransform Transform;

	/**
	* Translation of the movement space from the last detached state to target
	*/
	UPROPERTY()
	FVector MoveSpaceDetachedToTargetTranslation = FVector::ZeroVector;

	/**
	* Velocity along the blend direction in the world space
	*/
	UPROPERTY()
	FVector BlendVelocity = FVector::ZeroVector;

	FSplineMovementMoveSpace() 
	:	Transform ( ENoInit::NoInit )
	{
	}

};

USTRUCT()
struct FSplineMovementAttachmentState
{
	GENERATED_BODY()

	UPROPERTY()
	ESplineMovementAttachState State = ESplineMovementAttachState::Detached;

	UPROPERTY()
	float AttachingTime = 0.0F;

	/**
	* Transform in world space in the last detached state.
	*/
	UPROPERTY()
	FTransform MoveSpaceToWorld_BeforeAttaching;

	void SetAttached();
	void SetAttaching(const FTransform& InMoveSpaceToWorldBeforeAttaching);
	void SetDetached();
};

/**
* Implementation of spline movement common both for Actor and Pawn spline movement components.
*
* Movement is simulated in the move space.
* Move space is dependent on the state:
* In attached state - the move space is moving along the spline;
* In detached state (aka free move state) the move space is always linked with the updated component's local space.
* In attaching state - we're blending the move space between the last move space in the detached state
* and the given location on the spline;
*/
UCLASS()
class USplineMovementComponentImpl : public UObject
{
	GENERATED_BODY()

public:
	USplineMovementComponentImpl();

	static USplineMovementComponentImpl* CreateSplineMovementComponentImpl(FName InObjectName, UMovementComponent* InMovementComponent, FSplineMovementConfig* pInConfig, FSplineMovementDelegates* pInDelegates);

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

	// ~Spline Begin
	/** GetSpline*/
	USplineComponent* GetSplineComponent() const { return SplineComponent; }

	ESplineMovementAttachState GetAttachState() const { return AttachState.State; }

	/**
	* Free movement is movement state when we are detached from the spline.
	*/
	bool IsFreeMovement() const;

	/**
	* Returns whether we're in the attached-to-spline state now.
	* Attached-to-spline is the state when we're moving in the space of the spline.
	* @warn: When we're attaching, it's NOT the attached state yet!
	* @warn: In the attached state the spline component is always valid NON-null pointer.
	*/
	bool IsMovementAttachedToSpline() const;

	/**
	* Returns whether we're in the attaching-to-spline state now.
	* @warn In the attaching state the spline component is always valid NON-null pointer.
	*/
	bool IsAttachingMovementToSplineNow() const;
	bool IsMovementAttachedOrAttachingToSpline() const;

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

	/**
	* Sets the velocity in the space we currently moving in.
	*
	* @param bTrackingAccountedInVelocity If true, then tracking is already accounted in the new velocity, and will be removed.
	*/
	void SetVelocityInMoveSpace(const FVector& InVelocity, bool bTrackingAccountedInVelocity = false);

	/**
	* Sets the velocity in the world space.
	*
	* @param bTrackingAccountedInVelocity If true, then tracking is already accounted in the new velocity, and will be removed.
	*/
	void SetVelocityInWorldSpace(const FVector& InVelocity, bool bTrackingAccountedInVelocity = false);

	/** Current tracking speed */
	float GetTrackingSpeed() const;

	/** Updates tracking speed, but NOT instantenously*/
	void SetTrackingSpeed(float InTargetSpeed);

	/** Target tracking speed */
	float GetTargetTrackingSpeed() const;

	/**
	* Transform from the move space to the world space.
	* Must be called only when the move space can be calculated in the current state (we have spline or updated component)!
	* @warning: may fail (with error message) if unable to calculate move space in the current state!
	* @warning: Slow: recalculates move space each time!
	*/
	const FTransform& GetMoveSpaceToWorld() const;

	/**
	* Adds pending input vector in the move space.
	* Does nothing when the current move space cannot be calculated now.
	*
	* @param  InInputVector  Input vector in move space.
	* @returns   The given input vector transformed to the world space.
	*/
	FVector AddMoveSpaceMovementInput(const FVector& InInputVector);

	/**
	* Returns current pending input vector in the move space.
	* @warn  This vector is NOT normalized!
	*/
	const FVector& GetMoveSpacePendingInputVector() const;

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
	
	// ~Implementation-only movement component functions Begin
	// To be used from the UMovementComponent-derived implementation class, but NOT to be exposed for public use!
	/**
	* Must be called from the Component's BeginPlay()
	*
	* @warn   For implementation only (should not be exposed as a part of the movement component's interface)!
	*/
	void MyBeginPlay();

	/**
	* To be called when the PostInitProperties of the owning MovementComponent is called.
	*
	* @warn   For implementation only (should not be exposed as a part of the movement component's interface)!
	*/
	void MovementComponentPostInitProperties();

	/**
	* Typically to be called right before the Super call.
	* Always must be called before the MoveTick.
	* Returns true: if tick should be skipped, false, if should be done.
	*
	* @warn   For implementation only (should not be exposed as a part of the movement component's interface)!
	*/
	bool TickBeforeSuper_ReturnShouldSkipUpdate(float DeltaTime);

	/**
	* Performs all actions that are to be performed at the end of the tick.
	* Zeroes pending input vector.
	*
	* @warn   For implementation only (should not be exposed as a part of the movement component's interface)!
	*/
	void FinalizeTick();

	/**
	* Calculates all physics parameters.
	* And moves the updated component.
	*
	* @warn   For implementation only (should not be exposed as a part of the movement component's interface)!
	*/
	void MoveTick(float DeltaTime);

	/**
	* Set input vector at this time.
	* Typically called from TickComponent function.
	* Does nothing when the current move space cannot be calculated now.
	*
	* @param InInputVector Input vector in world space
	* 	Magnitue is always clamped to range 0.0F...1.0F
	* 	for UPawnMovementComponent: GetPendingInputVector() is typically passed.
	*
	* @warn   For implementation only (should not be exposed as a part of the movement component's interface)!
	* @warn  Prefer SetMoveSpacePendingInputVector when movement in the move space is needed.
	* @see  SetMoveSpacePendingInputVector  
	*/
	void SetPendingInputVector(const FVector& InInputVector);
	
	/**
	* Sets only the world-space input vector without setting the move-space.
	* @warn   For implementation only (should not be exposed as a part of the movement component's interface)!
	*/
	void SetOnlyWorldSpacePendingInputVector(const FVector& InInputVector);
	// ~Implementation-only movement component functions End

	// ~UMovementComponent Begin 
	/**
	* To be called from the OnTeleported of the Movement Component.
	*/
	void OnComponentTeleported();

	/**
	* To be called from the StopMovementImmediately of the Movement Component.
	*/
	void StopMovementImmediately();

	/**
	* To be called from the GetGravityZ() of the movement component.
	* @returns  The real value of the gravity.
	*/
	float GetComponentGravityZ(float InGravityZ) const;

	/**
	* To be called from the GetMaxSpeed() of the movement component.
	*
	* @returns
	* The approximate maximum magnitude of the movement component's velocity vector under normal conditions.
	* Tracking speed is accounted.
	* Attaching blending speed is accounted.
	*
	* @warn   Changes when target tracking speed or state change.
	*/
	float GetMaxSpeed() const;
	// ~UMovementComponent End
	
	// ~UObject Begin
	virtual UWorld* GetWorld() const override;
	// ~UObject End
	
private:
	// ~Spline attachment Begin
	void ReLinkToSpline();

	UPROPERTY()
	TWeakObjectPtr<AActor> SplineProvider = nullptr;

	UPROPERTY()
	USplineComponent* SplineComponent = nullptr;

	bool GotoState_Attaching();
	bool GotoState_Attached(bool bInSignalBeforeEvent = true);
	bool GotoState_Detached();

	UPROPERTY()
	FSplineMovementAttachmentState AttachState;

	// ~Spline attachnment End
	FTransform GetSplineToWorldAt(float InLocationAlongSpline) const;
	void FixLocationAlongSpline();
	void FixLocalToMoveSpace();
	void ResetToInitialTransformAndLocation();

	FVector UpdateMoveSpaceVelocity_AndReturnMoveDelta(float DeltaTime, const FVector& InAcceleration);
	void RecalculateTrackingSpeed(float DeltaTime);

	FTransform GetMoveSpaceToWorld_ForFreeMovement(const USceneComponent* InUpdatedComponent) const;

	bool CanCalculateMoveSpace(bool bInMoveOnOrToSpline) const;
	void RecalculateMoveSpace() const;
	void RecalculateMoveSpace(bool bInMoveOnOrToSpline, bool bInBlendToSpline) const;
	bool ShouldMoveFromOrToSpline() const;
	bool ShouldBlendToSplineWhenMoving() const;
	
	/**
	* Movement space
	* Always calculated on-demand (when some function needs the move space it calls the move space recalculation function by itself)
	*/
	UPROPERTY()
	mutable FSplineMovementMoveSpace MoveSpace;

	void ResetSplineMoveSpaceAndParamsFromWorldSpace(const USceneComponent* InUpdatedComponent, ESplineMovementSimulationResetFlags InFlags);
	void FixLocationAlongSplineFromWorldSpace(const USceneComponent* InUpdatedComponent);
	void FixRotationFromWorldSpace(const USceneComponent* InUpdatedComponent);
	void FixLocationFromWorldSpace(const USceneComponent* InUpdatedComponent);

	UPROPERTY()
	FTransform LocalToMoveSpace;

	UPROPERTY()
	float LocationAlongSpline = 0.0F;

	void SetOnlyMoveSpaceVelocity_InWorldSpace(const FVector& InVelocity, bool bTrackingAccountedInVelocity = true);
	void SetOnlyMoveSpaceVelocity(const FVector& InVelocity, bool bTrackingAccountedInVelocity = true);

	UPROPERTY()
	FSplineMovementPhysState Phys;

	FVector CalculateMoveSpaceAcceleration(const FVector& InInputVector, const FVector& InOldVelocity);

	UPROPERTY()
	FVector InputVector = FVector::ZeroVector;

	UPROPERTY()
	FVector MoveSpaceInputVector = FVector::ZeroVector;

	// ~Environment Begin
	/**
	* Current pending input vector from the MovementComponent
	*/
	/** MovementComponent*/
	UPROPERTY()
	UMovementComponent* MovementComponent = nullptr;

	/** Config*/
	FSplineMovementConfig* pConfig = nullptr;
	FSplineMovementDelegates* pDelegates = nullptr;
	// ~Environment End
	
};
