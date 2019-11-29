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
	// NOTE default property values are also set inside the Impl
	Impl = USplineMovementComponentImpl::CreateSplineMovementComponentImpl(TEXT("SplinePawnMovementComponentImpl"), this, &Config, &Delegates);
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

	if(Controller->IsLocalController())
	{
		if(Controller->IsLocalPlayerController() || ! Controller->IsFollowingAPath())
		{
			// @TODO: Check: we already must set the input vector when calling the AddMovementInput!
			//Impl->SetPendingInputVector(GetPendingInputVector());
		}
	}

	Impl->MoveTick(DeltaTime);
	
	// Finalize
	Impl->FinalizeTick();
	ConsumeInputVector();
}
//~ End ActorComponent Interface 

// ~UMovementComponent Begin
float USplinePawnMovement::GetGravityZ() const
{
	float const GravityZ = Super::GetGravityZ();
	return Impl->GetComponentGravityZ(GravityZ);
}

float USplinePawnMovement::GetMaxSpeed() const 
{
	return Impl->GetMaxSpeed();
}

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

// ~UPawnMovementComponent Begin
void USplinePawnMovement::AddInputVector(FVector const WorldVector, bool const bForce)
{
	Super::AddInputVector(WorldVector, bForce);
	Impl->SetPendingInputVector(GetPendingInputVector());
}

// ~UPawnMovementComponent End

void USplinePawnMovement::AddMoveSpaceMovementInput(FVector const MoveSpaceVector, bool const bForce)
{
	if( ! IsMoveInputIgnored() || bForce )
	{
		FVector const WorldSpaceVector = Impl->AddMoveSpaceMovementInput(MoveSpaceVector);
		// Warning! We must call the UPawnMovementComponent::AddInputVector (i.e. Super call),
		// because the overloaded version would also calculate the move-space vector which is already calculated
		Super::AddInputVector(WorldSpaceVector, bForce);
		Impl->SetOnlyWorldSpacePendingInputVector(GetPendingInputVector());
	}
}

const FVector& USplinePawnMovement::GetMoveSpacePendingInputVector() const
{
	return Impl->GetMoveSpacePendingInputVector();
}

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

void USplinePawnMovement::SetVelocityInMoveSpace(const FVector& InVelocity, bool const bTrackingAccountedInVelocity)
{
	return Impl->SetVelocityInMoveSpace(InVelocity, bTrackingAccountedInVelocity);
}

void USplinePawnMovement::SetVelocityInWorldSpace(const FVector& InVelocity, bool const bTrackingAccountedInVelocity)
{
	return Impl->SetVelocityInWorldSpace(InVelocity, bTrackingAccountedInVelocity);
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

const FTransform& USplinePawnMovement::GetMoveSpaceToWorld() const
{
	return Impl->GetMoveSpaceToWorld();
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
