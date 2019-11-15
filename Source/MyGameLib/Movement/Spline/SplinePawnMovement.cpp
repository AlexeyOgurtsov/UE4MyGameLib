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
	
//~ Begin ActorComponent Interface 
void USplinePawnMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if(Impl->ShouldSkipUpdate_BeforeSuper(DeltaTime))
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
	Impl->SetPendingInputVector(DeltaTime, GetPendingInputVector());
	
	// Finalize
	ConsumeInputVector();
	UpdateComponentVelocity();
}
//~ End ActorComponent Interface 

// ~ UObject Begin
void USplinePawnMovement::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.Property)
	{
		if(PropertyChangedEvent.Property->GetFName() == FName(TEXT("Config")))
		{
			Impl->UpdateFromConfig();
		}
	}
}
// ~ UObject End

USplineComponent* USplinePawnMovement::GetSplineComponent() const
{
	return Impl->GetSplineComponent();
}
