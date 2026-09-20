#include "RollBallSkillGraph.h"

#include "Engine/DataTable.h"

#define LOCTEXT_NAMESPACE "RollBallSkill"

DEFINE_LOG_CATEGORY_STATIC(LogRollBallSkill, Log, All);

bool URollBallSkillGraph::BuildFromTables(UDataTable* DefinitionTable, UDataTable* PlacementTable, UDataTable* EdgeTable)
{
	Nodes.Reset();
	Edges.Reset();
	NodeIndexByPlacementId.Reset();
	BuildErrors.Reset();
	CoreNodeIndex = INDEX_NONE;

	if (DefinitionTable == nullptr || PlacementTable == nullptr || EdgeTable == nullptr)
	{
		BuildErrors.Add(TEXT("스킬 데이터 테이블 3개(정의/배치/연결)가 모두 지정되어야 합니다."));
		return false;
	}

	static const FString Context(TEXT("RollBallSkillGraph"));

	TArray<FRollBallSkillPlacementRow*> PlacementRows;
	PlacementTable->GetAllRows(Context, PlacementRows);

	for (const FRollBallSkillPlacementRow* Placement : PlacementRows)
	{
		if (Placement == nullptr)
		{
			continue;
		}

		if (NodeIndexByPlacementId.Contains(Placement->PlacementId))
		{
			BuildErrors.Add(FString::Printf(
				TEXT("배치 식별자 %d 가 두 번 쓰였습니다."), Placement->PlacementId));
			continue;
		}

		const FRollBallSkillDefRow* Definition =
			DefinitionTable->FindRow<FRollBallSkillDefRow>(Placement->DefinitionRowName, Context, false);

		if (Definition == nullptr)
		{
			BuildErrors.Add(FString::Printf(
				TEXT("배치 %d 가 가리키는 정의 '%s' 가 정의 테이블에 없습니다."),
				Placement->PlacementId, *Placement->DefinitionRowName.ToString()));
			continue;
		}

		FRollBallSkillNode Node;
		Node.NodeIndex = Nodes.Num();
		Node.PlacementId = Placement->PlacementId;
		Node.DefinitionRowName = Placement->DefinitionRowName;
		Node.Cell = FVector2D(Placement->CellX, Placement->CellY);
		Node.Definition = *Definition;

		if (Node.Definition.MaxLevel < 1)
		{
			BuildErrors.Add(FString::Printf(
				TEXT("정의 '%s' 의 최대 레벨이 1 미만입니다."), *Placement->DefinitionRowName.ToString()));
			Node.Definition.MaxLevel = 1;
		}

		if (Node.Definition.ScaleX <= 0.0f || Node.Definition.ScaleY <= 0.0f)
		{
			BuildErrors.Add(FString::Printf(
				TEXT("정의 '%s' 의 크기 배율이 0 이하입니다."), *Placement->DefinitionRowName.ToString()));
			Node.Definition.ScaleX = FMath::Max(0.1f, Node.Definition.ScaleX);
			Node.Definition.ScaleY = FMath::Max(0.1f, Node.Definition.ScaleY);
		}

		if (Node.Definition.NodeKind == ERollBallSkillNodeKind::Core)
		{
			if (CoreNodeIndex != INDEX_NONE)
			{
				BuildErrors.Add(TEXT("시작점이 두 개 이상입니다. 정확히 하나여야 합니다."));
			}
			else
			{
				CoreNodeIndex = Node.NodeIndex;
			}
		}

		NodeIndexByPlacementId.Add(Node.PlacementId, Node.NodeIndex);
		Nodes.Add(MoveTemp(Node));
	}

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		for (int32 j = i + 1; j < Nodes.Num(); ++j)
		{
			if (Nodes[i].Cell.Equals(Nodes[j].Cell, 0.01))
			{
				BuildErrors.Add(FString::Printf(
					TEXT("배치 %d 와 %d 가 같은 칸 (%.1f, %.1f) 에 겹쳐 있습니다."),
					Nodes[i].PlacementId, Nodes[j].PlacementId, Nodes[i].Cell.X, Nodes[i].Cell.Y));
			}
		}
	}

	if (CoreNodeIndex == INDEX_NONE)
	{
		BuildErrors.Add(TEXT("시작점(Core) 노드가 없습니다."));
	}

	TArray<FRollBallSkillEdgeRow*> EdgeRows;
	EdgeTable->GetAllRows(Context, EdgeRows);

	TSet<uint64> SeenPairs;

	for (const FRollBallSkillEdgeRow* Row : EdgeRows)
	{
		if (Row == nullptr)
		{
			continue;
		}

		const int32* IndexA = NodeIndexByPlacementId.Find(Row->FromPlacementId);
		const int32* IndexB = NodeIndexByPlacementId.Find(Row->ToPlacementId);

		if (IndexA == nullptr || IndexB == nullptr)
		{
			BuildErrors.Add(FString::Printf(
				TEXT("간선 %d-%d 의 한쪽 배치가 없습니다."), Row->FromPlacementId, Row->ToPlacementId));
			continue;
		}

		if (*IndexA == *IndexB)
		{
			BuildErrors.Add(FString::Printf(
				TEXT("간선 %d 가 자기 자신을 잇고 있습니다."), Row->FromPlacementId));
			continue;
		}

		const uint64 Low = static_cast<uint64>(FMath::Min(*IndexA, *IndexB));
		const uint64 High = static_cast<uint64>(FMath::Max(*IndexA, *IndexB));
		const uint64 Key = (High << 32) | Low;

		if (SeenPairs.Contains(Key))
		{
			BuildErrors.Add(FString::Printf(
				TEXT("간선 %d-%d 가 두 번 들어 있습니다."), Row->FromPlacementId, Row->ToPlacementId));
			continue;
		}
		SeenPairs.Add(Key);

		FRollBallSkillEdge Edge;
		Edge.NodeIndexA = *IndexA;
		Edge.NodeIndexB = *IndexB;
		Edge.RouteType = Row->RouteType;
		Edges.Add(Edge);

		Nodes[*IndexA].Adjacent.AddUnique(*IndexB);
		Nodes[*IndexB].Adjacent.AddUnique(*IndexA);
	}

	for (const FString& Error : BuildErrors)
	{
		UE_LOG(LogRollBallSkill, Error, TEXT("[스킬 데이터] %s"), *Error);
	}

	EnsureCoreOwned();
	OnGraphChanged.Broadcast();

	return Nodes.Num() > 0 && CoreNodeIndex != INDEX_NONE;
}

int32 URollBallSkillGraph::FindNodeIndexByPlacementId(int32 PlacementId) const
{
	const int32* Found = NodeIndexByPlacementId.Find(PlacementId);
	return Found != nullptr ? *Found : INDEX_NONE;
}

void URollBallSkillGraph::GetCellBounds(FVector2D& OutMin, FVector2D& OutMax) const
{
	if (Nodes.Num() == 0)
	{
		OutMin = FVector2D::ZeroVector;
		OutMax = FVector2D::ZeroVector;
		return;
	}

	OutMin = Nodes[0].Cell;
	OutMax = Nodes[0].Cell;

	for (const FRollBallSkillNode& Node : Nodes)
	{
		OutMin.X = FMath::Min(OutMin.X, Node.Cell.X);
		OutMin.Y = FMath::Min(OutMin.Y, Node.Cell.Y);
		OutMax.X = FMath::Max(OutMax.X, Node.Cell.X);
		OutMax.Y = FMath::Max(OutMax.Y, Node.Cell.Y);
	}
}

void URollBallSkillGraph::SetLevelsByPlacementId(const TMap<int32, int32>& InLevels)
{
	LevelsByPlacementId.Reset();

	for (const TPair<int32, int32>& Pair : InLevels)
	{
		const int32 NodeIndex = FindNodeIndexByPlacementId(Pair.Key);
		if (NodeIndex == INDEX_NONE)
		{
			continue;
		}

		const int32 Level = FMath::Clamp(Pair.Value, 0, Nodes[NodeIndex].Definition.MaxLevel);
		if (Level > 0)
		{
			LevelsByPlacementId.Add(Pair.Key, Level);
		}
	}

	EnsureCoreOwned();
	OnGraphChanged.Broadcast();
}

int32 URollBallSkillGraph::GetLevel(int32 NodeIndex) const
{
	if (!Nodes.IsValidIndex(NodeIndex))
	{
		return 0;
	}

	const int32* Found = LevelsByPlacementId.Find(Nodes[NodeIndex].PlacementId);
	return Found != nullptr ? *Found : 0;
}

bool URollBallSkillGraph::IsMaxed(int32 NodeIndex) const
{
	if (!Nodes.IsValidIndex(NodeIndex))
	{
		return false;
	}

	return GetLevel(NodeIndex) >= Nodes[NodeIndex].Definition.MaxLevel;
}

int32 URollBallSkillGraph::CostAtLevel(const FRollBallSkillNode& Node, int32 Level) const
{

	const double Raw = Node.Definition.BaseCost * FMath::Pow(Node.Definition.CostGrowth, static_cast<float>(Level));
	return FMath::RoundToInt(Raw / 5.0) * 5;
}

int32 URollBallSkillGraph::GetNextCost(int32 NodeIndex) const
{
	if (!Nodes.IsValidIndex(NodeIndex) || IsMaxed(NodeIndex))
	{
		return 0;
	}

	return CostAtLevel(Nodes[NodeIndex], GetLevel(NodeIndex));
}

int32 URollBallSkillGraph::GetSpentGold(int32 NodeIndex) const
{
	if (!Nodes.IsValidIndex(NodeIndex))
	{
		return 0;
	}

	const int32 Level = GetLevel(NodeIndex);
	int32 Total = 0;
	for (int32 i = 0; i < Level; ++i)
	{
		Total += CostAtLevel(Nodes[NodeIndex], i);
	}
	return Total;
}

bool URollBallSkillGraph::IsGateOpen(int32 NodeIndex, int32 BestStage) const
{
	if (!Nodes.IsValidIndex(NodeIndex))
	{
		return false;
	}

	const FRollBallSkillDefRow& Definition = Nodes[NodeIndex].Definition;
	if (Definition.NodeKind != ERollBallSkillNodeKind::Gate)
	{
		return true;
	}

	return BestStage >= Definition.RequiredBestStage;
}

bool URollBallSkillGraph::HasOwnedNeighbour(int32 NodeIndex) const
{
	if (!Nodes.IsValidIndex(NodeIndex))
	{
		return false;
	}

	for (const int32 Neighbour : Nodes[NodeIndex].Adjacent)
	{
		if (IsOwned(Neighbour))
		{
			return true;
		}
	}
	return false;
}

bool URollBallSkillGraph::CanBuy(int32 NodeIndex, int32 Gold, int32 BestStage, FText& OutReason) const
{
	OutReason = FText::GetEmpty();

	if (!Nodes.IsValidIndex(NodeIndex))
	{
		OutReason = LOCTEXT("SkillBuyNoNode", "노드를 찾을 수 없습니다.");
		return false;
	}

	const FRollBallSkillNode& Node = Nodes[NodeIndex];

	if (Node.Definition.NodeKind == ERollBallSkillNodeKind::Core)
	{
		OutReason = LOCTEXT("SkillBuyCore", "시작점은 살 수 없습니다.");
		return false;
	}

	if (IsMaxed(NodeIndex))
	{
		OutReason = LOCTEXT("SkillBuyMaxed", "이미 만렙입니다.");
		return false;
	}

	if (!IsGateOpen(NodeIndex, BestStage))
	{
		OutReason = FText::Format(
			LOCTEXT("SkillBuyGate", "관문이 잠겨 있습니다. 최고 {0} 스테이지 도달이 필요합니다."),
			FText::AsNumber(Node.Definition.RequiredBestStage));
		return false;
	}

	if (!IsOwned(NodeIndex) && !HasOwnedNeighbour(NodeIndex))
	{
		OutReason = LOCTEXT("SkillBuyNotReachable", "이어진 노드를 먼저 찍어야 합니다.");
		return false;
	}

	const int32 Cost = GetNextCost(NodeIndex);
	if (Gold < Cost)
	{
		OutReason = FText::Format(
			LOCTEXT("SkillBuyPoor", "골드가 부족합니다. ({0} 필요)"), FText::AsNumber(Cost));
		return false;
	}

	return true;
}

bool URollBallSkillGraph::TryBuy(int32 NodeIndex, int32& InOutGold, int32 BestStage, FText& OutReason)
{
	if (!CanBuy(NodeIndex, InOutGold, BestStage, OutReason))
	{
		return false;
	}

	const int32 Cost = GetNextCost(NodeIndex);
	InOutGold -= Cost;

	const int32 PlacementId = Nodes[NodeIndex].PlacementId;
	LevelsByPlacementId.FindOrAdd(PlacementId) += 1;

	OnGraphChanged.Broadcast();
	return true;
}

bool URollBallSkillGraph::TryBuyPath(const TArray<int32>& Path, int32& InOutGold, int32 BestStage, FText& OutReason)
{
	OutReason = FText::GetEmpty();

	if (Path.Num() == 0)
	{
		OutReason = LOCTEXT("SkillPathEmpty", "살 수 있는 경로가 없습니다.");
		return false;
	}

	const int32 TotalCost = GetPathCost(Path);
	if (InOutGold < TotalCost)
	{
		OutReason = FText::Format(
			LOCTEXT("SkillPathPoor", "경로 전체를 사려면 {0} 골드가 필요합니다."), FText::AsNumber(TotalCost));
		return false;
	}

	const TMap<int32, int32> Snapshot = LevelsByPlacementId;
	const int32 GoldSnapshot = InOutGold;

	for (const int32 NodeIndex : Path)
	{
		FText StepReason;
		if (!CanBuy(NodeIndex, InOutGold, BestStage, StepReason))
		{
			LevelsByPlacementId = Snapshot;
			InOutGold = GoldSnapshot;
			OutReason = StepReason;
			return false;
		}

		InOutGold -= GetNextCost(NodeIndex);
		LevelsByPlacementId.FindOrAdd(Nodes[NodeIndex].PlacementId) += 1;
	}

	OnGraphChanged.Broadcast();
	return true;
}

bool URollBallSkillGraph::FindPathTo(int32 TargetIndex, int32 BestStage, TArray<int32>& OutPath) const
{
	OutPath.Reset();

	if (!Nodes.IsValidIndex(TargetIndex))
	{
		return false;
	}

	if (IsOwned(TargetIndex))
	{
		if (!IsMaxed(TargetIndex))
		{
			OutPath.Add(TargetIndex);
			return true;
		}
		return false;
	}

	TArray<int32> Queue;
	TArray<int32> CameFrom;
	TBitArray<> Seen(false, Nodes.Num());

	CameFrom.Init(INDEX_NONE, Nodes.Num());

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		if (IsOwned(i))
		{
			Queue.Add(i);
			Seen[i] = true;
		}
	}

	bool bReached = false;

	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const int32 Current = Queue[Head];
		if (Current == TargetIndex)
		{
			bReached = true;
			break;
		}

		for (const int32 Next : Nodes[Current].Adjacent)
		{
			if (Seen[Next])
			{
				continue;
			}

			if (!IsGateOpen(Next, BestStage) && Next != TargetIndex)
			{
				continue;
			}

			Seen[Next] = true;
			CameFrom[Next] = Current;
			Queue.Add(Next);
		}
	}

	if (!bReached && !Seen[TargetIndex])
	{
		return false;
	}

	for (int32 Step = TargetIndex; Step != INDEX_NONE; Step = CameFrom[Step])
	{
		if (!IsOwned(Step))
		{
			OutPath.Insert(Step, 0);
		}
	}

	return OutPath.Num() > 0;
}

int32 URollBallSkillGraph::GetPathCost(const TArray<int32>& Path) const
{
	int32 Total = 0;
	for (const int32 NodeIndex : Path)
	{
		Total += GetNextCost(NodeIndex);
	}
	return Total;
}

FRollBallSkillStats URollBallSkillGraph::Recalculate() const
{
	FRollBallSkillStats Stats;

	const int32 StatCount = static_cast<int32>(ERollBallSkillStat::Count);
	for (int32 i = 0; i < StatCount; ++i)
	{
		const ERollBallSkillStat Stat = static_cast<ERollBallSkillStat>(i);
		if (Stat == ERollBallSkillStat::None)
		{
			continue;
		}
		Stats.Amounts.Add(Stat, FRollBallSkillStats::BaseAmountOf(Stat));
	}

	for (const FRollBallSkillNode& Node : Nodes)
	{
		const int32 Level = GetLevel(Node.NodeIndex);
		if (Level <= 0 || Node.Definition.GrantStat == ERollBallSkillStat::None)
		{
			continue;
		}

		float& Amount = Stats.Amounts.FindOrAdd(Node.Definition.GrantStat);
		Amount += Node.Definition.AmountPerLevel * Level;
	}

	for (TPair<ERollBallSkillStat, float>& Pair : Stats.Amounts)
	{
		Pair.Value = FRollBallSkillStats::ClampAmount(Pair.Key, Pair.Value);
	}

	if (!Stats.IsUnlocked(ERollBallSkillStat::HammerUnlock))
	{
		Stats.Amounts.Add(ERollBallSkillStat::HammerCount, 0.0f);
	}

	return Stats;
}

int32 URollBallSkillGraph::GetTotalSpentGold() const
{
	int32 Total = 0;
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		Total += GetSpentGold(i);
	}
	return Total;
}

void URollBallSkillGraph::ResetAllLevels()
{
	LevelsByPlacementId.Reset();
	EnsureCoreOwned();
	OnGraphChanged.Broadcast();
}

void URollBallSkillGraph::EnsureCoreOwned()
{
	if (Nodes.IsValidIndex(CoreNodeIndex))
	{
		LevelsByPlacementId.Add(Nodes[CoreNodeIndex].PlacementId, 1);
	}
}

#undef LOCTEXT_NAMESPACE
