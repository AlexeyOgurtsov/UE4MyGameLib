#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "Movement/Spline/SplineMovementConfig.h"
#include "SplinePawnMovement.generated.h"

class USplineComponent;
class USplineMovementComponentImpl;

UCLASS()
class USplinePawnMovement : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	USplinePawnMovement();
	
	//~ Begin ActorComponent Interface 
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//~ End ActorComponent Interface 
	
	// ~ UObject Begin
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;	
	// ~ UObject End
	
	/** GetConfig*/
	const FSplineMovementConfig& GetConfig() const { Config; }
	
	// ~ Spline Begin
	/** GetSpline*/
	USplineComponent* GetSplineComponent() const;
	// ~ Spline End
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Meta=(AllowPrivateAccess=true), Category = Config)
	FSplineMovementConfig Config;
	
	UPROPERTY()
	USplineMovementComponentImpl* Impl = nullptr;
};

