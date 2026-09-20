#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RollBallMenuGameModeBase.generated.h"

class URollBallMenuWidget;

UCLASS()
class ROLLBALL_API ARollBallMenuGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARollBallMenuGameModeBase();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Menu")
	void StartGame(const FString& Nickname);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "RollBall|Widgets")
	TSubclassOf<class UUserWidget> MenuWidgetClass;

	UPROPERTY()
	URollBallMenuWidget* MenuWidget = nullptr;
};
