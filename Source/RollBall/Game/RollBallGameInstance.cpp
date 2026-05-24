#include "RollBallGameInstance.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"

FName URollBallGameInstance::ResolveRowName(int32 Index) const
{
	if (Index < 0)
	{
		return NAME_None;
	}

	if (StageOrder.Num() > 0)
	{
		return StageOrder.IsValidIndex(Index) ? StageOrder[Index] : NAME_None;
	}

	if (StageTable == nullptr)
	{
		return NAME_None;
	}

	const TArray<FName> RowNames = StageTable->GetRowNames();
	return RowNames.IsValidIndex(Index) ? RowNames[Index] : NAME_None;
}

bool URollBallGameInstance::GetCurrentStage(FRollBallStageRow& OutRow) const
{
	if (StageTable == nullptr)
	{
		return false;
	}

	const FName RowName = ResolveRowName(CurrentStageIndex);
	if (RowName.IsNone())
	{
		return false;
	}

	if (const FRollBallStageRow* Row = StageTable->FindRow<FRollBallStageRow>(RowName, TEXT("RollBallGameInstance")))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}

bool URollBallGameInstance::HasNextStage() const
{
	return !ResolveRowName(CurrentStageIndex + 1).IsNone();
}

void URollBallGameInstance::StartFromFirstStage()
{
	CurrentStageIndex = 0;
}

void URollBallGameInstance::AdvanceToNextStage()
{
	if (HasNextStage())
	{
		++CurrentStageIndex;
	}
}

bool URollBallGameInstance::OpenCurrentStageLevel()
{
	FRollBallStageRow Row;
	if (!GetCurrentStage(Row) || Row.LevelName.IsNone())
	{
		return false;
	}

	UGameplayStatics::OpenLevel(this, Row.LevelName);
	return true;
}
