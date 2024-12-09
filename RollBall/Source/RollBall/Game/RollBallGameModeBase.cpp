// Fill out your copyright notice in the Description page of Project Settings.


#include "RollBallGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "RollBall/Items/RollBallItemBase.h"

void ARollBallGameModeBase::BeginPlay()
{
	TArray<AActor*> Items;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARollBallItemBase::StaticClass(), Items);

	ItemsInLevel = Items.Num();
}

void ARollBallGameModeBase::UpdateItemText()
{
}

void ARollBallGameModeBase::ItemCollected()
{
	ItemsCollected++;
}
