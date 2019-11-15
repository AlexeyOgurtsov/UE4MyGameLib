#include "SplineMoveDemoGameMode.h"
#include "SplineMoveDemoPC.h"
#include "SplineMoveDemoPawn.h"
#include "SplineMoveDemoConfig.h"
#include "Util/Core/LogUtilLib.h"

MyGameModeType::ASplineMoveDemoGameMode()
{
	DefaultPawnClass = MyPawnType::StaticClass();
	PlayerControllerClass = MyPCType::StaticClass();
}
