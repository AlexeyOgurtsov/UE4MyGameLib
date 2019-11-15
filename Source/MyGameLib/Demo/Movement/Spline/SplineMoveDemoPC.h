#pragma once

/**
* To be copied to the destination and changed.
*/

#include "Util/TestUtil/TUPlayerController.h"
#include "Util/Core/Log/MyLoggingTypes.h"
#include "SplineMoveDemoPC.generated.h"

class ASplineMoveDemoPawn;

UCLASS()
class ASplineMoveDemoPC : public ATUPlayerController
{
	GENERATED_BODY()

public:
	ASplineMoveDemoPC();

	// ~AActor Begin
	virtual void BeginPlay() override;
	// ~AActor End

	// ~Access helpers Begin
	UFUNCTION(BlueprintPure, Category = Pawn)
	ASplineMoveDemoPawn* GetMyPawn() const;

	UFUNCTION(BlueprintPure, Category = Pawn)
	ASplineMoveDemoPawn* GetMyPawnLogged(ELogFlags InLogFlags = ELogFlags::None) const;

	/**
	* Checks that the given pawn NOT null.
	*/
	UFUNCTION(BlueprintPure, Category = Pawn)
	ASplineMoveDemoPawn* GetMyPawnChecked() const;
	// ~Access helpers End

private:
};
