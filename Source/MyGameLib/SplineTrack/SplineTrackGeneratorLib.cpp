#include "SplineTrackGeneratorLib.h"
#include "Util/Core/LogUtilLib.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
//#include "Engine/EngineTypes.h" // FAttachmentTransformRules

/**
* @TODO
*/

bool USplineTrackGeneratorLib::ResetUniformSplineTrack
(
	USplineComponent* Spline,
	const FSplineTrackSegment& SegmentTemplate,
	EMyObjectCreationFlags CreationFlags
)
{
	checkf(Spline, TEXT("When calling \"%s\" passed spline component pointer must be valid NON-null pointer"), TEXT(__FUNCTION__));
	AActor* const Actor = Spline->GetOwner();
	checkf(Actor, TEXT("When calling \"%s\" owner of the passed spline component must be valid NON-null pointer"), TEXT(__FUNCTION__));
	DestroyAllSplineMeshComponents(Actor);
	return CreateUniformSplineTrack(Spline, SegmentTemplate, CreationFlags);
}

bool USplineTrackGeneratorLib::ResetUniformSplineTrack_Validate
(
	USplineComponent* Spline,
	const FSplineTrackSegment& SegmentTemplate,
	EMyObjectCreationFlags CreationFlags
)
{
	return CreateUniformSplineTrack_Validate(Spline, SegmentTemplate, CreationFlags);
}

void USplineTrackGeneratorLib::DestroyAllSplineMeshComponents(AActor* Actor)
{
	checkf(Actor, TEXT("When calling \"%s\" passed actor must be valid NON-null pointer"), TEXT(__FUNCTION__));
	TArray<USplineMeshComponent*> SplineMeshes;
	Actor->GetComponents<USplineMeshComponent>(SplineMeshes, /*bIncludeFromChildActors*/false);
	for(USplineMeshComponent* SplineMesh : SplineMeshes)
	{
		SplineMesh->DestroyComponent(/*bPromoteChildren*/false);
	}
}

bool USplineTrackGeneratorLib::DestroyAllSplineMeshComponents_Validate(AActor* Actor)
{
	return Actor != nullptr;
}

USplineMeshComponent* USplineTrackGeneratorLib::CreateAttachedSplineSegmentMesh
(
	USplineComponent* const Spline, int32 const SegmentIndex,
	const FSplineTrackSegment& SegmentData,
	EMyObjectCreationFlags CreationFlags,
	FName SubobjectName
)
{
	checkf(Spline, TEXT("When calling \"%s\" passed spline component pointer must be valid NON-null pointer"), TEXT(__FUNCTION__));
	checkf(SegmentIndex >=0, TEXT("When calling \"%s\" segment index must be NON-negative"), TEXT(__FUNCTION__));
	// @TODO: Refactor as a separate util
	int32 const NumSegments = GetNumberOfSplineTrackSegments(Spline);
	checkf(SegmentIndex < NumSegments, TEXT("When calling \"%s\" segment index must be less than number of segments"), TEXT(__FUNCTION__));
	AActor* const OwnerActor = Spline->GetOwner();
	checkf(OwnerActor, TEXT("When calling \"%s\" owner actor of the passed spline component must be valid NON-null pointer"), TEXT(__FUNCTION__));

	int32 const StartIndex = SegmentIndex;
	int32 const EndIndex = (SegmentIndex < Spline->GetNumberOfSplinePoints()) ? (SegmentIndex + 1) : 0;

	USplineMeshComponent* SplineMesh = nullptr;
	bool const bDynamicObject = (CreationFlags & EMyObjectCreationFlags::Dynamic) != EMyObjectCreationFlags::None;
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
	
	{	

		USceneComponent* const ParentComponent = Spline;
		if(bDynamicObject)
		{
			FAttachmentTransformRules Rules { EAttachmentRule::KeepRelative, /*bWeldSimulatedBodies*/false };
			bool bAttached = SplineMesh->AttachToComponent(ParentComponent, Rules);
			M_LOG_ERROR_IF( ! bAttached, TEXT("USplineMesh::AttachToComponent failed while calling \"%s\""), TEXT(__FUNCTION__) );
			if( ! bAttached )
			{
				return nullptr;
			}
		}
		else
		{
			SplineMesh->SetupAttachment(ParentComponent);
		}
	}
	SplineMesh->UpdateMesh();

	return SplineMesh;
}

bool USplineTrackGeneratorLib::CreateAttachedSplineSegmentMesh_Validate
(
	USplineComponent* Spline, int32 SegmentIndex,
	const FSplineTrackSegment& SegmentData,
	EMyObjectCreationFlags CreationFlags,
	FName SubobjectName
)
{
	return (Spline != nullptr);
}

bool USplineTrackGeneratorLib::CreateUniformSplineTrack
(
	USplineComponent* Spline,
	const FSplineTrackSegment& SegmentTemplate,
	EMyObjectCreationFlags CreationFlags
)
{
	checkf(Spline, TEXT("When calling \"%s\" passed spline component pointer must be valid NON-null pointer"), TEXT(__FUNCTION__));
	// @TODO: Make real parent component
	int32 const NumSegments = GetNumberOfSplineTrackSegments(Spline);
	for(int32 SegmentIndex = 0; SegmentIndex < NumSegments; SegmentIndex++)
	{
		USplineMeshComponent* SplineMesh = CreateAttachedSplineSegmentMesh (Spline, SegmentIndex, SegmentTemplate, CreationFlags);
		if(SplineMesh == nullptr)
		{
			return false;
		}
	}
	return true;
}
bool USplineTrackGeneratorLib::CreateUniformSplineTrack_Validate
(
	USplineComponent* Spline,
	const FSplineTrackSegment& SegmentTemplate,
	EMyObjectCreationFlags CreationFlags
)
{
	return (Spline != nullptr);
}

int32 USplineTrackGeneratorLib::GetNumberOfSplineTrackSegments(USplineComponent* Spline)
{
	checkf(Spline, TEXT("When calling \"%s\" passed spline component pointer must be valid NON-null pointer"), TEXT(__FUNCTION__));
	return Spline->IsClosedLoop() ? (Spline->GetNumberOfSplinePoints()) : (Spline->GetNumberOfSplinePoints() - 1);
}

bool USplineTrackGeneratorLib::GetNumberOfSplineTrackSegments_Validate(USplineComponent* Spline)
{
	return Spline != nullptr;
}
