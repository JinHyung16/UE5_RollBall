#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RollBall/Data/RollBallStageRow.h"
#include "RollBall/Skill/RollBallSkillTypes.h"
#include "RollBall/Stage/RollBallStageTypes.h"
#include "RollBallGameInstance.generated.h"

class UDataTable;
class URollBallSaveGame;
class URollBallSkillGraph;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRollBallGoldChanged, int32, NewGold);

UCLASS()
class ROLLBALL_API URollBallGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Stages")
	UDataTable* StageTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Stages")
	TArray<FName> StageOrder;

	UPROPERTY(BlueprintReadOnly, Category = "RollBall|Stages")
	int32 CurrentStageIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "RollBall|Player")
	FString PlayerNickname;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	bool GetCurrentStage(FRollBallStageRow& OutRow) const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	bool HasNextStage() const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	void StartFromFirstStage();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	void AdvanceToNextStage();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	bool OpenCurrentStageLevel();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Stages")
	FRollBallStageRules StageRules;

	UFUNCTION(BlueprintPure, Category = "RollBall|Stages")
	int32 GetCurrentStageNumber() const { return CurrentStageIndex + 1; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Stages")
	FRollBallStageSetup GetStageSetup(int32 StageNumber) const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Stages")
	FRollBallStageSetup GetCurrentStageSetup() const { return GetStageSetup(GetCurrentStageNumber()); }

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stages")
	bool TravelToStage(int32 StageNumber);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Skill")
	TObjectPtr<UDataTable> SkillDefinitionTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Skill")
	TObjectPtr<UDataTable> SkillPlacementTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Skill")
	TObjectPtr<UDataTable> SkillEdgeTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall|Skill")
	FString SaveSlotName = TEXT("RollBallAccount");

	UPROPERTY(BlueprintAssignable, Category = "RollBall|Skill")
	FRollBallGoldChanged OnGoldChanged;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	URollBallSkillGraph* EnsureSkillGraph();

	UFUNCTION(BlueprintPure, Category = "RollBall|Account")
	int32 GetGold() const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Account")
	int32 GetBestStage() const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Account")
	int32 GetRebirthCount() const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Account")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Account")
	int32 AddEarnedGold(int32 BaseAmount);

	UFUNCTION(BlueprintPure, Category = "RollBall|Account")
	float GetGoldMultiplier() const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Account")
	void ReportStageReached(int32 StageNumber);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Account")
	int32 Rebirth();

	UFUNCTION(BlueprintPure, Category = "RollBall|Account")
	int32 GetRebirthReward() const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Account")
	void SaveAccount();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Account")
	void LoadAccount();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Account")
	void DeleteAccount();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	bool BuySkillNode(int32 NodeIndex, FText& OutReason);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	bool BuySkillPath(const TArray<int32>& Path, FText& OutReason);

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	FRollBallSkillStats GetSkillStats() const { return SkillStats; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	float GetSkillStat(ERollBallSkillStat Stat) const { return SkillStats.Get(Stat); }

private:
	UPROPERTY()
	TObjectPtr<URollBallSkillGraph> SkillGraph = nullptr;

	UPROPERTY()
	TObjectPtr<URollBallSaveGame> SaveData = nullptr;

	UPROPERTY()
	FRollBallSkillStats SkillStats;

	UPROPERTY(EditAnywhere, Category = "RollBall|Account", meta = (ClampMin = "0"))
	int32 RebirthGoldPerStage = 800;

	UPROPERTY(EditAnywhere, Category = "RollBall|Account", meta = (ClampMin = "0"))
	float RebirthGoldBonusPerCount = 0.1f;

	URollBallSaveGame* EnsureSaveData();

	void LoadSkillTablesFromCsv();

	bool LevelExists(FName LevelName) const;

	void ApplySaveToGraph();

	UFUNCTION()
	void HandleSkillGraphChanged();

	FName ResolveRowName(int32 Index) const;
};
