// Fill out your copyright notice in the Description page of Project Settings.


#include "RollBallGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "RollBall/Items/RollBallItemBase.h"
#include "Blueprint/UserWidget.h"
#include "RollBallWidget.h"

void ARollBallGameModeBase::BeginPlay()
{
	TArray<AActor*> Items;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARollBallItemBase::StaticClass(), Items);

	ItemsInLevel = Items.Num();

	if (GameWidgetClass != nullptr)
	{
		GameWidget = Cast<URollBallWidget>(CreateWidget(GetWorld(), GameWidgetClass));
		if (GameWidget != nullptr)
		{
			GameWidget->AddToViewport();
			UpdateItemText();
		}
	}
}

void ARollBallGameModeBase::UpdateItemText()
{
	GameWidget->SetItemText(ItemsCollected, ItemsInLevel);
}

void ARollBallGameModeBase::ItemCollected()
{
	ItemsCollected++;
	UpdateItemText();
}
