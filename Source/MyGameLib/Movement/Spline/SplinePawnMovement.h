#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "Movement/Spline/SplineMovementTypes.h"
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
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//~ End ActorComponent Interface 
	
	// ~ UObject Begin
	virtual void PostInitProperties() override;
	// ~ UObject End
	
	// ~AActor Begin
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;	
	#endif // WITH_EDITOR
	// ~AActor End
	
	// ~UMovementComponent Begin
	virtual void StopMovementImmediately() override;
	virtual void OnTeleported() override;
	// ~UMovementComponent End
	
	/** GetConfig*/
	const FSplineMovementConfig& GetConfig() const { Config; }
	
	// ~ Spline Begin
	UFUNCTION(BlueprintGetter, Category = Spline)
	USplineComponent* GetSplineComponent() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	ESplineMovementAttachState GetAttachState() const;

	FBeforeMovementAttachedToSplineEvent& OnBeforeMovementAttachedToSpline();
	FBeforeMovementBeginAttachingToSplineEvent& OnBeforeMovementBeginAttachingToSpline();
	FBeforeMovementDetachedFromSplineEvent& OnBeforeMovementDetachedFromSpline();
	FMovementAttachedToSplineEvent& OnMovementAttachedToSpline();
	FMovementBeginAttachingToSplineEvent& OnMovementBeginAttachingToSpline();
	FMovementDetachedFromSplineEvent& OnMovementDetachedFromSpline();

	UFUNCTION(BlueprintGetter, Category = Spline)
	bool IsFreeMovement() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	bool IsMovementAttachedToSpline() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	bool IsAttachingMovementToSplineNow() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	bool IsMovementAttachedOrAttachingToSpline();
	
	UFUNCTION(BlueprintGetter, Category = Spline)
	float GetAttachingTime() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	FTransform GetLocalToMoveSpace() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	float GetLocationAlongSpline() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	FVector GetMoveSpaceVelocity(bool bInAddTrackSpeed = true) const;

	UFUNCTION(BlueprintCallable, Category = Spline)
	void SetVelocityInMoveSpace(const FVector& InVelocity, bool bTrackingAccountedInVelocity = false);

	UFUNCTION(BlueprintCallable, Category = Spline)
	void SetVelocityInWorldSpace(const FVector& InVelocity, bool bTrackingAccountedInVelocity = false);

	UFUNCTION(BlueprintGetter, Category = Spline)
	float GetTrackingSpeed() const;

	UFUNCTION(BlueprintSetter, Category = Spline)
	void SetTrackingSpeed(float InTargetSpeed);

	UFUNCTION(BlueprintGetter, Category = Spline)
	float GetTargetTrackingSpeed() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	FTransform GetSplineToWorld() const;

	UFUNCTION(BlueprintGetter, Category = Spline)
	const FTransform& GetMoveSpaceToWorld() const;

	UFUNCTION(BlueprintCallable, Category = Spline)
	bool AttachToSpline();

	UFUNCTION(BlueprintCallable, Category = Spline)
	bool DetachFromSpline();

	UFUNCTION(BlueprintCallable, Category = Spline)
	bool ToggleAttachToSpline();

	UFUNCTION(BlueprintCallable, Category = Spline)
	void SetLocationAlongSpline(float NewLocationAlongSpline);
	// ~ Spline End
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Meta=(AllowPrivateAccess=true), Category = Config)
	FSplineMovementConfig Config;
	
	UPROPERTY()
	USplineMovementComponentImpl* Impl = nullptr;
};

