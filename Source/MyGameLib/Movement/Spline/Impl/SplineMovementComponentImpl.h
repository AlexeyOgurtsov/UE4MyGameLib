#pragma once

#include "Movement/Spline/SplineMovementConfig.h"
#include "SplineMovementComponentImpl.generated.h"

class UMovementComponent;
class USplineComponent;

USTRUCT(BlueprintType, Category=Spline)
struct FSplineMovementOnSplineState
{
	GENERATED_BODY()
	
	
	/** LocationAlongSpline*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LocationAlongSpline = 0.0F;
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

	static USplineMovementComponentImpl* CreateSplineMovementComponentImpl(FName InObjectName, UMovementComponent* InMovementComponent, const FSplineMovementConfig* pInConfig);

	/** GetConfig*/
	const FSplineMovementConfig& GetConfig() const { return *pConfig; }

	/**
	* To be called each time the config is updated to reset the component.
	*/
	void UpdateFromConfig();
	
	/** GetMovementComponent*/
	UMovementComponent* GetMovementComponent() const { return MovementComponent; }
	
	/** GetSpline*/
	USplineComponent* GetSplineComponent() const { return SplineComponent; }
	
	// ~From Movement Component Begin
	// (To be delegated from the owning movement component)
	
	/**
	* Typically to be called right before the Super call.
	* Returns true: if tick should be skipped, false, if should be done.
	*/
	bool ShouldSkipUpdate_BeforeSuper(float DeltaTime);

	/**
	* Set input vector at this time.
	* Typically called from TickComponent function.
	*
	* @param InInputVector: Input vector in world space
	* 	Magnitue is always clamped to range 0.0F...1.0F
	* 	for UPawnMovementComponent: GetPendingInputVector() is typically passed.
	*/
	void SetPendingInputVector(float DeltaTime, const FVector& InInputVector);
	// ~From Movement Component End

	/** GetLocationAlongSpline*/
	float GetLocationAlongSpline() const { return SplineState.LocationAlongSpline; }

private:
	/** Config*/
	const FSplineMovementConfig* pConfig = nullptr;

	void ReattachToSpline();

	/** Spline*/
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta=(AllowPrivateAccess=true), Category=Spline)
	USplineComponent* SplineComponent = nullptr;

	void RecalculateStateRelativeToSpline();
	
	/**
	* Spline state
	*/
	UPROPERTY(BlueprintReadOnly, Meta=(AllowPrivateAccess=true), Category=Spline)
	FSplineMovementOnSplineState SplineState;

	/**
	* Transform relative to spline space
	*/
	UPROPERTY(BlueprintReadOnly, Meta=(AllowPrivateAccess=true), Category=Spline)
	FTransform SplineSpaceTransform;

	/** MovementComponent*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta=(AllowPrivateAccess=true), Category=Movement)
	UMovementComponent* MovementComponent = nullptr;
};
