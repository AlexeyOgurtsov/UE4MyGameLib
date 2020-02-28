#include "SplineMoveDemoPawn.h"
#include "SplineMoveDemoPC.h"
#include "SplineMoveDemoConfig.h"

#include "Util/Core/LogUtilLib.h"

#include "Movement/Spline/SplinePawnMovement.h"
#include "GameFramework/FloatingPawnMovement.h"

/**
* TODO Fix: Movement
* 1. Floating pawn movement: does it work by default (seems like not)?
*/

namespace
{
	namespace Config
	{
		constexpr float MAX_SPEED = 300.0F;
		constexpr float ACCELERATION = 100.0F;
		constexpr float DECELERATION = 100.0F;
	} // Config
} // anonymous

MyPawnType::ASplineMoveDemoPawn()
{
	InitMovement();
}
void MyPawnType::MyBeginPlay_Implementation()
{
	M_LOGFUNC();

	Super::MyBeginPlay_Implementation();
	// Custom actions here
}

void MyPawnType::InitMovement()
{
	M_LOGFUNC();
	//InitFloatingMovement();
	InitSplineMovement();
}

void MyPawnType::InitSplineMovement()
{
	M_LOGFUNC();
	USplinePawnMovement* const MyMovement = CreateDefaultSubobject<USplinePawnMovement>(TEXT("SplineMovement"));
	MyMovement->bEditableWhenInherited = true;
	Movement = MyMovement;
}

void MyPawnType::InitFloatingMovement()
{
	M_LOGFUNC();
	UFloatingPawnMovement* const MyMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
	MyMovement->Acceleration = Config::ACCELERATION;
	MyMovement->Deceleration = Config::DECELERATION;
	MyMovement->MaxSpeed = Config::MAX_SPEED;
	MyMovement->bEditableWhenInherited = true;
	Movement = MyMovement;
}

MyPCType* MyPawnType::GetMyPC() const
{
	return Cast<MyPCType>(GetController());
}

MyPCType* MyPawnType::GetMyPCLogged(ELogFlags InLogFlags) const
{
	MyPCType* const PC = GetMyPC();
	if(PC == nullptr)
	{
		M_LOG_ERROR_IF_FLAGS(InLogFlags, TEXT("GetMyPC() returned NULL"));
	}
	return PC;
}

MyPCType* MyPawnType::GetMyPCChecked() const
{
	MyPCType* const PC = GetMyPC();
	checkf(PC, TEXT("GetMyPawn must return non-NULL pawn!"));
	return PC;
}
class USplinePawnMovement* MyPawnType::GetSplineMovement() const
{
	return Cast<USplinePawnMovement>(Movement);
}
class USplinePawnMovement* MyPawnType::GetSplineMovementChecked() const
{
	return CastChecked<USplinePawnMovement>(Movement);
}
