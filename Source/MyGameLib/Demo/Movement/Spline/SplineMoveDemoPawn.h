#pragma once

/**
* To be copied to the destination and changed.
*/

#include "Util/TestUtil/TUPawn.h"
#include "Util/Core/Log/MyLoggingTypes.h"
#include "SplineMoveDemoPawn.generated.h"

UCLASS()
class ASplineMoveDemoPawn : public ATUPawn
{
	GENERATED_BODY()

	// ~Framework Begin
	virtual void MyBeginPlay_Implementation() override;
	// ~Framework End

	// ~Access helpers Begin
	UFUNCTION(BlueprintPure, Category = Controller)
	ASplineMoveDemoPC* GetMyPC() const;

	UFUNCTION(BlueprintPure, Category = Controller)
	ASplineMoveDemoPC* GetMyPCLogged(ELogFlags InLogFlags = ELogFlags::None) const;

	/**
	* Checks that the given pawn NOT null.
	*/
	UFUNCTION(BlueprintPure, Category = Controller)
	ASplineMoveDemoPC* GetMyPCChecked() const;
	// ~Access helpers End

public:
	ASplineMoveDemoPawn();

	UFUNCTION(BlueprintPure, Category = Movement)
	class USplinePawnMovement* GetSplineMovement() const;

	UFUNCTION(BlueprintPure, Category = Movement)
	class USplinePawnMovement* GetSplineMovementChecked() const;

protected:
	// ~Motion Begin
	UFUNCTION(BlueprintCallable, Category = Movement)
	void InitFloatingMovement();

	UFUNCTION(BlueprintCallable, Category = Movement)
	void InitSplineMovement();

	/**
	* Inits default movement: WARNING!!! Never create it native event,
	* as it will be called inside constructor!
	*/
	UFUNCTION(Category = Movement)
	void InitMovement();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Motion, Meta=(AllowPrivateAccess = true))
	UPawnMovementComponent *Movement = nullptr;
	// ~Motion End
};
