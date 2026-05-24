#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RollBallGameModeBase.h"
#include "RollBallWidget.generated.h"

// HUD + Result widget. The Blueprint subclass implements all three events
// (no extra rooms; same widget owns the in-game HUD and the result overlay,
// which keeps button wiring simple — the result panel just toggles on top).
UCLASS()
class ROLLBALL_API URollBallWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Update the "X / Y" item counter.
	UFUNCTION(BlueprintImplementableEvent, Category = "RollBall|HUD")
	void SetItemText(int32 ItemsCollected, int32 ItemsInLevel);

	// Update the countdown timer (seconds remaining; <=0 means time's up).
	UFUNCTION(BlueprintImplementableEvent, Category = "RollBall|HUD")
	void SetTimeText(float RemainingSeconds);

	// Show the Win or Lose panel with its buttons.
	UFUNCTION(BlueprintImplementableEvent, Category = "RollBall|Result")
	void ShowResult(ERollBallStageResult Result);
};
