#include "SplinePawnMovement.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Impl/SplineMovementComponentImpl.h"
#include "Util/Core/LogUtilLib.h"

/**
* @TODO:
*
* 1. Delegating PendingInputVector (TickComponent)
* Check for AI Controller case
*/

USplinePawnMovement::USplinePawnMovement()
{
	Impl = USplineMovementComponentImpl::CreateSplineMovementComponentImpl(TEXT("SplinePawnMovementComponentImpl"), this, &Config);
}

void USplinePawnMovement::PostInitProperties()
{
	Super::PostInitProperties();
	Impl->MovementComponentPostInitProperties();
}
	
//~ Begin ActorComponent Interface 
void USplinePawnMovement::BeginPlay()
{
	Impl->MyBeginPlay();
}

void USplinePawnMovement::TickComponent(float const DeltaTime, enum ELevelTick const TickType, FActorComponentTickFunction * const ThisTickFunction)
{
	if(Impl->TickBeforeSuper_ReturnShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent)
	{
		return;
	}

	const AController* const Controller = PawnOwner->GetController();

	if(Controller == nullptr)
	{
		return;
	}

	// @TODO: Check for AI Controller case
	Impl->SetPendingInputVector(GetPendingInputVector());

	Impl->MoveTick(DeltaTime);
	
	// Finalize
	Impl->FinalizeTick();
	ConsumeInputVector();
}
//~ End ActorComponent Interface 

// ~UMovementComponent Begin
void USplinePawnMovement::StopMovementImmediately()
{
	Super::StopMovementImmediately();
	Impl->StopMovementImmediately();
}

void USplinePawnMovement::OnTeleported()
{
	Super::OnTeleported();
	Impl->OnComponentTeleported();
}
// ~UMovementComponent End

// ~ UObject Begin
#if WITH_EDITOR
void USplinePawnMovement::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.Property)
	{
		if(PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USplinePawnMovement, Config))
		{
			Impl->UpdateFromConfig();
		}
	}
}
#endif // WITH_EDITOR
// ~ UObject End

USplineComponent* USplinePawnMovement::GetSplineComponent() const
{
	return Impl->GetSplineComponent();
}

ESplineMovementAttachState USplinePawnMovement::GetAttachState() const
{
	return Impl->GetAttachState();
}

FBeforeMovementAttachedToSplineEvent& USplinePawnMovement::OnBeforeMovementAttachedToSpline()
{
	return Impl->OnBeforeMovementAttachedToSpline();
}

FBeforeMovementBeginAttachingToSplineEvent& USplinePawnMovement::OnBeforeMovementBeginAttachingToSpline()
{
	return Impl->OnBeforeMovementBeginAttachingToSpline();
}

FBeforeMovementDetachedFromSplineEvent& USplinePawnMovement::OnBeforeMovementDetachedFromSpline()
{
	return Impl->OnBeforeMovementDetachedFromSpline();
}

FMovementAttachedToSplineEvent& USplinePawnMovement::OnMovementAttachedToSpline()
{
	return Impl->OnMovementAttachedToSpline();
}

FMovementBeginAttachingToSplineEvent& USplinePawnMovement::OnMovementBeginAttachingToSpline()
{
	return Impl->OnMovementBeginAttachingToSpline();
}

FMovementDetachedFromSplineEvent& USplinePawnMovement::OnMovementDetachedFromSpline()
{
	return Impl->OnMovementDetachedFromSpline();
}

bool USplinePawnMovement::IsFreeMovement() const
{
	return Impl->IsFreeMovement();
}

bool USplinePawnMovement::IsMovementAttachedToSpline() const
{
	return Impl->IsMovementAttachedToSpline();
}

bool USplinePawnMovement::IsAttachingMovementToSplineNow() const
{
	return Impl->IsAttachingMovementToSplineNow();
}

bool USplinePawnMovement::IsMovementAttachedOrAttachingToSpline()
{
	return Impl->IsMovementAttachedOrAttachingToSpline();
}

float USplinePawnMovement::GetAttachingTime() const
{
	return Impl->GetAttachingTime();
}

FTransform USplinePawnMovement::GetLocalToMoveSpace() const
{
	return Impl->GetLocalToMoveSpace();
}

float USplinePawnMovement::GetLocationAlongSpline() const
{
	return Impl->GetLocationAlongSpline();
}

FVector USplinePawnMovement::GetMoveSpaceVelocity(bool const bInAddTrackSpeed) const
{
	return Impl->GetMoveSpaceVelocity(bInAddTrackSpeed);
}

float USplinePawnMovement::GetTrackingSpeed() const
{
	return Impl->GetTrackingSpeed();
}

void USplinePawnMovement::SetTrackingSpeed(float InTargetSpeed)
{
	Impl->SetTrackingSpeed(InTargetSpeed);
}

float USplinePawnMovement::GetTargetTrackingSpeed() const
{
	return Impl->GetTargetTrackingSpeed();
}

FTransform USplinePawnMovement::GetSplineToWorld() const
{
	return Impl->GetSplineToWorld();
}

bool USplinePawnMovement::AttachToSpline()
{
	return Impl->AttachToSpline();
}

bool USplinePawnMovement::DetachFromSpline()
{
	return Impl->DetachFromSpline();
}

bool USplinePawnMovement::ToggleAttachToSpline()
{
	return Impl->ToggleAttachToSpline();
}

void USplinePawnMovement::SetLocationAlongSpline(float const NewLocationAlongSpline)
{
	return Impl->SetLocationAlongSpline(NewLocationAlongSpline);
}
