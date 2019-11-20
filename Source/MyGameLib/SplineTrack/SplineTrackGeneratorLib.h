#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SplineTrackTypes.h"
#include "Util/Core/Object/MyObjectCreationTypes.h"
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
	* Like CreateUniformSplineTrack, but removes all spline mesh components before adding any new.
	*
	* @see: CreateUniformSplineTrack
	*/
	UFUNCTION(BlueprintCallable, Category=SplineTrackGeneratorLib, Meta=(WithValidation="true"))
	static bool ResetUniformSplineTrack
	(
		USplineComponent* Spline,
		const FSplineTrackSegment& SegmentTemplate,
		EMyObjectCreationFlags CreationFlags = EMyObjectCreationFlags::Dynamic
	);
	static bool ResetUniformSplineTrack_Validate
	(
		USplineComponent* Spline,
		const FSplineTrackSegment& SegmentTemplate,
		EMyObjectCreationFlags CreationFlags
	);

	/**
	* CreateUniformSplineTrack
	*
	* @return: true if the track was created without errors
	*/
	UFUNCTION(BlueprintCallable, Category=SplineTrackGeneratorLib, Meta=(WithValidation="true"))
	static bool CreateUniformSplineTrack
	(
		USplineComponent* Spline,
		const FSplineTrackSegment& SegmentTemplate,
		EMyObjectCreationFlags CreationFlags = EMyObjectCreationFlags::Dynamic
	);
	static bool CreateUniformSplineTrack_Validate
	(
		USplineComponent* Spline,
		const FSplineTrackSegment& SegmentTemplate,
		EMyObjectCreationFlags CreationFlags
	);

	/** 
	* CreateAttachedSplineSegmentMesh
	*
	* Spline mesh will component be attached automatically to the spline component.
	*
	* @param Spline            Spline component to get spline data from
	* (must be valid NON-null pointer)
	* @param SegmentIndex      Index of start spline point 
	* of the segment in question, starting from ZERO.
	* It's possible to pass the last spline point index
	* if the spline is looped.
	* @param ParentComponent: must be valid pointer!
	* @param SubobjectName    Name of the spline mesh component 
	* subobject that is to be created
	* If NAME_None is passed, then the name is to be generated automatically.
	* @param CreationFlags    Flags that determine how the object is to be created
	* Warning: if not within construction script or constructor is called,
	* Dynamic flag must be passed!
	*
	* @returns returns pointer to the created spline mesh, or nullptr, if there was an error
	* while attaching the spline segment mesh.
	*/
	UFUNCTION(BlueprintCallable, Category=SplineTrackGeneratorLib, Meta=(WithValidation="true"))
	static USplineMeshComponent* CreateAttachedSplineSegmentMesh
	(
		USplineComponent* Spline, int32 SegmentIndex,
		const FSplineTrackSegment& SegmentData,
		EMyObjectCreationFlags CreationFlags = EMyObjectCreationFlags::Dynamic,
		FName SubobjectName = NAME_None
	);
	static bool CreateAttachedSplineSegmentMesh_Validate
	(
		USplineComponent* Spline, int32 SegmentIndex,
		const FSplineTrackSegment& SegmentData,
		EMyObjectCreationFlags CreationFlags,
		FName SubobjectName
	);
	
	UFUNCTION(BlueprintCallable, Category=SplineTrackGeneratorLib, Meta=(WithValidation="true"))
	static void DestroyAllSplineMeshComponents(AActor* Actor);
	static bool DestroyAllSplineMeshComponents_Validate(AActor* Actor);

	UFUNCTION(BlueprintPure, Category=SplineTrackGeneratorLib, Meta=(WithValidation="true"))
	static int32 GetNumberOfSplineTrackSegments(USplineComponent* Spline);
	static bool GetNumberOfSplineTrackSegments_Validate(USplineComponent* Spline);
};
