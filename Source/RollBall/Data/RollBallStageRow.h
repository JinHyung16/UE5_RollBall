#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RollBallStageRow.generated.h"

USTRUCT(BlueprintType)
struct FRollBallStageRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	FName LevelName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage", meta = (ClampMin = "1"))
	int32 RequiredItemCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage", meta = (ClampMin = "0"))
	float TimeLimitSeconds = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	FText DisplayName;
};
