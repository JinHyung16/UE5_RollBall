#pragma once

#include "CoreMinimal.h"
#include "RollBallStageTypes.generated.h"

UENUM(BlueprintType)
enum class ERollBallStagePhase : uint8
{
	Normal UMETA(DisplayName = "일반"),
	Elite  UMETA(DisplayName = "정예"),
	Boss   UMETA(DisplayName = "보스"),
};

UENUM(BlueprintType)
enum class ERollBallSpawnRank : uint8
{
	Normal UMETA(DisplayName = "일반"),
	Elite  UMETA(DisplayName = "정예"),
	Boss   UMETA(DisplayName = "보스"),
};

USTRUCT(BlueprintType)
struct ROLLBALL_API FRollBallStageRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "스테이지", meta = (ClampMin = "1"))
	int32 MaxStage = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "스테이지", meta = (ClampMin = "10"))
	float BaseSurviveSeconds = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "스테이지", meta = (ClampMin = "0"))
	float SurviveSecondsPerStage = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "스테이지", meta = (ClampMin = "0"))
	float EliteSeconds = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "스테이지", meta = (ClampMin = "1"))
	int32 BossEveryStages = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "난이도", meta = (ClampMin = "1.0"))
	float EnemyHealthGrowth = 1.038f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "난이도", meta = (ClampMin = "1.0"))
	float EnemySpeedGrowth = 1.008f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "난이도", meta = (ClampMin = "0.05"))
	float BaseSpawnInterval = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "난이도", meta = (ClampMin = "1.0"))
	float SpawnRateGrowth = 1.025f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "난이도", meta = (ClampMin = "0.05"))
	float MinSpawnInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "난이도", meta = (ClampMin = "1.0"))
	float EliteScale = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "난이도", meta = (ClampMin = "1.0"))
	float BossScale = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "보상", meta = (ClampMin = "0"))
	int32 BaseGoldPerKill = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "보상", meta = (ClampMin = "1.0"))
	float GoldGrowth = 1.03f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "보상", meta = (ClampMin = "0"))
	int32 SurvivalGoldPerMinute = 40;
};

USTRUCT(BlueprintType)
struct ROLLBALL_API FRollBallStageSetup
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "스테이지")
	int32 StageNumber = 1;

	UPROPERTY(BlueprintReadOnly, Category = "스테이지")
	FName LevelName;

	UPROPERTY(BlueprintReadOnly, Category = "스테이지")
	float SurviveSeconds = 720.0f;

	UPROPERTY(BlueprintReadOnly, Category = "스테이지")
	float EliteSeconds = 120.0f;

	UPROPERTY(BlueprintReadOnly, Category = "스테이지")
	bool bBossStage = false;

	UPROPERTY(BlueprintReadOnly, Category = "난이도")
	float EnemyHealthScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "난이도")
	float EnemySpeedScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "난이도")
	float SpawnInterval = 1.6f;

	UPROPERTY(BlueprintReadOnly, Category = "보상")
	int32 GoldPerKill = 5;

	UPROPERTY(BlueprintReadOnly, Category = "보상")
	int32 SurvivalGoldPerMinute = 40;
};
