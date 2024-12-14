// Fill out your copyright notice in the Description page of Project Settings.


#include "RollBallItemBase.h"
#include "RollBall/Game/RollBallPlayer.h"
#include "RollBall/Game/RollBallGameModeBase.h"

// Sets default values
ARollBallItemBase::ARollBallItemBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	RootComponent = Mesh;

	Mesh->OnComponentBeginOverlap.AddDynamic(this, &ARollBallItemBase::OverlapBegin);
}

// Called when the game starts or when spawned
void ARollBallItemBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
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

