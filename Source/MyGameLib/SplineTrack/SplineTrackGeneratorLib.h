#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SplineTrackTypes.h"
#include "SplineTrackGeneratorLib.generated.h"

class USplineComponent;
class USceneComponent;
class USplineMeshComponent;

UCLASS()
class USplineTrackGeneratorLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 
	* CreateAttachedSplineSegmentMesh
	*
	* @param Spline            Spline component to get spline data from
	* (must be valid NON-null pointer)
	* @param SegmentIndex      Index of start spline point 
	* of the segment in question, starting from ZERO.
	* It's possible to pass the last spline point index
	* if the spline is looped.
	* @param ParentComponent   Parent component to attach to
	* (must be valid NON-null pointer)
	* @param SubobjectName    Name of the spline mesh component 
	* subobject that is to be created
	* @param bDynamicObject Must be true if called NOT from constructor, false otherwise.
	* If NAME_None is passed, the name is to be generated automatically.
	*
	* @returns always returns valid pointer (checked with checkf)
	*/
	UFUNCTION(BlueprintCallable, Category=SplineTrackGeneratorLib, Meta=(UnsafeDuringActorConstruction="false"))
	static USplineMeshComponent* CreateAttachedSplineSegmentMesh
	(
		USplineComponent* Spline, int32 SegmentIndex,
		const FSplineTrackSegment& SegmentData,
	 	USceneComponent* ParentComponent,
		bool bDynamicObject,
		FName SubobjectName = NAME_None
	);
	
};
