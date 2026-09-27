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
};
