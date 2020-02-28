#include "GameMath.h"

float UGameMath::K2_GetFloatUpdatedToTarget(float const InDeltaTime, float const InCurrValue, float const InTargetValue, const FGameFloatUpdate& InUpdate)
{
	return GetFloatUpdatedToTarget(InDeltaTime, InCurrValue, InTargetValue, InUpdate);
}

float UGameMath::GetFloatUpdatedToTarget(float const InDeltaTime, float const InCurrValue, float const InTargetValue, const FGameFloatUpdate& InUpdate, float InErrorTolerance)
{
	float UpdatedValue = InCurrValue;
	if(FMath::IsNearlyEqual(InTargetValue, InCurrValue, InErrorTolerance))
	{
		UpdatedValue = InTargetValue;
	}
	else
	{
		float const DeltaToTarget = InTargetValue - InCurrValue;
		if(DeltaToTarget < 0)
		{
			UpdatedValue -= InDeltaTime * InUpdate.Deceleration;
			UpdatedValue = FMath::Max(InTargetValue, UpdatedValue);
		}	
		else
		{
			UpdatedValue += InDeltaTime * InUpdate.Acceleration;
			UpdatedValue = FMath::Min(InTargetValue, UpdatedValue);
		}
	}
	return UpdatedValue;
}
