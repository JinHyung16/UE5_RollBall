#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#include "RollBall/Game/RollBallGameInstance.h"
#include "RollBall/Skill/RollBallSkillGraph.h"

DEFINE_LOG_CATEGORY_STATIC(LogRollBallDebug, Log, All);

namespace
{
	URollBallGameInstance* FindGameInstance(UWorld* World)
	{
		return World != nullptr ? World->GetGameInstance<URollBallGameInstance>() : nullptr;
	}

	void AddGold(const TArray<FString>& Args, UWorld* World)
	{
		URollBallGameInstance* GameInstance = FindGameInstance(World);
		if (GameInstance == nullptr)
		{
			return;
		}

		const int32 Amount = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1000;

		GameInstance->AddGold(Amount);

		UE_LOG(LogRollBallDebug, Log, TEXT("골드 %d 지급. 지금 %d"), Amount, GameInstance->GetGold());
	}

	void AutoBuy(const TArray<FString>& Args, UWorld* World)
	{
		URollBallGameInstance* GameInstance = FindGameInstance(World);
		if (GameInstance == nullptr)
		{
			return;
		}

		URollBallSkillGraph* Graph = GameInstance->EnsureSkillGraph();
		if (Graph == nullptr || !Graph->IsBuilt())
		{
			UE_LOG(LogRollBallDebug, Warning, TEXT("스킬 그래프가 비어 있습니다."));
			return;
		}

		const int32 Limit = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 200;
		int32 Bought = 0;

		const ERollBallSkillStat UnlockStats[] = {
			ERollBallSkillStat::HammerUnlock,
			ERollBallSkillStat::SawUnlock,
			ERollBallSkillStat::DrillUnlock,
			ERollBallSkillStat::BulldozerUnlock,
		};

		for (const ERollBallSkillStat Wanted : UnlockStats)
		{
			if (GameInstance->GetSkillStats().IsUnlocked(Wanted))
			{
				continue;
			}

			for (int32 i = 0; i < Graph->GetNodes().Num(); ++i)
			{
				if (Graph->GetNode(i).Definition.GrantStat != Wanted || Graph->IsOwned(i))
				{
					continue;
				}

				TArray<int32> Path;
				if (!Graph->FindPathTo(i, GameInstance->GetBestStage(), Path))
				{
					continue;
				}

				FText Reason;
				if (GameInstance->BuySkillPath(Path, Reason))
				{
					Bought += Path.Num();
					UE_LOG(LogRollBallDebug, Log, TEXT("경로 %d칸으로 무기 해금. 남은 골드 %d"),
						Path.Num(), GameInstance->GetGold());
				}
				else
				{
					UE_LOG(LogRollBallDebug, Log, TEXT("경로 구매 실패: %s"), *Reason.ToString());
				}
				break;
			}
		}

		for (int32 Step = 0; Step < Limit; ++Step)
		{
			int32 BestNode = INDEX_NONE;
			int32 BestCost = MAX_int32;

			for (int32 i = 0; i < Graph->GetNodes().Num(); ++i)
			{
				FText Reason;
				if (!Graph->CanBuy(i, GameInstance->GetGold(), GameInstance->GetBestStage(), Reason))
				{
					continue;
				}

				const int32 Cost = Graph->GetNextCost(i);
				if (Cost < BestCost)
				{
					BestCost = Cost;
					BestNode = i;
				}
			}

			if (BestNode == INDEX_NONE)
			{
				break;
			}

			FText Reason;
			if (!GameInstance->BuySkillNode(BestNode, Reason))
			{
				UE_LOG(LogRollBallDebug, Warning, TEXT("구매 실패: %s"), *Reason.ToString());
				break;
			}

			++Bought;
		}

		const FRollBallSkillStats Stats = GameInstance->GetSkillStats();

		UE_LOG(LogRollBallDebug, Log,
			TEXT("노드 %d개 삼. 남은 골드 %d. 체력 %d, 가속 %.0f, 속도 %.0f, 헤머 %s%d개, 톱 %s, 불도저 %s, 드릴 %s"),
			Bought, GameInstance->GetGold(),
			Stats.GetInt(ERollBallSkillStat::MaxHealth),
			Stats.Get(ERollBallSkillStat::MoveForce),
			Stats.Get(ERollBallSkillStat::MaxSpeed),
			Stats.IsUnlocked(ERollBallSkillStat::HammerUnlock) ? TEXT("") : TEXT("잠김 "),
			Stats.GetInt(ERollBallSkillStat::HammerCount),
			Stats.IsUnlocked(ERollBallSkillStat::SawUnlock) ? TEXT("장착") : TEXT("잠김"),
			Stats.IsUnlocked(ERollBallSkillStat::BulldozerUnlock) ? TEXT("장착") : TEXT("잠김"),
			Stats.IsUnlocked(ERollBallSkillStat::DrillUnlock) ? TEXT("장착") : TEXT("잠김"));
	}

	FAutoConsoleCommandWithWorldAndArgs AddGoldCommand(
		TEXT("rollball.AddGold"),
		TEXT("테스트용 골드 지급. rollball.AddGold 5000"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AddGold));

	FAutoConsoleCommandWithWorldAndArgs AutoBuyCommand(
		TEXT("rollball.AutoBuy"),
		TEXT("살 수 있는 가장 싼 노드를 계속 산다. rollball.AutoBuy 200"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AutoBuy));
}
