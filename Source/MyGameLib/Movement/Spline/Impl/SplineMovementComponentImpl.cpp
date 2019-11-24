#include "SplineMovementComponentImpl.h"
#include "Util/Core/LogUtilLib.h"
#include "GameFramework/MovementComponent.h"
#include "Components/SplineComponent.h"
#include "Math/UnrealMathUtility.h"
#include "GenericPlatform/GenericPlatformMath.h"

/**
* @TODO Events:
* 1. Attaching/Detaching/Attached states etc.
* 1.0. Call the events (+DONE)
* 1.1. Make MovementCompoent's events bindable from Blueprint
*
* @TODO
* 1. Attaching to spline dynamically
* 1.2. Calculate initial transform for KeepWorld attachment mode
*
* @TODO: Detaching from spline dynamically:
* 1. After the detachment we must always keep current transform in world space (ever when detaching from attaching with blending state!).
*
* @TODO Free Mode
* 1. Rotation is to be rotated by the SplineSpaceTransform's rotation relative to the movement axis
*
* @TODO Handling Tracking Speed
* 1. Allow to disable it (necessary for stop immediately)
* 2. Provide tracking acceleration and deceleration
*
* @TODO Stop movement immediately (+DONE)
*
* @TODO Check world bounds
*
* @TODO Check max speed
*
* @TODO Update velocity
*
* @TODO On Teleported
*
* @TODO Receiving a blocking hit: we should alter our spline space velocity according to the updated velocity
*
* @TODO Slide along surface
*
* @TODO Gravity:
* 1. To be considered when free movement is used and the physics mode is not flying.
*/

USplineMovementComponentImpl::USplineMovementComponentImpl()
{
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
}

void USplineMovementComponentImpl::MoveTick(float const DeltaTime)
{
	// Warning: we must perform the state transitions earlier then the state-dependent variables calculation.
	if(AttachState.State == ESplineMovementAttachState::Attaching)
	{
		AttachState.AttachingTime += DeltaTime;
		if(AttachState.AttachingTime >= GetConfig().AttachRules.AttachBlendTime)
		{
			GotoState_Attached();
		}
	}

	// All initializer values here are to be valid for the detached state!
	bool bTargetDestinationOnSpline = false; // Is target destination on spline?
	// Is arbitrary movement control is allowed in this state
	bool bAllowMoveControl = true;
	bool bSweep = true;
	switch(AttachState.State)
	{
	case ESplineMovementAttachState::Detached:
		// Using the initialized variable values in this state
		break;

	case ESplineMovementAttachState::Attaching:
		bTargetDestinationOnSpline = true;
		bAllowMoveControl = false;
		bSweep = GetConfig().AttachRules.bAttachSweep;
		break;
	
	case ESplineMovementAttachState::Attached:
		bTargetDestinationOnSpline = true;
		bAllowMoveControl = true;
		bSweep = true;
		break;

	default:
		M_LOG_ERROR(TEXT("Unknown spline movement attach state"));
		checkNoEntry();
	}
	// Transform from space, in which the movement is calculated now to the world space
	FTransform MoveSpaceTransform { ENoInit::NoInit };
	if(bTargetDestinationOnSpline)
	{
		MoveSpaceTransform = GetSplineToWorld();
	}
	else
	{
		MoveSpaceTransform = GetUpdatedComponent()->GetComponentTransform();
	}
	
	FVector const MoveSpaceInputVector = MoveSpaceTransform.InverseTransformVectorNoScale(InputVector);	
	FVector const MoveSpaceControlAcceleration = CalculateAccelerationFromInputVector(MoveSpaceInputVector, Phys.MoveSpaceVelocity);
	// Acceleration vector in the coordinate space we move and control now
	FVector const MoveSpaceAcceleration = bAllowMoveControl ? MoveSpaceControlAcceleration : FVector::ZeroVector;

	RecalculateTrackingSpeed(DeltaTime);

	// MoveDelta in the Move space (@see ComputeMoveDelta help)
	FVector const MoveDelta = ComputeMoveDelta(DeltaTime, MoveSpaceAcceleration);

	FVector DeltaLocation; // Delta Location In World Space
	FRotator NewRotation; // New Rotation in World Space
	{
		if(bTargetDestinationOnSpline)
		{
			{
				SplineSpaceTransform.AddToTranslation(MoveDelta);
				LocationAlongSpline += SplineSpaceTransform.GetLocation().X;
				FixSplineTransformAndLocation();
			}
	
			FTransform const TargetTransform = SplineSpaceTransform * MoveSpaceTransform;

			// WARNING! We should not check here whether we should blend based on the config,
			// because the go-to-state functions should already check it.
			bool const bWithBlend = (AttachState.State == ESplineMovementAttachState::Attaching);
			FTransform NewTransform { ENoInit::NoInit };
			if(bWithBlend)
			{
				float const AttachBlendTime = GetConfig().AttachRules.AttachBlendTime;
				// Blend between Target Transform and the DetachedTransform, so we are attaching smoothly.
				NewTransform.Blend( AttachState.DetachedTransform, TargetTransform, AttachState.AttachingTime / AttachBlendTime );
			}
			else
			{
				NewTransform = TargetTransform;
			}
			DeltaLocation = NewTransform.GetLocation() - GetUpdatedComponent()->GetComponentLocation();
			NewRotation = NewTransform.Rotator();
		}
		else
		{
			checkf( ! bTargetDestinationOnSpline, TEXT("We on the branch where target destination is not on the spline") );
			DeltaLocation = MoveSpaceTransform.TransformVectorNoScale(MoveDelta);
			NewRotation = GetUpdatedComponent()->GetComponentRotation();
		}
	}

	{
		FHitResult Hit;
		bool const bMoved = MovementComponent->MoveUpdatedComponent(DeltaLocation, NewRotation, bSweep, &Hit);
		// @TODO: Handle hit + slide
	}

	// @TODO: Attaching state goes to detached when we received a blocking hit
}

void USplineMovementComponentImpl::OnComponentTeleported()
{
	M_LOGFUNC();
	// @TODO
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

FTransform USplineMovementComponentImpl::GetSplineToWorld() const
{
	return GetSplineComponent()->GetTransformAtDistanceAlongSpline(GetLocationAlongSpline(), ESplineCoordinateSpace::Type::World);
}

void USplineMovementComponentImpl::RecalculateTrackingSpeed(float const DeltaTime)
{
	float const TargetSpeed = GetTargetTrackingSpeed();
	if(FMath::IsNearlyEqual(TargetSpeed, Phys.Tracking.Speed))
	{
		Phys.Tracking.Speed = TargetSpeed;
	}
	else
	{
		if(TargetSpeed - GetTrackingSpeed() < 0)
		{
			Phys.Tracking.Speed -= DeltaTime * GetConfig().Phys.Tracking.Deceleration;
			Phys.Tracking.Speed = FMath::Max(TargetSpeed, Phys.Tracking.Speed);
		}	
		else
		{
			Phys.Tracking.Speed += DeltaTime * GetConfig().Phys.Tracking.Acceleration;
			Phys.Tracking.Speed = FMath::Min(TargetSpeed, Phys.Tracking.Speed);
		}
	}
}

/**
* Simulates the new move and returns the delta of location.
* Updates the Velocity.
*
* @returns Location delta vector in move space
* @param: InAcceleration: Acceleration in the move space
* @param bAddTrackSpeed: if true, then add tracking speed component to the forward velocity part
*/
FVector USplineMovementComponentImpl::ComputeMoveDelta(float const DeltaTime, const FVector& InAcceleration)
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

void USplineMovementComponentImpl::SetTrackingSpeed(float InTargetSpeed)
{
	Phys.Tracking.TargetSpeed = FMath::Clamp(InTargetSpeed, 0.0F, 50000.0F);
}

float USplineMovementComponentImpl::GetTargetTrackingSpeed() const
{
	return Phys.Tracking.TargetSpeed;
}

/**
* Calculates acceleration based on the input vector value.
*
* @return Target Acceleration in the same coordinate space as the input vector
* @param InInputVector Input vector in arbitrary coordinate space
* @param InOldVelocity Input velocity in the same coordinate space as the input vector
*/
FVector USplineMovementComponentImpl::CalculateAccelerationFromInputVector(const FVector& InInputVector, const FVector& InOldVelocity) const
{
	// Deceleration values represented as a vector.
	// Each values is negative.
	// Each value's absolute is not greater then velocity's absolute in the corresponding local axis.
	FVector const DecelerationVector = -InOldVelocity.GetAbs().ComponentMin
	(
		FVector
		{ 
			GetConfig().Phys.ForwardDeceleration, 
			GetConfig().Phys.StrafeDeceleration, 
			GetConfig().Phys.LiftDeceleration 
		}
	);
	FVector const AccelerationVector = 
	{
		GetConfig().Phys.ForwardAcceleration, 
		GetConfig().Phys.StrafeAcceleration, 
		GetConfig().Phys.LiftAcceleration 
	};
	auto const GetAccelerationComponent = [&, DecelerationVector, AccelerationVector, InInputVector](EAxis::Type InAxis)
	{
		if(FMath::IsNearlyZero(InInputVector.GetComponentForAxis(InAxis)))
		{
			return DecelerationVector.GetComponentForAxis(InAxis);
		}
		else
		{
			return AccelerationVector.GetComponentForAxis(InAxis);
		}
	};
	return InOldVelocity.GetSignVector() * InInputVector.GetSignVector() * FVector
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
	if(SplineProvider != GetConfig().SplineProvider.Get(false))
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
	SplineProvider = GetConfig().SplineProvider.Get(false);
	if(SplineProvider == nullptr)
	{
		M_LOG(TEXT("Spline provider is NULL"))
		SplineComponent = nullptr;
	}	
	else
	{
		M_LOG(TEXT("Spline provider is NON-null pointer"))
		ULogUtilLib::LogKeyedNameClassSafeC(TEXT("Spline provider"), SplineProvider);
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

void USplineMovementComponentImpl::ResetToInitialTransformAndLocation()
{
	SplineSpaceTransform = GetConfig().AttachRules.InitialSplineTransform;
	LocationAlongSpline = GetConfig().AttachRules.InitialSplineTransform.GetLocation().X;
	FixSplineTransformAndLocation();
}

void USplineMovementComponentImpl::FixSplineTransformAndLocation()
{
	FixLocationAlongSpline();
	SplineSpaceTransform.SetLocation(FVector{0.0F, SplineSpaceTransform.GetLocation().Y, SplineSpaceTransform.GetLocation().Z});
}

void USplineMovementComponentImpl::FixLocationAlongSpline()
{
	if(GetSplineComponent() == nullptr)
	{
		return;
	}
	LocationAlongSpline = FGenericPlatformMath::Fmod(LocationAlongSpline, GetSplineComponent()->GetSplineLength());
}

/**
* Never checks whether we are in the goal state - always performs the state transition.
* @returns: true if went to attaching or directly to attached state.
*/
bool USplineMovementComponentImpl::GotoState_Attaching()
{
	M_LOGFUNC();
	checkf( GetSplineComponent(), TEXT("When calling \"%s\" the spline component must be valid non-NULL pointer"), TEXT(__FUNCTION__));
	checkf( GetUpdatedComponent(), TEXT("When calling \"%s\" the updated primitive must be valid non-NULL pointer"), TEXT(__FUNCTION__));

	// Should we keep world location at least
	bool bKeepWorldLocation = (GetConfig().AttachRules.AttachTransformMode == ESplineMovementAttachTransformMode::KeepWorld)
		|| (GetConfig().AttachRules.AttachTransformMode == ESplineMovementAttachTransformMode::KeepWorldLocationOnly);

	// @TODO: Implement keeping world location when attaching 
	// (WARNG!!! NOT right here! There's a TODO placeholder below this block!!!)
	if(bKeepWorldLocation)
	{
		{
			M_TO_BE_IMPL(TEXT("Keeping World Location is NOT yet implemented - using KeepSplineTransform instead"));
			pConfig->AttachRules.AttachTransformMode = ESplineMovementAttachTransformMode::KeepSplineTransform;
			bKeepWorldLocation = false;
		}
	}

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
		// WARNING! This if should be executed before the rotation if,
		// because the rotation if depends on the updated location along the spline!
		if(bKeepWorldLocation)
		{
			// @TODO: Recalculate location along the spline

			// @TODO: Recalculate new SplineSpaceTransform location
		}

		if(bKeepWorldRotation)
		{
			// @TODO: Recalculate new SplineSpaceTransform rotation
		}
	}


	if(bAttachInstantly)
	{
		{
			// Move to the new transform in spline space
			FTransform const NewTransform = SplineSpaceTransform * GetSplineToWorld();
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
		M_LOG(TEXT("GotoState: Attaching"));
		FTransform const OldTransform = GetUpdatedComponent()->GetComponentTransform();
		AttachState.SetAttaching(OldTransform);
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

void FSplineMovementAttachmentState::SetAttaching(const FTransform& InDetachedTransform)
{
	State                  = ESplineMovementAttachState::Attaching;
	DetachedTransform      = InDetachedTransform;
	AttachingTime          = 0.0F;
}

void FSplineMovementAttachmentState::SetDetached()
{
	State                  = ESplineMovementAttachState::Detached;
	AttachingTime          = 0.0F;
}
// ~ FSplineMovementAttachmentState End
