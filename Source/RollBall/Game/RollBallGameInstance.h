// GameInstance holds data that must survive level transitions: current stage
// index, the stage DataTable reference, and the player nickname.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RollBall/Data/RollBallStageRow.h"
#include "RollBallGameInstance.generated.h"

class UDataTable;

UCLASS()
class ROLLBALL_API URollBallGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// DataTable asset holding FRollBallStageRow rows. Assigned in the Blueprint
	// subclass (Project Settings > Maps & Modes > Game Instance Class).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Stages")
	UDataTable* StageTable = nullptr;

	// Ordered list of row names to play through. If empty we walk the table in
	// its natural row order.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Stages")
	TArray<FName> StageOrder;

	// Zero-based index into StageOrder (or table row order).
	UPROPERTY(BlueprintReadOnly, Category = "RollBall|Stages")
	int32 CurrentStageIndex = 0;

	// Player nickname captured on the main menu.
	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Player")
	FString PlayerNickname;

	// --- Stage navigation -------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	bool GetCurrentStage(FRollBallStageRow& OutRow) const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	bool HasNextStage() const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	void StartFromFirstStage();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	void AdvanceToNextStage();

	// Open the level for the current stage. Returns false if no current stage.
	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	bool OpenCurrentStageLevel();

private:
	// Returns the row name at StageOrder[Index], or the table's Nth row if
	// StageOrder is empty. NAME_None if out of range.
	FName ResolveRowName(int32 Index) const;
};
