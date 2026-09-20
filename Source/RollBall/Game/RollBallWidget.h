#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RollBallGameModeBase.h"
#include "RollBall/Stage/RollBallStageTypes.h"
#include "RollBallWidget.generated.h"

UCLASS()
class ROLLBALL_API URollBallWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RollBall|HUD")
	void SetTimeText(float RemainingSeconds);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RollBall|HUD")
	void SetStageText(int32 StageNumber, bool bBossStage);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RollBall|HUD")
	void SetHealth(int32 Health, int32 MaxHealth);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RollBall|HUD")
	void SetGoldText(int32 GoldEarned, int32 KillCount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RollBall|HUD")
	void SetPhase(ERollBallStagePhase Phase);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RollBall|Result")
	void SetResultSummary(ERollBallStageResult Result, int32 KillCount, int32 GoldEarned);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RollBall|Result")
	void ShowResult(ERollBallStageResult Result);
};
