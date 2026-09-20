#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RollBall/Data/RollBallStageRow.h"
#include "RollBall/Enemy/RollBallEnemy.h"
#include "RollBall/Stage/RollBallStageTypes.h"
#include "RollBallGameModeBase.generated.h"

class URollBallWidget;
class ARollBallPlayer;

UENUM(BlueprintType)
enum class ERollBallStageResult : uint8
{
	None        UMETA(DisplayName = "None"),
	Cleared     UMETA(DisplayName = "Cleared"),
	Failed      UMETA(DisplayName = "Failed"),
};

UCLASS()
class ROLLBALL_API ARollBallGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARollBallGameModeBase();

	UFUNCTION(BlueprintCallable, Category = "RollBall")
	void ItemCollected();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Result")
	void RequestNextStage();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Result")
	void RequestRetryStage();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Result")
	void RequestExitToMenu();

	UFUNCTION(BlueprintPure, Category = "RollBall|Stage")
	ERollBallStagePhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Stage")
	float GetRemainingTime() const { return RemainingTime; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Stage")
	int32 GetGoldEarned() const { return GoldEarned; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Stage")
	int32 GetKillCount() const { return KillCount; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "RollBall|Spawning")
	TSubclassOf<ARollBallEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, Category = "RollBall|Spawning", meta = (ClampMin = "100"))
	float SpawnRadius = 2600.0f;

	UPROPERTY(EditAnywhere, Category = "RollBall|Spawning", meta = (ClampMin = "1"))
	int32 MaxAliveEnemies = 70;

	UPROPERTY(EditAnywhere, Category = "RollBall|Spawning", meta = (ClampMin = "100"))
	float CullDistance = 7000.0f;

	UPROPERTY(EditAnywhere, Category = "RollBall|Widgets")
	TSubclassOf<class UUserWidget> HudWidgetClass;

	UPROPERTY(EditAnywhere, Category = "RollBall|Spawning")
	bool bAutoBuildArena = true;

	UPROPERTY(EditAnywhere, Category = "RollBall|Flow")
	FName MainMenuLevelName = TEXT("L_MainMenu");

	UPROPERTY(EditAnywhere, Category = "RollBall|Flow", meta = (ClampMin = "0"))
	float ResultAutoAdvanceSeconds = 4.0f;

	UPROPERTY(EditAnywhere, Category = "RollBall|Flow")
	bool bAdvanceOnFailure = false;

	UPROPERTY()
	URollBallWidget* GameWidget = nullptr;

	UPROPERTY()
	FRollBallStageSetup Setup;

	UPROPERTY()
	ERollBallStagePhase Phase = ERollBallStagePhase::Normal;

	UPROPERTY()
	float RemainingTime = 0.0f;

	UPROPERTY()
	int32 GoldEarned = 0;

	UPROPERTY()
	int32 KillCount = 0;

	UPROPERTY()
	int32 ClockTicks = 0;

	UPROPERTY()
	bool bStageFinished = false;

	UPROPERTY()
	ERollBallStageResult StageResult = ERollBallStageResult::None;

	UPROPERTY()
	TArray<ARollBallEnemy*> AliveEnemies;

	UPROPERTY()
	class ARollBallArena* Arena = nullptr;

	FTimerHandle ClockHandle;
	FTimerHandle SpawnHandle;
	FTimerHandle AdvanceHandle;

	void TickClock();
	void EnterPhase(ERollBallStagePhase NewPhase);

	void SpawnWave();
	ARollBallEnemy* SpawnEnemy(ERollBallEnemyKind Kind, ERollBallSpawnRank Rank);
	ERollBallEnemyKind PickKindForPhase() const;

	float ScaleForRank(ERollBallSpawnRank Rank) const;

	bool FindSpawnLocation(ERollBallEnemyKind Kind, const ARollBallPlayer* Player, FVector& OutLocation) const;

	void CullDistantEnemies();

	void BuildArenaIfMissing();

	void FinishStage(ERollBallStageResult Result);
	void UpdateHud();

	ARollBallPlayer* GetBallPlayer() const;

	UFUNCTION()
	void HandleEnemyDied(ARollBallEnemy* Enemy, int32 GoldReward);

	UFUNCTION()
	void HandlePlayerDied();

	UFUNCTION()
	void HandleAutoAdvance();
};
