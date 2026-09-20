#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RollBallSkillTypes.h"
#include "RollBallSkillGraph.generated.h"

class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRollBallSkillGraphChanged);

UCLASS(BlueprintType)
class ROLLBALL_API URollBallSkillGraph : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category = "RollBall|Skill")
	FRollBallSkillGraphChanged OnGraphChanged;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	bool BuildFromTables(UDataTable* DefinitionTable, UDataTable* PlacementTable, UDataTable* EdgeTable);

	const TArray<FString>& GetBuildErrors() const { return BuildErrors; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	bool IsBuilt() const { return Nodes.Num() > 0; }

	const TArray<FRollBallSkillNode>& GetNodes() const { return Nodes; }
	const TArray<FRollBallSkillEdge>& GetEdges() const { return Edges; }

	bool IsValidNodeIndex(int32 NodeIndex) const { return Nodes.IsValidIndex(NodeIndex); }
	const FRollBallSkillNode& GetNode(int32 NodeIndex) const { return Nodes[NodeIndex]; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	int32 FindNodeIndexByPlacementId(int32 PlacementId) const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	int32 GetCoreNodeIndex() const { return CoreNodeIndex; }

	void GetCellBounds(FVector2D& OutMin, FVector2D& OutMax) const;

	const TMap<int32, int32>& GetLevelsByPlacementId() const { return LevelsByPlacementId; }

	void SetLevelsByPlacementId(const TMap<int32, int32>& InLevels);

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	int32 GetLevel(int32 NodeIndex) const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	bool IsOwned(int32 NodeIndex) const { return GetLevel(NodeIndex) > 0; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	bool IsMaxed(int32 NodeIndex) const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	int32 GetNextCost(int32 NodeIndex) const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	int32 GetSpentGold(int32 NodeIndex) const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	bool IsGateOpen(int32 NodeIndex, int32 BestStage) const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	bool HasOwnedNeighbour(int32 NodeIndex) const;

	bool CanBuy(int32 NodeIndex, int32 Gold, int32 BestStage, FText& OutReason) const;

	bool TryBuy(int32 NodeIndex, int32& InOutGold, int32 BestStage, FText& OutReason);

	bool TryBuyPath(const TArray<int32>& Path, int32& InOutGold, int32 BestStage, FText& OutReason);

	bool FindPathTo(int32 TargetIndex, int32 BestStage, TArray<int32>& OutPath) const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	int32 GetPathCost(const TArray<int32>& Path) const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	FRollBallSkillStats Recalculate() const;

	UFUNCTION(BlueprintPure, Category = "RollBall|Skill")
	int32 GetTotalSpentGold() const;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	void ResetAllLevels();

private:
	UPROPERTY()
	TArray<FRollBallSkillNode> Nodes;

	UPROPERTY()
	TArray<FRollBallSkillEdge> Edges;

	UPROPERTY()
	TMap<int32, int32> LevelsByPlacementId;

	TMap<int32, int32> NodeIndexByPlacementId;

	TArray<FString> BuildErrors;

	int32 CoreNodeIndex = INDEX_NONE;

	int32 CostAtLevel(const FRollBallSkillNode& Node, int32 Level) const;

	void EnsureCoreOwned();
};
