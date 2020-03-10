#pragma once

#include "GameMathTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "GameMath.generated.h"

UCLASS()
class UGameMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** GetFloatUpdatedToTarget*/
	UFUNCTION(BlueprintCallable, Category=GameMath, Meta=(DisplayName="GetFloatUpdatedToTarget"))
	static float K2_GetFloatUpdatedToTarget(float InDeltaTime, float InCurrValue, float InTargetValue, const FGameFloatUpdate& InUpdate);

	/** GetFloatUpdatedToTarget*/
	static float GetFloatUpdatedToTarget(float InDeltaTime, float InCurrValue, float InTargetValue, const FGameFloatUpdate& InUpdate, float InErrorTolerance = SMALL_NUMBER);

	/** 
	 * Computes rotation to apply to the X axis to aim at the given target.
	 * Only Pitch and Roll components are computed.
	 */
	UFUNCTION(BlueprintCallable, Category=GameMath)
	static FRotator ComputeXAxisAimRotation(FVector TargetLocation);
};
