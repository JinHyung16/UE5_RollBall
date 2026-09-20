#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RollBallSaveGame.generated.h"

UCLASS()
class ROLLBALL_API URollBallSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Save")
	TMap<int32, int32> SkillLevelsByPlacementId;

	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Save")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Save")
	int32 BestStage = 0;

	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Save")
	int32 CurrentRunBestStage = 0;

	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Save")
	int32 RebirthCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Save")
	FString PlayerNickname;

	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Save")
	int32 SaveVersion = 1;
};
