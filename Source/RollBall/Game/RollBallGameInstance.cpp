#include "RollBallGameInstance.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

#include "RollBall/Skill/RollBallSaveGame.h"
#include "RollBall/Skill/RollBallSkillGraph.h"

DEFINE_LOG_CATEGORY_STATIC(LogRollBallAccount, Log, All);

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

FRollBallStageSetup URollBallGameInstance::GetStageSetup(int32 StageNumber) const
{
	FRollBallStageSetup Setup;

	Setup.StageNumber = FMath::Clamp(StageNumber, 1, FMath::Max(1, StageRules.MaxStage));

	const int32 Steps = Setup.StageNumber - 1;
	const float StepsFloat = static_cast<float>(Steps);

	Setup.SurviveSeconds = StageRules.BaseSurviveSeconds + StageRules.SurviveSecondsPerStage * StepsFloat;
	Setup.EliteSeconds = FMath::Min(StageRules.EliteSeconds, Setup.SurviveSeconds * 0.5f);
	Setup.bBossStage = (StageRules.BossEveryStages > 0)
		&& (Setup.StageNumber % StageRules.BossEveryStages == 0);

	Setup.EnemyHealthScale = FMath::Pow(StageRules.EnemyHealthGrowth, StepsFloat);
	Setup.EnemySpeedScale = FMath::Pow(StageRules.EnemySpeedGrowth, StepsFloat);

	Setup.SpawnInterval = FMath::Max(
		StageRules.MinSpawnInterval,
		StageRules.BaseSpawnInterval / FMath::Pow(StageRules.SpawnRateGrowth, StepsFloat));

	Setup.GoldPerKill = FMath::Max(1,
		FMath::RoundToInt(StageRules.BaseGoldPerKill * FMath::Pow(StageRules.GoldGrowth, StepsFloat)));

	Setup.SurvivalGoldPerMinute = FMath::Max(0,
		FMath::RoundToInt(StageRules.SurvivalGoldPerMinute * FMath::Pow(StageRules.GoldGrowth, StepsFloat)));

	if (StageTable != nullptr)
	{
		const TArray<FName> RowNames = StageTable->GetRowNames();
		if (RowNames.Num() > 0)
		{
			const FName RowName = RowNames[Steps % RowNames.Num()];
			if (const FRollBallStageRow* Row =
				StageTable->FindRow<FRollBallStageRow>(RowName, TEXT("RollBallGameInstance"), false))
			{

				if (LevelExists(Row->LevelName))
				{
					Setup.LevelName = Row->LevelName;
				}
				else
				{
					UE_LOG(LogRollBallAccount, Warning,
						TEXT("스테이지 표의 맵 '%s' 이 없습니다. 지금 맵을 그대로 씁니다."),
						*Row->LevelName.ToString());
				}
			}
		}
	}

	return Setup;
}

bool URollBallGameInstance::LevelExists(FName LevelName) const
{
	if (LevelName.IsNone())
	{
		return false;
	}

	FString PackageName = LevelName.ToString();

	if (!PackageName.StartsWith(TEXT("/")))
	{
		PackageName = TEXT("/Game/") + PackageName;
	}

	return FPackageName::DoesPackageExist(PackageName);
}

bool URollBallGameInstance::TravelToStage(int32 StageNumber)
{
	const int32 Clamped = FMath::Clamp(StageNumber, 1, FMath::Max(1, StageRules.MaxStage));
	CurrentStageIndex = Clamped - 1;

	const FRollBallStageSetup Setup = GetStageSetup(Clamped);

	static const FString PlayMode(TEXT("game=/Script/RollBall.RollBallGameModeBase"));

	if (Setup.LevelName.IsNone())
	{

		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)),
			true, PlayMode);
		return false;
	}

	UGameplayStatics::OpenLevel(this, Setup.LevelName, true, PlayMode);
	return true;
}

URollBallSkillGraph* URollBallGameInstance::EnsureSkillGraph()
{
	if (SkillGraph != nullptr)
	{
		return SkillGraph;
	}

	SkillGraph = NewObject<URollBallSkillGraph>(this, TEXT("SkillGraph"));
	SkillGraph->OnGraphChanged.AddDynamic(this, &URollBallGameInstance::HandleSkillGraphChanged);

	if (SkillDefinitionTable == nullptr && SkillPlacementTable == nullptr && SkillEdgeTable == nullptr)
	{
		LoadSkillTablesFromCsv();
	}

	if (!SkillGraph->BuildFromTables(SkillDefinitionTable, SkillPlacementTable, SkillEdgeTable))
	{
		UE_LOG(LogRollBallAccount, Error,
			TEXT("스킬 그래프를 만들지 못했습니다. 오류 %d 건. GameInstance 블루프린트에서 테이블 3개를 확인하세요."),
			SkillGraph->GetBuildErrors().Num());
	}

	EnsureSaveData();
	ApplySaveToGraph();

	return SkillGraph;
}

void URollBallGameInstance::LoadSkillTablesFromCsv()
{
#if WITH_EDITOR
	struct FTableToLoad
	{
		const TCHAR* FileName;
		UScriptStruct* RowStruct;
		TObjectPtr<UDataTable>* Target;
	};

	const FTableToLoad ToLoad[] = {
		{ TEXT("DT_SkillDef.csv"),       FRollBallSkillDefRow::StaticStruct(),       &SkillDefinitionTable },
		{ TEXT("DT_SkillPlacement.csv"), FRollBallSkillPlacementRow::StaticStruct(), &SkillPlacementTable },
		{ TEXT("DT_SkillEdge.csv"),      FRollBallSkillEdgeRow::StaticStruct(),      &SkillEdgeTable },
	};

	for (const FTableToLoad& Entry : ToLoad)
	{
		const FString Path = FPaths::ProjectContentDir() / TEXT("Data") / Entry.FileName;

		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			UE_LOG(LogRollBallAccount, Warning, TEXT("스킬 CSV 를 못 읽었습니다: %s"), *Path);
			continue;
		}

		UDataTable* Table = NewObject<UDataTable>(this);
		Table->RowStruct = Entry.RowStruct;

		const TArray<FString> Problems = Table->CreateTableFromCSVString(Text);
		for (const FString& Problem : Problems)
		{
			UE_LOG(LogRollBallAccount, Warning, TEXT("%s: %s"), Entry.FileName, *Problem);
		}

		*Entry.Target = Table;
	}

	UE_LOG(LogRollBallAccount, Warning,
		TEXT("스킬 표가 지정되지 않아 CSV 를 직접 읽었습니다. 에디터에서만 됩니다. "
			 "빌드한 게임에서는 DataTable 로 가져와 GameInstance 에 넣어야 합니다."));
#endif
}

void URollBallGameInstance::ApplySaveToGraph()
{
	if (SkillGraph == nullptr || SaveData == nullptr)
	{
		return;
	}

	SkillGraph->SetLevelsByPlacementId(SaveData->SkillLevelsByPlacementId);
}

void URollBallGameInstance::HandleSkillGraphChanged()
{
	if (SkillGraph == nullptr)
	{
		return;
	}

	SkillStats = SkillGraph->Recalculate();

	if (SaveData != nullptr)
	{
		SaveData->SkillLevelsByPlacementId = SkillGraph->GetLevelsByPlacementId();
	}
}

bool URollBallGameInstance::BuySkillNode(int32 NodeIndex, FText& OutReason)
{
	if (SkillGraph == nullptr || EnsureSaveData() == nullptr)
	{
		return false;
	}

	int32 Gold = SaveData->Gold;
	if (!SkillGraph->TryBuy(NodeIndex, Gold, SaveData->BestStage, OutReason))
	{
		return false;
	}

	SaveData->Gold = Gold;
	OnGoldChanged.Broadcast(Gold);
	SaveAccount();
	return true;
}

bool URollBallGameInstance::BuySkillPath(const TArray<int32>& Path, FText& OutReason)
{
	if (SkillGraph == nullptr || EnsureSaveData() == nullptr)
	{
		return false;
	}

	int32 Gold = SaveData->Gold;
	if (!SkillGraph->TryBuyPath(Path, Gold, SaveData->BestStage, OutReason))
	{
		return false;
	}

	SaveData->Gold = Gold;
	OnGoldChanged.Broadcast(Gold);
	SaveAccount();
	return true;
}

URollBallSaveGame* URollBallGameInstance::EnsureSaveData()
{
	if (SaveData == nullptr)
	{
		LoadAccount();
	}
	return SaveData;
}

int32 URollBallGameInstance::GetGold() const
{
	return SaveData != nullptr ? SaveData->Gold : 0;
}

int32 URollBallGameInstance::GetBestStage() const
{
	return SaveData != nullptr ? SaveData->BestStage : 0;
}

int32 URollBallGameInstance::GetRebirthCount() const
{
	return SaveData != nullptr ? SaveData->RebirthCount : 0;
}

void URollBallGameInstance::AddGold(int32 Amount)
{
	if (EnsureSaveData() == nullptr)
	{
		return;
	}

	SaveData->Gold = FMath::Max(0, SaveData->Gold + Amount);
	OnGoldChanged.Broadcast(SaveData->Gold);
}

float URollBallGameInstance::GetGoldMultiplier() const
{

	const float FromSkills = SkillStats.Get(ERollBallSkillStat::GoldGain);
	const float FromRebirths = 1.0f + GetRebirthCount() * RebirthGoldBonusPerCount;

	return FromSkills * FromRebirths;
}

int32 URollBallGameInstance::AddEarnedGold(int32 BaseAmount)
{
	const int32 Earned = FMath::RoundToInt(BaseAmount * GetGoldMultiplier());

	AddGold(Earned);
	return Earned;
}

void URollBallGameInstance::ReportStageReached(int32 StageNumber)
{
	if (EnsureSaveData() == nullptr)
	{
		return;
	}

	SaveData->CurrentRunBestStage = FMath::Max(SaveData->CurrentRunBestStage, StageNumber);
	SaveData->BestStage = FMath::Max(SaveData->BestStage, StageNumber);
	SaveAccount();
}

int32 URollBallGameInstance::GetRebirthReward() const
{
	if (SaveData == nullptr)
	{
		return 0;
	}

	const float Multiplier = SkillStats.Get(ERollBallSkillStat::RebirthMultiplier);
	return FMath::RoundToInt(SaveData->BestStage * RebirthGoldPerStage * Multiplier);
}

int32 URollBallGameInstance::Rebirth()
{
	if (EnsureSaveData() == nullptr || SkillGraph == nullptr)
	{
		return 0;
	}

	const int32 Reward = GetRebirthReward();

	SkillGraph->ResetAllLevels();
	SaveData->RebirthCount += 1;
	SaveData->CurrentRunBestStage = 0;
	SaveData->Gold += Reward;

	CurrentStageIndex = 0;

	OnGoldChanged.Broadcast(SaveData->Gold);
	SaveAccount();
	return Reward;
}

void URollBallGameInstance::SaveAccount()
{
	if (SaveData == nullptr)
	{
		return;
	}

	if (SkillGraph != nullptr)
	{
		SaveData->SkillLevelsByPlacementId = SkillGraph->GetLevelsByPlacementId();
	}
	SaveData->PlayerNickname = PlayerNickname;

	UGameplayStatics::SaveGameToSlot(SaveData, SaveSlotName, 0);
}

void URollBallGameInstance::LoadAccount()
{

	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		SaveData = Cast<URollBallSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
	}

	if (SaveData == nullptr)
	{
		SaveData = Cast<URollBallSaveGame>(
			UGameplayStatics::CreateSaveGameObject(URollBallSaveGame::StaticClass()));
	}

	PlayerNickname = SaveData->PlayerNickname;

	ApplySaveToGraph();
	OnGoldChanged.Broadcast(SaveData->Gold);
}

void URollBallGameInstance::DeleteAccount()
{
	UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);

	SaveData = Cast<URollBallSaveGame>(
		UGameplayStatics::CreateSaveGameObject(URollBallSaveGame::StaticClass()));

	PlayerNickname.Empty();
	CurrentStageIndex = 0;

	if (SkillGraph != nullptr)
	{
		SkillGraph->ResetAllLevels();
	}

	OnGoldChanged.Broadcast(SaveData->Gold);
}
