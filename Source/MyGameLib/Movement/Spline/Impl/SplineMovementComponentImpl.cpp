#include "SplineMovementComponentImpl.h"
#include "Util/Core/LogUtilLib.h"
#include "GameFramework/MovementComponent.h"
#include "Components/SplineComponent.h"

/**
* @TODO:
* 1. RecalculateStateRelativeToSpline:
* 1.1. Spline input key to location along spline
*
* 2. SetPendingInputVector
*/

USplineMovementComponentImpl::USplineMovementComponentImpl()
{
}

USplineMovementComponentImpl* USplineMovementComponentImpl::CreateSplineMovementComponentImpl(FName InObjectName, UMovementComponent* InMovementComponent, const FSplineMovementConfig* pInConfig)
{
	checkf(InMovementComponent, TEXT("When calling \"%s\" the given movement component must be valid non-NULL pointer"), TEXT(__FUNCTION__));
	checkf(pInConfig, TEXT("When calling \"%s\" the given config pointer must be valid non-NULL pointer"), TEXT(__FUNCTION__));
	USplineMovementComponentImpl* const Obj = InMovementComponent->CreateDefaultSubobject<USplineMovementComponentImpl>(InObjectName);
	Obj->MovementComponent = InMovementComponent;
	Obj->pConfig = pInConfig;
	return Obj;

}

bool USplineMovementComponentImpl::ShouldSkipUpdate_BeforeSuper(float DeltaTime)
{
	if(SplineComponent == nullptr)
	{
		return true;
	}

	if(MovementComponent->ShouldSkipUpdate(DeltaTime))
	{
		return true;
	}

	return false;
}

void USplineMovementComponentImpl::SetPendingInputVector(float DeltaTime, const FVector& InInputVector)
{
	// @TODO
}

void USplineMovementComponentImpl::UpdateFromConfig()
{
	M_LOGFUNC();
	ReattachToSpline();
}

void USplineMovementComponentImpl::ReattachToSpline()
{
	M_LOGFUNC();
	if(GetConfig().SplineProvider == nullptr)
	{
		SplineComponent = nullptr;
	}	
	else
	{
		SplineComponent = GetConfig().SplineProvider->FindComponentByClass<USplineComponent>();
	}
	if(SplineComponent == nullptr)
	{
		return;
	}
	RecalculateStateRelativeToSpline();
}

void USplineMovementComponentImpl::RecalculateStateRelativeToSpline()
{
	// @TODO
	M_LOGFUNC();
	SplineState.LocationAlongSpline = 0;
	SplineSpaceTransform = FTransform{};
}
