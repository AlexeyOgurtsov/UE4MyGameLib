#pragma once

#include "SplineMovementConfig.generated.h"

/**
* Config, that is common both for actor and pawn spline movement components.
*/
USTRUCT(BlueprintType)
struct FSplineMovementConfig
{
	GENERATED_BODY()

	/** Provider of the spline component*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* SplineProvider = nullptr;
};
