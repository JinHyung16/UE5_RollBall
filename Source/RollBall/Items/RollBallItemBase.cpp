#include "RollBallItemBase.h"
#include "RollBall/Game/RollBallPlayer.h"
#include "RollBall/Game/RollBallGameModeBase.h"

ARollBallItemBase::ARollBallItemBase()
{

	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	RootComponent = Mesh;

	Mesh->OnComponentBeginOverlap.AddDynamic(this, &ARollBallItemBase::OverlapBegin);
}

void ARollBallItemBase::BeginPlay()
{
	Super::BeginPlay();

}

void ARollBallItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARollBallItemBase::OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ARollBallPlayer>(OtherActor) != nullptr)
	{
		Collected();
	}
}

void ARollBallItemBase::Collected_Implementation()
{
	ARollBallGameModeBase* GameMode = Cast<ARollBallGameModeBase>(GetWorld()->GetAuthGameMode());
	if (GameMode != nullptr)
	{
		GameMode->ItemCollected();
	}
}
