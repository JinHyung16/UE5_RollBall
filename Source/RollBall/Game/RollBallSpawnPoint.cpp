#include "RollBallSpawnPoint.h"
#include "Components/BillboardComponent.h"

ARollBallSpawnPoint::ARollBallSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	RootComponent = Billboard;
}
