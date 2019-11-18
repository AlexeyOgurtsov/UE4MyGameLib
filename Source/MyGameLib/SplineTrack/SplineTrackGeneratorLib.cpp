#include "SplineTrackGeneratorLib.h"
#include "Util/Core/LogUtilLib.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

/**
* @TODO
*/

USplineMeshComponent* USplineTrackGeneratorLib::CreateAttachedSplineSegmentMesh
(
	USplineComponent* const Spline, int32 const SegmentIndex,
	const FSplineTrackSegment& SegmentData,
	USceneComponent* const ParentComponent,
	bool bDynamicObject,
	FName SubobjectName
)
{
	checkf(Spline, TEXT("When calling \"%s\" passed spline component pointer must be valid NON-null pointer"), TEXT(__FUNCTION__));
	checkf(SegmentIndex >=0, TEXT("When calling \"%s\" segment index must be NON-negative"), TEXT(__FUNCTION__));
	int32 const NumSegments = Spline->IsClosedLoop() ? (Spline->GetNumberOfSplinePoints()) : (Spline->GetNumberOfSplinePoints() - 1);
	checkf(SegmentIndex < NumSegments, TEXT("When calling \"%s\" segment index must be less than number of segments"), TEXT(__FUNCTION__));
	checkf(ParentComponent, TEXT("When calling \"%s\" passed parent component pointer must be valid NON-null pointer"), TEXT(__FUNCTION__));
	AActor* const OwnerActor = ParentComponent->GetOwner();
	checkf(OwnerActor, TEXT("When calling \"%s\" owner actor of the passed parent component pointer must be valid NON-null pointer"), TEXT(__FUNCTION__));

	int32 const StartIndex = SegmentIndex;
	int32 const EndIndex = (SegmentIndex < Spline->GetNumberOfSplinePoints()) ? (SegmentIndex + 1) : 0;

	USplineMeshComponent* SplineMesh = nullptr;
	if(bDynamicObject)
	{
		SplineMesh = NewObject<USplineMeshComponent>(OwnerActor, SubobjectName);
	}
	else
	{
		SplineMesh = OwnerActor->CreateDefaultSubobject<USplineMeshComponent>(SubobjectName);
	}
	SplineMesh->SetStaticMesh(SegmentData.Mesh);
	SplineMesh->SetForwardAxis(SegmentData.ForwardAxis, false);

	ESplineCoordinateSpace::Type const SplineCoordSpace = ESplineCoordinateSpace::Type::Local;
	{
		FVector const StartPos = Spline->GetLocationAtSplinePoint(StartIndex, SplineCoordSpace);
		FVector const StartTangent = Spline->GetLeaveTangentAtSplinePoint(StartIndex, SplineCoordSpace);
		FVector const EndPos = Spline->GetLocationAtSplinePoint(EndIndex, SplineCoordSpace);
		FVector const EndTangent = Spline->GetArriveTangentAtSplinePoint(EndIndex, SplineCoordSpace);
		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, false);
	}
	{
		float const StartRoll = Spline->GetRollAtSplinePoint(StartIndex, SplineCoordSpace);
		SplineMesh->SetStartRoll(StartRoll, false);
	}
	{
		float const EndRoll = Spline->GetRollAtSplinePoint(EndIndex, SplineCoordSpace);
		SplineMesh->SetEndRoll(EndRoll, false);
	}

	SplineMesh->SetupAttachment(ParentComponent);
	SplineMesh->UpdateMesh();

	return SplineMesh;
}
