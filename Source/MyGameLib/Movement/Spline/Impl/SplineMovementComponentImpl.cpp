#include "SplineMovementComponentImpl.h"
#include "GameUtil/Math/GameMath.h"
#include "GameUtil/Spline/MySplineUtil.h"
#include "Util/Core/LogUtilLib.h"

#include "GameFramework/MovementComponent.h"
#include "Components/SplineComponent.h"
#include "Math/UnrealMathUtility.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Engine/World.h"

/**
* @TODO Events:
* 1. Attaching/Detaching/Attached states etc.
* 1.0. Call the events (+DONE)
* 1.1. Make MovementComponent's events bindable from Blueprint
*
* @TODO Free Mode
* 1. Rotation is to be rotated by the LocalToMoveSpace's rotation relative to the movement axis (+CODED)
*
* @TODO Check world bounds
*/

USplineMovementComponentImpl::USplineMovementComponentImpl()
{
	// Now we always need tick to update the spline provider.
	MovementComponent->bAutoUpdateTickRegistration = false;

	AttachState.SetDetached();
}

USplineMovementComponentImpl* USplineMovementComponentImpl::CreateSplineMovementComponentImpl(FName const InObjectName, UMovementComponent* const InMovementComponent, FSplineMovementConfig* const pInConfig)
{
	checkf(InMovementComponent, TEXT("When calling \"%s\" the given movement component must be valid non-NULL pointer"), TEXT(__FUNCTION__));
	checkf(pInConfig, TEXT("When calling \"%s\" the given config pointer must be valid non-NULL pointer"), TEXT(__FUNCTION__));
	USplineMovementComponentImpl* const Obj = InMovementComponent->CreateDefaultSubobject<USplineMovementComponentImpl>(InObjectName);
	Obj->MovementComponent = InMovementComponent;
	Obj->pConfig = pInConfig;
	return Obj;

}

void USplineMovementComponentImpl::MovementComponentPostInitProperties()
{
	M_LOGFUNC();
	Phys.Tracking.Speed = GetConfig().Phys.Tracking.InitialSpeed;
	Phys.Tracking.TargetSpeed = Phys.Tracking.Speed;
	ResetToInitialTransformAndLocation();
	UpdateFromConfig();
}

void USplineMovementComponentImpl::MyBeginPlay()
{
	M_LOGFUNC();
	if(GetConfig().AttachRules.bAutoAttach)
	{
		bool const bAttachedToSpline = AttachToSpline();
	}
}

bool USplineMovementComponentImpl::TickBeforeSuper_ReturnShouldSkipUpdate(float const DeltaTime)
{
	bool bShouldSkip = false;

	if( ! IsActive() )
	{
		bShouldSkip = true;
	}

	// For now we simulate only in game
	if( ! GetWorld()->IsGameWorld() )
	{
		bShouldSkip = true;
	}

	if( ! GetConfig().SplineProvider.IsValid(false) )
	{
		if(SplineComponent)
		{
			M_LOG(TEXT("Spline provider is NOT valid, detaching from it automatically"));
			SplineComponent = nullptr;
		}
	}

	bool bShouldBeDetached = false;
	if(SplineComponent == nullptr)
	{
		bShouldBeDetached = true;
		bShouldSkip = true;
	}

	if(GetUpdatedComponent() == nullptr)
	{
		bShouldBeDetached = true;
		bShouldSkip = true;
	}

	if(UPrimitiveComponent* const UpdatedPrimitive = GetUpdatedPrimitive())
	{
		if(UpdatedPrimitive->IsSimulatingPhysics())
		{
			// For now, primitives that are simulating physics are instantly detached from the spline,
			// and never processed by the simulation
			bShouldBeDetached = true;
			bShouldSkip = true;
		}
	}

	if(IsMovementAttachedOrAttachingToSpline() && bShouldBeDetached)
	{
		M_LOG(TEXT("Automatic detaching from spline"));
		GotoState_Detached();
	}

	if(MovementComponent->ShouldSkipUpdate(DeltaTime))
	{
		bShouldSkip = true;
	}

	return bShouldSkip;
}


void USplineMovementComponentImpl::FinalizeTick()
{
	InputVector = FVector::ZeroVector;
	MovementComponent->UpdateComponentVelocity();
}

void USplineMovementComponentImpl::MoveTick(float const DeltaTime)
{
	if(AttachState.State == ESplineMovementAttachState::Attaching)
	{
		AttachState.AttachingTime += DeltaTime;
		if(AttachState.AttachingTime >= GetConfig().AttachRules.AttachBlendTime)
		{
			GotoState_Attached();
		}
	}

	bool const bMoveOnOrToSpline = ShouldMoveFromOrToSpline();
	// All initializer values here are to be valid for the detached state!
	// Is arbitrary movement control is allowed in this state
	bool bAllowMoveControl = true;
	bool bSweep = true;
	bool const bBlendMoveSpaceToSpline = ShouldBlendToSplineWhenMoving();
	bool bDetachOnBlockingHit = false;
	switch(AttachState.State)
	{
	case ESplineMovementAttachState::Detached:
		// Using the initialized variable values in this state
		break;

	case ESplineMovementAttachState::Attaching:
		bAllowMoveControl = GetConfig().AttachRules.bAllowAttachingControl;
		bSweep = GetConfig().AttachRules.bAttachSweep;
		bDetachOnBlockingHit = true;
		break;
	
	case ESplineMovementAttachState::Attached:
		bAllowMoveControl = true;
		bSweep = true;
		bDetachOnBlockingHit = GetConfig().AttachRules.bDetachOnBlockingHit;
		break;

	default:
		M_LOG_ERROR(TEXT("Unknown spline movement attach state"));
		checkNoEntry();
	}

	RecalculateMoveSpace(bMoveOnOrToSpline, bBlendMoveSpaceToSpline);

	FVector const MoveSpaceInputVector = bAllowMoveControl ? MoveSpace.Transform.InverseTransformVectorNoScale(InputVector) : FVector::ZeroVector;	
	FVector const MoveSpaceAcceleration = CalculateMoveSpaceAcceleration(MoveSpaceInputVector, Phys.MoveSpaceVelocity);
	RecalculateTrackingSpeed(DeltaTime);

	// MoveDelta in the Move space (@see ComputeMoveDelta help)
	FVector const MoveDelta = UpdateMoveSpaceVelocity_AndReturnMoveDelta(DeltaTime, MoveSpaceAcceleration);

	if(bMoveOnOrToSpline)
	{
		LocalToMoveSpace.AddToTranslation(MoveDelta);
		if( ! bBlendMoveSpaceToSpline )
		{
			LocationAlongSpline += LocalToMoveSpace.GetLocation().X;
			FixLocationAlongSpline();
		}
		else
		{
			checkf(bBlendMoveSpaceToSpline, TEXT("In \"%s\": We are on the branch where blending to spline is enabled"), TEXT(__FUNCTION__));
			// Blend faster when moving forward
			// @TODO
		}
		// Warning: Fixing local-to-move-space must be done after it's .X component is accounted, as it's zeroed when fixing!
		FixLocalToMoveSpace();
	}


	FVector DeltaLocation; // Delta Location In World Space
	FRotator NewRotation; // New Rotation in World Space
	{
		if(bMoveOnOrToSpline)
		{
			// New transform of the updated component in world space
			FTransform const TargetTransform = LocalToMoveSpace * MoveSpace.Transform;
			DeltaLocation = TargetTransform.GetLocation() - GetUpdatedComponent()->GetComponentLocation();
			NewRotation = TargetTransform.Rotator();
		}
		else
		{
			DeltaLocation = MoveSpace.Transform.TransformVectorNoScale(MoveDelta);
			NewRotation = MoveSpace.Transform.TransformRotation(LocalToMoveSpace.GetRotation()).Rotator();
		}
	}


	{
		MovementComponent->Velocity = MoveSpace.Transform.TransformVectorNoScale(GetMoveSpaceVelocity());
		MovementComponent->Velocity += MoveSpace.BlendVelocity * DeltaTime;
	}

	{
		FHitResult Hit(1.f);
		bool const bMoved = MovementComponent->SafeMoveUpdatedComponent(DeltaLocation, NewRotation, bSweep, Hit);
		if(Hit.IsValidBlockingHit())
		{
			M_LOG(TEXT("Blocking hit while calling SafeMoveUpdatedComponent"));

			MovementComponent->HandleImpact(Hit, DeltaTime, DeltaLocation);
			MovementComponent->SlideAlongSurface(DeltaLocation, 1.f-Hit.Time, Hit.Normal, Hit, true);

			if(bDetachOnBlockingHit)
			{
				if( ! IsFreeMovement() )
				{
					M_LOG(TEXT("Detaching because of blocking hit"));
					GotoState_Detached();
				}
			}

			if(IsMovementAttachedToSpline())
			{
				ResetSplineMoveSpaceAndParamsFromWorldSpace
				(
					GetUpdatedComponent(),
					ESplineMovementSimulationResetFlags::KeepWorldSpaceLocation | 
				       	ESplineMovementSimulationResetFlags::KeepWorldSpaceVelocity
				);
			}
		}
	}
}

float USplineMovementComponentImpl::GetComponentGravityZ(float const InGravityZ) const
{
	if(IsFreeMovement())
	{
		return InGravityZ;
	}

	return 0.0F;
}

float USplineMovementComponentImpl::GetMaxSpeed() const
{
	float const MaxSpeed = FVector 
	{
	 	GetConfig().Phys.MaxForwardSpeed + GetTargetTrackingSpeed(),
		GetConfig().Phys.MaxStrafeSpeed,
		GetConfig().Phys.MaxLiftSpeed
	}.Size();
	if(IsAttachingMovementToSplineNow())
	{
		return MaxSpeed + MoveSpace.BlendVelocity.Size();
	}
	return MaxSpeed;
}

bool USplineMovementComponentImpl::ShouldBlendToSplineWhenMoving() const
{
	switch(AttachState.State)
	{
	case ESplineMovementAttachState::Detached:
		return false;

	case ESplineMovementAttachState::Attaching:
		// WARNING! We should not check here whether we should blend based on the config,
		// because the go-to-state functions should already check it.
		return true;
	
	case ESplineMovementAttachState::Attached:
		return false;

	default:
		M_LOG_ERROR(TEXT("Unknown spline movement attach state"));
		checkNoEntry();
	}
	return false;

}

bool USplineMovementComponentImpl::ShouldMoveFromOrToSpline() const
{
	switch(AttachState.State)
	{
	case ESplineMovementAttachState::Detached:
		return false;

	case ESplineMovementAttachState::Attaching:
		return true;
	
	case ESplineMovementAttachState::Attached:
		return true;

	default:
		M_LOG_ERROR(TEXT("Unknown spline movement attach state"));
		checkNoEntry();
	}
	return false;
}

/**
* @returns true if able to calculate move space of the given type
*/
bool USplineMovementComponentImpl::CanCalculateMoveSpace(bool const bInMoveOnOrToSpline) const
{
	if(bInMoveOnOrToSpline)
	{
		return GetSplineComponent() != nullptr;
	}
	else
	{
		return GetUpdatedComponent() != nullptr;
	}
	return true;
}

void USplineMovementComponentImpl::RecalculateMoveSpace() const
{
	RecalculateMoveSpace(ShouldMoveFromOrToSpline(), ShouldBlendToSplineWhenMoving());
}

/**
* Recalculates the move space of the given type, provided by the arguments.
* @see CanCalculateMoveSpace
* @param bInMoveOnOrToSpline Must be true if we're currently moving on spline or we're blending to spline
* @param bInBlendToSpline True if the target move space is to be calculated by blending the destination on the spline and the before-attached move-space.
*/
void USplineMovementComponentImpl::RecalculateMoveSpace(bool const bInMoveOnOrToSpline, bool const bInBlendToSpline) const
{
	checkf( ! bInBlendToSpline || bInMoveOnOrToSpline, TEXT("When calling \"%s\" blending to spline must be enabled only if moving on or to spline is enabled!"), TEXT(__FUNCTION__));
	checkf( ! bInBlendToSpline || IsAttachingMovementToSplineNow(), TEXT("Calling \"%s\" with blending to spline enabled valid only in the attaching state!"), TEXT(__FUNCTION__));

	checkf( CanCalculateMoveSpace(bInMoveOnOrToSpline), TEXT("Wrong to call \"%s\" in this state - unable to calculate the move space"), TEXT(__FUNCTION__));

	MoveSpace.MoveSpaceDetachedToTargetTranslation = FVector::ZeroVector;
	MoveSpace.BlendVelocity = FVector::ZeroVector;

	if(bInMoveOnOrToSpline)
	{
		FTransform const TargetSplineToWorld = GetSplineToWorldAt(LocationAlongSpline);
		if(bInBlendToSpline)
		{
			float const AttachBlendTime = GetConfig().AttachRules.AttachBlendTime;
			FTransform const MoveSpaceToWorld_BeforeAttachingStarted = AttachState.MoveSpaceToWorld_BeforeAttaching;
			if( ! FMath::IsNearlyZero(AttachBlendTime) )
			{
				MoveSpace.Transform.Blend(MoveSpaceToWorld_BeforeAttachingStarted, TargetSplineToWorld, AttachState.AttachingTime / AttachBlendTime );
				MoveSpace.MoveSpaceDetachedToTargetTranslation = TargetSplineToWorld.GetLocation() - MoveSpaceToWorld_BeforeAttachingStarted.GetLocation();
				MoveSpace.BlendVelocity = MoveSpace.MoveSpaceDetachedToTargetTranslation / AttachBlendTime;
			}
		}
		else
		{
			MoveSpace.Transform = TargetSplineToWorld;
		}
	}
	else
	{
		MoveSpace.Transform = GetMoveSpaceToWorld_ForFreeMovement(GetUpdatedComponent());
	}
}

/**
* Recalculates parameters of the simulation from the world-space state.
* Parameters are recalculate as if were moved on the spline at the current Location-along-spline!
* Recalculates the MoveSpace by itself.
*/
void USplineMovementComponentImpl::ResetSplineMoveSpaceAndParamsFromWorldSpace(const USceneComponent* const InUpdatedComponent, ESplineMovementSimulationResetFlags const InFlags)
{
	checkf(InUpdatedComponent, TEXT("When calling \"%s\" provided Update Component argument must be valid NON-null pointer"), TEXT(__FUNCTION__));
	checkf(GetSplineComponent(), TEXT("When calling \"%s\" SplineComponent must be valid NON-null pointer"), TEXT(__FUNCTION__));

	bool const bKeepWorldLocation = (InFlags & ESplineMovementSimulationResetFlags::KeepWorldSpaceLocation) != ESplineMovementSimulationResetFlags::None;
	if(bKeepWorldLocation)
	{
		FixLocationAlongSplineFromWorldSpace(InUpdatedComponent);
	}
	// Warning! We always must recalculate the move space here,
	// ever if location along spline is not changed, because
	// we could be in the detached state right before calling this function!
	MoveSpace.Transform = GetSplineToWorldAt(LocationAlongSpline);

	if(bKeepWorldLocation)
	{
		FixLocationFromWorldSpace(InUpdatedComponent);
	}
	if((InFlags & ESplineMovementSimulationResetFlags::KeepWorldSpaceRotation) != ESplineMovementSimulationResetFlags::None)
	{
		FixRotationFromWorldSpace(InUpdatedComponent);
	}
	if((InFlags & ESplineMovementSimulationResetFlags::KeepWorldSpaceVelocity) != ESplineMovementSimulationResetFlags::None)
	{
		SetOnlyMoveSpaceVelocity_InWorldSpace(MovementComponent->Velocity);
	}

}

/**
* Recalculates the local-to-move-space transform's location from the actor's location in world space
* Assumes that the MoveSpace matrix is valid.
* Must work from any state (attaching/attached/detached).
*/
void USplineMovementComponentImpl::FixLocationFromWorldSpace(const USceneComponent* const InUpdatedComponent)
{
	checkf(InUpdatedComponent, TEXT("When calling \"%s\" the updated component must be valid NON-null pointer"), TEXT(__FUNCTION__));
	FVector const MoveToLocalDelta = FTransform::SubtractTranslations(InUpdatedComponent->GetComponentTransform(), MoveSpace.Transform);
	LocalToMoveSpace.SetLocation(MoveToLocalDelta);
	
	// Now we need to fix the the X of the transform's location.
	// It's already to be near-zero because origins of the Move and Local spaces are to have the same X.
	// (X axis is the same for both).
	FixLocalToMoveSpace();
}

/**
* Recalculates the local-to-move-space transform's rotation from the actor's rotation in world space
* Assumes that the MoveSpace matrix is valid.
* Must work from any state (attaching/attached/detached).
*/
void USplineMovementComponentImpl::FixRotationFromWorldSpace(const USceneComponent* const InUpdatedComponent)
{
	checkf(InUpdatedComponent, TEXT("When calling \"%s\" the updated component must be valid NON-null pointer"), TEXT(__FUNCTION__));
	FQuat const MoveToLocalRotation = MoveSpace.Transform.GetRotation().Inverse() * InUpdatedComponent->GetComponentQuat();
	LocalToMoveSpace.SetRotation(MoveToLocalRotation);
}

void USplineMovementComponentImpl::FixLocationAlongSplineFromWorldSpace(const USceneComponent* const InUpdatedComponent)
{
	checkf(InUpdatedComponent, TEXT("When calling \"%s\" the updated component must be valid"), TEXT(__FUNCTION__));
	// Update location along the spline
	// WARNING! Note that we're calculating APPROXIMATE distance along the spline!
	LocationAlongSpline = UMySplineUtil::GetDistanceAlongSplineClosestToPoint(GetSplineComponent(), InUpdatedComponent->GetComponentLocation());
	FixLocationAlongSpline();
}

/*
* Sets the move-space velocity from the given velocity in the world space optionally accounting tracking.
* @warn: Does NOT update the world-space Velocity.
*
* @param InVelocity: velocity in the world space.
**/
void USplineMovementComponentImpl::SetOnlyMoveSpaceVelocity_InWorldSpace(const FVector& InVelocity, bool const bTrackingAccountedInVelocity)
{
	SetOnlyMoveSpaceVelocity(MoveSpace.Transform.InverseTransformVectorNoScale(InVelocity), bTrackingAccountedInVelocity);
}

/*
* Sets the move-space velocity optionally accounting tracking.
* @warn: Does NOT clamp the move space velocity!
* @warn: Does NOT update the world-space Velocity.
*
* @param InVelocity: velocity in the move space.
**/
void USplineMovementComponentImpl::SetOnlyMoveSpaceVelocity(const FVector& InVelocity, bool const bTrackingAccountedInVelocity)
{
	Phys.MoveSpaceVelocity = InVelocity;
	if(bTrackingAccountedInVelocity)
	{
		Phys.MoveSpaceVelocity.X -= GetTrackingSpeed();
	}
}

void USplineMovementComponentImpl::SetVelocityInMoveSpace(const FVector& InVelocity, bool const bTrackingAccountedInVelocity)
{
	SetOnlyMoveSpaceVelocity(InVelocity, bTrackingAccountedInVelocity);
	bool const bOnSpline = ShouldMoveFromOrToSpline();
	bool const bWithBlend = ShouldBlendToSplineWhenMoving();
	if(CanCalculateMoveSpace(bOnSpline))
	{
		RecalculateMoveSpace(bOnSpline, bWithBlend);
		MovementComponent->Velocity = MoveSpace.Transform.TransformVectorNoScale(InVelocity);
		MovementComponent->UpdateComponentVelocity();
	}
	else
	{
		M_LOG_WARN(TEXT("Calling \"%s\" in the state where move space cannot be recalculated - world-space velocity will not be updated!"), TEXT(__FUNCTION__));
	}
}


void USplineMovementComponentImpl::SetVelocityInWorldSpace(const FVector& InVelocity, bool const bTrackingAccountedInVelocity)
{
	SetOnlyMoveSpaceVelocity_InWorldSpace(InVelocity, bTrackingAccountedInVelocity);
	MovementComponent->Velocity = InVelocity;
	MovementComponent->UpdateComponentVelocity();
}
/**
* Calculate the move space to world transform as if the current mode is a free movement mode.
*/
FTransform USplineMovementComponentImpl::GetMoveSpaceToWorld_ForFreeMovement(const USceneComponent* const InUpdatedComponent) const
{
	// Remove rotation of the local coordinate system relative to the move space
	return InUpdatedComponent->GetComponentTransform() * LocalToMoveSpace.GetRotation().Inverse();
}

const FTransform& USplineMovementComponentImpl::GetMoveSpaceToWorld() const
{
	bool const bOnSpline = ShouldMoveFromOrToSpline();
	bool const bWithBlend = ShouldBlendToSplineWhenMoving();
	bool const bCanCalculate = CanCalculateMoveSpace(bOnSpline);
	if ( ! bCanCalculate )
	{
		M_LOG_ERROR(TEXT("Unable to calculate the move space in the current state!"));
		return FTransform::Identity;
	}

	RecalculateMoveSpace(bOnSpline, bWithBlend);
	return MoveSpace.Transform;
}

void USplineMovementComponentImpl::OnComponentTeleported()
{
	M_LOGFUNC();

	if ( IsAttachingMovementToSplineNow() )
	{
		M_LOG(TEXT("Teleported - detaching from spline"));
		GotoState_Detached();
	}

	if( IsMovementAttachedToSpline() )
	{
		// NOTE: Here the spline component must be always valid, because of the IsMovementAttachedToSpline requirement!
		M_LOG(TEXT("Teleported - recalculating new spline transform"));
		ResetSplineMoveSpaceAndParamsFromWorldSpace(GetUpdatedComponent(), ESplineMovementSimulationResetFlags::KeepWorldSpaceLocation | ESplineMovementSimulationResetFlags::KeepWorldSpaceRotation);
		MovementComponent->Velocity = MoveSpace.Transform.InverseTransformVectorNoScale(GetMoveSpaceVelocity());
		MovementComponent->UpdateComponentVelocity();
	}
}

void USplineMovementComponentImpl::StopMovementImmediately()
{
	M_LOGFUNC();
	Phys.Tracking.Speed = 0.0F;
	Phys.Tracking.TargetSpeed = 0.0F;
	Phys.MoveSpaceVelocity = FVector::ZeroVector;
	if(IsAttachingMovementToSplineNow())
	{
		M_LOG(TEXT("Stopping attaching - StopMovementImmeditely requested"));
		GotoState_Detached();
	}
}

void USplineMovementComponentImpl::SetLocationAlongSpline(float const NewLocationAlongSpline)
{
	LocationAlongSpline = NewLocationAlongSpline;
	FixLocationAlongSpline();
}

/**
* Gets spline-to-world matrix at the given location along the spline.
* @warn! The distance along the spline must already be fixed! 
*/
FTransform USplineMovementComponentImpl::GetSplineToWorldAt(float const InLocationAlongSpline) const
{
	checkf(GetSplineComponent(), TEXT("When calling \"%s\" SplineComponent must be valid NON-null pointer"), TEXT(__FUNCTION__));
	return GetSplineComponent()->GetTransformAtDistanceAlongSpline(InLocationAlongSpline, ESplineCoordinateSpace::Type::World);
}

void USplineMovementComponentImpl::RecalculateTrackingSpeed(float const DeltaTime)
{
	FGameFloatUpdate const SpeedUpdate { GetConfig().Phys.Tracking.Acceleration, GetConfig().Phys.Tracking.Deceleration };
	Phys.Tracking.Speed = UGameMath::GetFloatUpdatedToTarget(DeltaTime, Phys.Tracking.Speed, GetTargetTrackingSpeed(), SpeedUpdate);
}

/**
* Simulates the new move and returns the delta of location.
* Updates the Velocity.
*
* @returns Location delta vector in move space
* @param: InAcceleration: Acceleration in the move space
* @param bAddTrackSpeed: if true, then add tracking speed component to the forward velocity part
*/
FVector USplineMovementComponentImpl::UpdateMoveSpaceVelocity_AndReturnMoveDelta(float const DeltaTime, const FVector& InAcceleration)
{
	// Verlet velocity integration
	FVector const DeltaLocation = DeltaTime * GetMoveSpaceVelocity() + 0.5F * DeltaTime * DeltaTime * InAcceleration;
	Phys.MoveSpaceVelocity += DeltaTime * InAcceleration; // @TODO: Is It correct integration at all?
	return DeltaLocation;
}

FVector USplineMovementComponentImpl::GetMoveSpaceVelocity(bool const bInAddTrackSpeed) const 
{
	if(bInAddTrackSpeed)
	{
		return Phys.MoveSpaceVelocity + FVector(Phys.Tracking.Speed, 0.0F, 0.0F);
	}
	else
	{
       		return Phys.MoveSpaceVelocity; 
	}
}

float USplineMovementComponentImpl::GetTrackingSpeed() const
{
	return Phys.Tracking.Speed;
}

void USplineMovementComponentImpl::SetTrackingSpeed(float const InTargetSpeed)
{
	Phys.Tracking.TargetSpeed = FMath::Clamp(InTargetSpeed, 0.0F, 50000.0F);
}

float USplineMovementComponentImpl::GetTargetTrackingSpeed() const
{
	return Phys.Tracking.TargetSpeed;
}

/**
* Calculates acceleration in the move space.
*
* @return Target Acceleration in the move space
* @param InInputVector Input vector in the move space 
* @param InOldVelocity Input velocity in the move space
*/
FVector USplineMovementComponentImpl::CalculateMoveSpaceAcceleration(const FVector& InInputVector, const FVector& InOldVelocity)
{
	FVector const MaxSpeedVector = GetConfig().Phys.GetMaxSpeedVector();
	FVector const AccelerationVector = GetConfig().Phys.GetAccelerationVector();
	FVector const DecelerationVector = GetConfig().Phys.GetDecelerationVector();
	
	FVector const AccelSignVector = (InOldVelocity * InInputVector).GetSignVector();

	// Acceleration component to be applied to the axis in DIRECTION of the INPUT VECTOR's component SIGN!
	auto const GetAccelerationComponent = [&, MaxSpeedVector, DecelerationVector, AccelerationVector, AccelSignVector, InInputVector, InOldVelocity](EAxis::Type InAxis)
	{
		float const Vel = InOldVelocity.GetComponentForAxis(InAxis);
		float const Speed = FMath::Abs(Vel);
		float const AccelSign = AccelSignVector.GetComponentForAxis(InAxis);

		float const DeltaToMaxSpeed = MaxSpeedVector.GetComponentForAxis(InAxis) - Speed;
		bool const bControlAccel = ( ! FMath::IsNearlyZero(AccelSign) ) && AccelSign > 0;
		bool const bMaxSpeedExceeded = DeltaToMaxSpeed < 0.0F;
		if( ! bControlAccel || bMaxSpeedExceeded)
		{
			// We should not decelerate more than to the point when maximal speed for component is gained if it's gained
			// And cannot change sign of the velocity after the deceleration, so we decelerate up to the point,
			// when the corresponding velocity component is zeroed
			float const MaxDecelAbs = bMaxSpeedExceeded ? (-DeltaToMaxSpeed) : Speed;

			return - FMath::Min(DecelerationVector.GetComponentForAxis(InAxis), MaxDecelAbs);
		}
		else
		{
			if( bControlAccel )
			{
				return FMath::Min(AccelerationVector.GetComponentForAxis(InAxis), DeltaToMaxSpeed);
			}
		}
		return 0.0F;
	};
	return AccelSignVector * FVector
	{
		GetAccelerationComponent(EAxis::Type::X),
		GetAccelerationComponent(EAxis::Type::Y),
		GetAccelerationComponent(EAxis::Type::Z)
	};

}

void USplineMovementComponentImpl::SetPendingInputVector(const FVector& InInputVector)
{
	InputVector = InInputVector.GetClampedToMaxSize(1.F);
}

void USplineMovementComponentImpl::UpdateFromConfig()
{
	M_LOGFUNC();
	
	UpdateSplineProvider();
}

void USplineMovementComponentImpl::UpdateSplineProvider()
{
	M_LOGFUNC();
	bool bShouldRelink = false;
	if(SplineProvider != GetConfig().SplineProvider)
	{
		M_LOG(TEXT("Spline provider is changed - we should relink to spline"));
		bShouldRelink = true;
	}

	if(GetSplineComponent() == nullptr)
	{
		M_LOG(TEXT("Old spline component is removed - we should relink to spline"));
		bShouldRelink = true;
	}
	
	if(bShouldRelink)
	{
		if( ! IsFreeMovement() )
		{	
			M_LOG(TEXT("Detach forcefully (as the spline provider is changed)"))
			GotoState_Detached();
		}

		ReLinkToSpline();
	}
}

bool USplineMovementComponentImpl::IsFreeMovement() const 
{
       	return GetAttachState() == ESplineMovementAttachState::Detached; 
}
bool USplineMovementComponentImpl::IsMovementAttachedToSpline() const 
{
       	bool const bAttached = (GetAttachState() == ESplineMovementAttachState::Attached); 
	// @TODO: Is it possible that we're attached to spline, but the spline is NULL (not updated)
	checkf( GetSplineComponent() || !bAttached, TEXT("In this state the spline component must always be valid NON-null pointer!"));
	return bAttached;
}
bool USplineMovementComponentImpl::IsAttachingMovementToSplineNow() const 
{
       	bool const bAttaching = (GetAttachState() == ESplineMovementAttachState::Attaching); 
	// @TODO: Is it possible that we're attaching to spline, but the spline is NULL (not updated)
	checkf( GetSplineComponent() || !bAttaching, TEXT("In this state the spline component must always be valid NON-null pointer!"));
	return bAttaching;
}
bool USplineMovementComponentImpl::IsMovementAttachedOrAttachingToSpline() const 
{
       	return IsMovementAttachedToSpline() || IsAttachingMovementToSplineNow(); 
}

bool USplineMovementComponentImpl::AttachToSpline()
{
	if(IsMovementAttachedOrAttachingToSpline())
	{
		M_LOG_WARN(TEXT("Skipping attaching to spline - already attached or attaching"));
		return true;
	}
	if(GetUpdatedComponent() == nullptr)
	{
		M_LOG_ERROR(TEXT("Skipping attaching to spline - no updated component"));
		return false;
	}
	if(GetSplineComponent() == nullptr)
	{
		M_LOG_ERROR(TEXT("Skipping attaching to spline - no spline component"));
		return false;
	}
	return GotoState_Attaching();
}

bool USplineMovementComponentImpl::DetachFromSpline()
{
	if(IsFreeMovement())
	{
		M_LOG_WARN(TEXT("Skipping detaching from spline - already detached"));
		return true;
	}
	return GotoState_Detached();
}

bool USplineMovementComponentImpl::ToggleAttachToSpline()
{
	if(IsMovementAttachedOrAttachingToSpline())
	{
		return DetachFromSpline();
	}
	else
	{
		return AttachToSpline();
	}
}

void USplineMovementComponentImpl::ReLinkToSpline()
{
	M_LOGFUNC();
	checkf(IsFreeMovement(), TEXT("\"%s\" should be called only in the detached-from-spline state"), TEXT(__FUNCTION__));
	SplineProvider = GetConfig().SplineProvider;
	if(SplineProvider.Get() == nullptr)
	{
		M_LOG(TEXT("Spline provider is NULL"))
		SplineComponent = nullptr;
	}	
	else
	{
		M_LOG(TEXT("Spline provider is NON-null pointer"))
		ULogUtilLib::LogKeyedNameClassSafeC(TEXT("Spline provider"), SplineProvider.Get());
		SplineComponent = SplineProvider->FindComponentByClass<USplineComponent>();
		ULogUtilLib::LogFloatC(TEXT("New spline component"), GetLocationAlongSpline());
	}
	if(SplineComponent != nullptr)
	{
		FixLocationAlongSpline();
		ULogUtilLib::LogFloatC(TEXT("New location along spline"), GetLocationAlongSpline());
	}
}

AActor* USplineMovementComponentImpl::GetOwner() const
{
	return MovementComponent->GetOwner();
}

USceneComponent* USplineMovementComponentImpl::GetUpdatedComponent() const
{
	return MovementComponent->UpdatedComponent;
}

UPrimitiveComponent* USplineMovementComponentImpl::GetUpdatedPrimitive() const
{
	return MovementComponent->UpdatedPrimitive;
}

bool USplineMovementComponentImpl::IsActive() const
{
	return MovementComponent->IsActive();
}

void USplineMovementComponentImpl::ResetToInitialTransformAndLocation()
{
	LocalToMoveSpace = GetConfig().AttachRules.InitialLocalToMoveSpace;
	LocationAlongSpline = GetConfig().AttachRules.InitialLocalToMoveSpace.GetLocation().X;
	FixLocationAlongSpline();
	FixLocalToMoveSpace();
}

void USplineMovementComponentImpl::FixLocalToMoveSpace()
{
	LocalToMoveSpace.SetLocation(FVector{0.0F, LocalToMoveSpace.GetLocation().Y, LocalToMoveSpace.GetLocation().Z});
}

void USplineMovementComponentImpl::FixLocationAlongSpline()
{
	if(GetSplineComponent() == nullptr)
	{
		return;
	}
	float const MaxLocation = GetSplineComponent()->GetSplineLength();
	LocationAlongSpline = MaxLocation > 0.0F ? FGenericPlatformMath::Fmod(LocationAlongSpline, MaxLocation) : 0.0F;
}

/**
* Never checks whether we are in the goal state - always performs the state transition.
* @returns: true if went to attaching or directly to attached state.
*/
bool USplineMovementComponentImpl::GotoState_Attaching()
{
	M_LOGFUNC();
	checkf( ! GetWorld()->IsGameWorld(), TEXT("\"%s\" is designed to be called in game worlds only"), TEXT(__FUNCTION__));
	checkf( GetSplineComponent(), TEXT("When calling \"%s\" the spline component must be valid non-NULL pointer"), TEXT(__FUNCTION__));
	checkf( GetUpdatedComponent(), TEXT("When calling \"%s\" the updated primitive must be valid non-NULL pointer"), TEXT(__FUNCTION__));

	// Should we keep world location at least
	bool bKeepWorldLocation = (GetConfig().AttachRules.AttachTransformMode == ESplineMovementAttachTransformMode::KeepWorld)
		|| (GetConfig().AttachRules.AttachTransformMode == ESplineMovementAttachTransformMode::KeepWorldLocationOnly);

	// Should we get both location and rotation components from the world transform?
	bool const bKeepWorldLocationAndRotation = (GetConfig().AttachRules.AttachTransformMode == ESplineMovementAttachTransformMode::KeepWorld);
	
	float const AttachBlendTime = GetConfig().AttachRules.AttachBlendTime;
	bool const bAttachInstantly = bKeepWorldLocationAndRotation || (AttachBlendTime == 0.0F);

	if( bAttachInstantly )
	{
		BeforeMovementAttachedToSpline.Broadcast(GetAttachState());
	}
	else
	{
		BeforeMovementBeginAttachingToSpline.Broadcast(GetAttachState());
	}

	// Should we keep world rotation
	bool const bKeepWorldRotation = bKeepWorldLocationAndRotation;

	{
		ESplineMovementSimulationResetFlags SimulationResetFlags = ESplineMovementSimulationResetFlags::None;
		if(bKeepWorldLocation)
		{
			SimulationResetFlags |= ESplineMovementSimulationResetFlags::KeepWorldSpaceLocation;
		}
		if(bKeepWorldRotation)
		{
			SimulationResetFlags |= ESplineMovementSimulationResetFlags::KeepWorldSpaceRotation;
		}
		ResetSplineMoveSpaceAndParamsFromWorldSpace(GetUpdatedComponent(), SimulationResetFlags);
	}

	if(bAttachInstantly)
	{
		{	
			FTransform const NewTransform = LocalToMoveSpace * MoveSpace.Transform;
			FVector const DeltaLocation = NewTransform.GetLocation() - GetUpdatedComponent()->GetComponentLocation();
			FRotator const NewRotation = NewTransform.Rotator();
			bool constexpr bSweep = false;
			FHitResult Hit;
			bool const bMoved = MovementComponent->MoveUpdatedComponent(DeltaLocation, NewRotation, bSweep, &Hit);
		}
		// We already signaled before this event
		bool const bSignalBeforeAttachedEvent = false;
		return GotoState_Attached(bSignalBeforeAttachedEvent);
	}
	else
	{
		M_LOG(TEXT("GotoState: Attaching (Smooth transition)"));

		// @TODO: It it really necessary? Looks like bug!
		// Move-space location offset is not used in the detached state, 
		// so to make smooth transition, we must reset the target blend destination location.
		//LocalToMoveSpace.SetLocation(FVector::ZeroVector);
		FTransform const OldMoveSpaceToWorld = GetMoveSpaceToWorld_ForFreeMovement(GetUpdatedComponent());
		AttachState.SetAttaching(OldMoveSpaceToWorld);
		MovementBeginAttachingToSpline.Broadcast();
		return true;
	}
}
bool USplineMovementComponentImpl::GotoState_Detached()
{
	M_LOGFUNC();
	BeforeMovementDetachedFromSpline.Broadcast(GetAttachState());
	AttachState.SetDetached();
	MovementDetachedFromSpline.Broadcast();
	return true;
}

/**
* Never checks whether we are in the goal state - always performs the state transition.
* Never changes the transform of the updated component - always keeps as is (@see GotoState_Attaching for that case).
* @returns: true if went to attached state.
*/
bool USplineMovementComponentImpl::GotoState_Attached(bool const bInSignalBeforeEvent)
{
	M_LOGFUNC();
	if(bInSignalBeforeEvent)
	{
		BeforeMovementAttachedToSpline.Broadcast(GetAttachState());
	}
	AttachState.SetAttached();
	// @TODO: Handle tracking speed
	MovementAttachedToSpline.Broadcast();
	return true;
}

// ~ FSplineMovementAttachmentState Begin
void FSplineMovementAttachmentState::SetAttached()
{
	State                  = ESplineMovementAttachState::Attached;
	AttachingTime          = 0.0F;
}

void FSplineMovementAttachmentState::SetAttaching(const FTransform& InMoveSpaceToWorldBeforeAttaching)
{
	State                                  = ESplineMovementAttachState::Attaching;
	MoveSpaceToWorld_BeforeAttaching       = InMoveSpaceToWorldBeforeAttaching;
	AttachingTime                          = 0.0F;
}

void FSplineMovementAttachmentState::SetDetached()
{
	State                  = ESplineMovementAttachState::Detached;
	AttachingTime          = 0.0F;
}
// ~ FSplineMovementAttachmentState End
//
UWorld* USplineMovementComponentImpl::GetWorld() const
{
	return MovementComponent->GetWorld();
}
