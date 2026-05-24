#include "RollBallMenuWidget.h"

#include "Kismet/GameplayStatics.h"
#include "RollBallMenuGameModeBase.h"

void URollBallMenuWidget::RequestStartGame(const FString& Nickname)
{
	if (ARollBallMenuGameModeBase* GM = Cast<ARollBallMenuGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GM->StartGame(Nickname);
	}
}
