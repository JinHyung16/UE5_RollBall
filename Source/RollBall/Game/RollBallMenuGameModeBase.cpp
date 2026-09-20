#include "RollBallMenuGameModeBase.h"

#include "Blueprint/UserWidget.h"
#include "RollBallGameInstance.h"
#include "RollBallMenuWidget.h"

ARollBallMenuGameModeBase::ARollBallMenuGameModeBase()
{
	PrimaryActorTick.bCanEverTick = false;

	MenuWidgetClass = URollBallMenuWidget::StaticClass();
}

void ARollBallMenuGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (MenuWidgetClass != nullptr)
	{
		MenuWidget = Cast<URollBallMenuWidget>(CreateWidget(GetWorld(), MenuWidgetClass));
		if (MenuWidget != nullptr)
		{
			MenuWidget->AddToViewport();
		}
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
}

void ARollBallMenuGameModeBase::StartGame(const FString& Nickname)
{
	URollBallGameInstance* GI = GetGameInstance<URollBallGameInstance>();
	if (GI == nullptr)
	{
		return;
	}

	GI->PlayerNickname = Nickname;
	GI->TravelToStage(1);
}
