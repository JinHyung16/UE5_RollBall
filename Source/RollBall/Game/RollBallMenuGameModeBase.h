// GameMode used by the main menu level. Spawns the menu widget on BeginPlay
// and exposes entry points the menu's Start button calls.

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

	// Called by the Start button on the menu widget.
	UFUNCTION(BlueprintCallable, Category = "RollBall|Menu")
	void StartGame(const FString& Nickname);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "RollBall|Widgets")
	TSubclassOf<class UUserWidget> MenuWidgetClass;

	UPROPERTY()
	URollBallMenuWidget* MenuWidget = nullptr;
};
