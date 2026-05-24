// Menu widget. The Blueprint subclass builds the visual layout (Roll Ball
// title, nickname text box, Start button) and calls StartGame on click.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RollBallMenuWidget.generated.h"

UCLASS()
class ROLLBALL_API URollBallMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Convenience entry point for the Start button's OnClicked binding in the
	// Blueprint subclass — it forwards the typed nickname to the GameMode.
	UFUNCTION(BlueprintCallable, Category = "RollBall|Menu")
	void RequestStartGame(const FString& Nickname);
};
