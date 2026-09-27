#include "RollBallMenuGameModeBase.h"

#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "RollBallGameInstance.h"
#include "RollBallMenuWidget.h"

ARollBallMenuGameModeBase::ARollBallMenuGameModeBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 메뉴 배치는 위젯 블루프린트에 있다.
	static ConstructorHelpers::FClassFinder<UUserWidget> MenuWidgetFinder(TEXT("/Game/UI/WBP_MainMenu"));
	MenuWidgetClass = MenuWidgetFinder.Succeeded() ? MenuWidgetFinder.Class : TSubclassOf<UUserWidget>(URollBallMenuWidget::StaticClass());
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

void ARollBallMenuGameModeBase::StartGame()
{
	if (URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>())
	{
		GameInstance->TravelToStage(1);
	}
}
