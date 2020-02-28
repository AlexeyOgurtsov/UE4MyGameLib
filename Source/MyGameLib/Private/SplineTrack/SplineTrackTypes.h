#pragma once

#include "Components/SplineMeshComponent.h" // ESplineMeshAxis::Type
#include "SplineTrackTypes.generated.h"

class UStaticMesh;

USTRUCT(BlueprintType, Category=SplineTrack)
struct FSplineTrackSegment
{
	GENERATED_BODY()

	/** Mesh*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis = ESplineMeshAxis::Type::Z;

	/** Default ctor */
	FSplineTrackSegment()
	{
	}

	/** Constructor from static mesh */
	explicit FSplineTrackSegment(UStaticMesh* InMesh) :
		Mesh (InMesh)
	{
	}
};

