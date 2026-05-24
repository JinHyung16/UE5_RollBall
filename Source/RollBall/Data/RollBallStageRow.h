// Stage data row used by UDataTable. Author rows in CSV/JSON and import into a
// DataTable asset (Content Browser > Miscellaneous > Data Table > pick this struct).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RollBallStageRow.generated.h"

USTRUCT(BlueprintType)
struct FRollBallStageRow : public FTableRowBase
{
	GENERATED_BODY()

	// Map (level) name to open for this stage, e.g. "L_Stage_01".
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	FName LevelName = NAME_None;

	// Number of items the player must collect to clear this stage.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage", meta = (ClampMin = "1"))
	int32 RequiredItemCount = 5;

	// Time limit in seconds. 0 or negative means no limit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage", meta = (ClampMin = "0"))
	float TimeLimitSeconds = 30.0f;

	// Optional human-readable label, shown in HUD/menus if desired.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	FText DisplayName;
};
