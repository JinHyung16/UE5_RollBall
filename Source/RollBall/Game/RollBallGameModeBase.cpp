#include "RollBallGameModeBase.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

#include "RollBallGameInstance.h"
#include "RollBallPlayer.h"
#include "RollBallWidget.h"
#include "RollBall/Stage/RollBallArena.h"
#include "RollBall/UI/RollBallHudWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogRollBallStage, Log, All);

static TAutoConsoleVariable<int32> CVarAutoShotSeconds(
	TEXT("rollball.AutoShot"),
	0,
	TEXT("N 초마다 스크린샷을 찍는다. 0 이면 끔. 테스트용."),
	ECVF_Cheat);

ARollBallGameModeBase::ARollBallGameModeBase()
{
	PrimaryActorTick.bCanEverTick = false;

	EnemyClass = ARollBallEnemy::StaticClass();
	DefaultPawnClass = ARollBallPlayer::StaticClass();

	// HUD 배치는 위젯 블루프린트에 있다. 에셋이 없으면 FClassFinder 가 오류 로그를 남기고 C++ 클래스로 뜬다(화면은 빈다).
	static ConstructorHelpers::FClassFinder<UUserWidget> HudWidgetFinder(TEXT("/Game/UI/WBP_Hud"));
	HudWidgetClass = HudWidgetFinder.Succeeded() ? HudWidgetFinder.Class : TSubclassOf<UUserWidget>(URollBallHudWidget::StaticClass());
}

void ARollBallGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>();

	if (GameInstance != nullptr)
	{
		Setup = GameInstance->GetCurrentStageSetup();

		GameInstance->ReportStageReached(Setup.StageNumber);
	}
	else
	{
		Setup = FRollBallStageSetup();
	}

	RemainingTime = Setup.SurviveSeconds;
	Phase = ERollBallStagePhase::Normal;

	BuildArenaIfMissing();
	PrewarmPool();

	if (ARollBallPlayer* Player = GetBallPlayer())
	{
		Player->ApplySkillStats();
		Player->OnDied.AddDynamic(this, &ARollBallGameModeBase::HandlePlayerDied);
	}

	if (HudWidgetClass != nullptr)
	{
		GameWidget = Cast<URollBallWidget>(CreateWidget(GetWorld(), HudWidgetClass));
		if (GameWidget != nullptr)
		{
			GameWidget->AddToViewport();
		}
	}

	// 메뉴와 결과 창이 건 UIOnly 는 뷰포트에 "입력 무시"를 켜고, 이건 레벨을 옮겨도 남는다.
	// 되돌리지 않으면 메뉴에서 시작하거나 판을 다시 시작했을 때 WASD 가 먹지 않는다.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	UpdateHud();

	UE_LOG(LogRollBallStage, Log,
		TEXT("스테이지 %d 시작. %.0f초 버티기, 스폰 %.2f초, 적 체력 x%.2f, 킬당 %d골드%s"),
		Setup.StageNumber, Setup.SurviveSeconds, Setup.SpawnInterval,
		Setup.EnemyHealthScale, Setup.GoldPerKill,
		Setup.bBossStage ? TEXT(", 보스 판") : TEXT(""));

	GetWorldTimerManager().SetTimer(ClockHandle, this, &ARollBallGameModeBase::TickClock, 0.1f, true);
	GetWorldTimerManager().SetTimer(SpawnHandle, this, &ARollBallGameModeBase::SpawnWave,
		FMath::Max(0.05f, Setup.SpawnInterval), true);
}

void ARollBallGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ARollBallPlayer* Player = GetBallPlayer())
	{
		Player->OnDied.RemoveDynamic(this, &ARollBallGameModeBase::HandlePlayerDied);
	}

	for (ARollBallEnemy* Enemy : AllEnemies)
	{
		if (IsValid(Enemy))
		{
			Enemy->OnDied.RemoveDynamic(this, &ARollBallGameModeBase::HandleEnemyDied);
		}
	}

	UE_LOG(LogRollBallStage, Log, TEXT("풀 %d개, 부족해서 새로 만든 것 %d개"), AllEnemies.Num(), PoolMisses);

	Super::EndPlay(EndPlayReason);
}

void ARollBallGameModeBase::TickClock()
{
	if (bStageFinished)
	{
		return;
	}

	RemainingTime = FMath::Max(0.0f, RemainingTime - 0.1f);

	if (Phase == ERollBallStagePhase::Normal && RemainingTime <= Setup.EliteSeconds)
	{
		EnterPhase(Setup.bBossStage ? ERollBallStagePhase::Boss : ERollBallStagePhase::Elite);
	}

	CullDistantEnemies();
	UpdateHud();

	++ClockTicks;

	const int32 ShotSeconds = CVarAutoShotSeconds.GetValueOnGameThread();
	if (ShotSeconds > 0 && ClockTicks % (ShotSeconds * 10) == 0)
	{
		FScreenshotRequest::RequestScreenshot(true);
	}

	if (ClockTicks % 300 == 0)
	{
		const ARollBallPlayer* Player = GetBallPlayer();
		UE_LOG(LogRollBallStage, Log,
			TEXT("  %.0f초 남음. 적 %d, 공 (%.0f, %.0f, %.0f) 체력 %d, 골드 %d"),
			RemainingTime, AliveEnemies.Num(),
			Player != nullptr ? Player->GetActorLocation().X : 0.0,
			Player != nullptr ? Player->GetActorLocation().Y : 0.0,
			Player != nullptr ? Player->GetActorLocation().Z : 0.0,
			Player != nullptr ? Player->GetHealth() : 0,
			GoldEarned);
	}

	if (RemainingTime <= 0.0f)
	{
		FinishStage(ERollBallStageResult::Cleared);
	}
}

void ARollBallGameModeBase::EnterPhase(ERollBallStagePhase NewPhase)
{
	if (Phase == NewPhase)
	{
		return;
	}

	Phase = NewPhase;

	if (Phase == ERollBallStagePhase::Boss)
	{
		SpawnEnemy(ERollBallEnemyKind::Chaser, ERollBallSpawnRank::Boss);
	}

	if (GameWidget != nullptr)
	{
		GameWidget->SetPhase(Phase);
	}
}

void ARollBallGameModeBase::SpawnWave()
{
	if (bStageFinished || EnemyClass == nullptr)
	{
		return;
	}

	if (AliveEnemies.Num() >= MaxAliveEnemies)
	{
		return;
	}

	const ERollBallSpawnRank Rank = (Phase == ERollBallStagePhase::Normal)
		? ERollBallSpawnRank::Normal
		: ERollBallSpawnRank::Elite;

	SpawnEnemy(PickKindForPhase(), Rank);
}

float ARollBallGameModeBase::ScaleForRank(ERollBallSpawnRank Rank) const
{
	const URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>();

	switch (Rank)
	{
	case ERollBallSpawnRank::Elite:
		return GameInstance != nullptr ? GameInstance->StageRules.EliteScale : 2.5f;

	case ERollBallSpawnRank::Boss:
		return GameInstance != nullptr ? GameInstance->StageRules.BossScale : 18.0f;

	default:
		return 1.0f;
	}
}

ERollBallEnemyKind ARollBallGameModeBase::PickKindForPhase() const
{
	const int32 Roll = FMath::RandRange(0, 99);

	if (Phase == ERollBallStagePhase::Normal)
	{
		if (Roll < 65) return ERollBallEnemyKind::Chaser;
		if (Roll < 85) return ERollBallEnemyKind::Roller;
		return ERollBallEnemyKind::Blocker;
	}

	if (Roll < 50) return ERollBallEnemyKind::Chaser;
	if (Roll < 75) return ERollBallEnemyKind::Roller;
	return ERollBallEnemyKind::Meteor;
}

ARollBallEnemy* ARollBallGameModeBase::SpawnEnemy(ERollBallEnemyKind Kind, ERollBallSpawnRank Rank)
{
	UWorld* World = GetWorld();
	if (World == nullptr || EnemyClass == nullptr)
	{
		return nullptr;
	}

	const ARollBallPlayer* Player = GetBallPlayer();
	FVector Location;
	if (!FindSpawnLocation(Kind, Player, Location))
	{
		return nullptr;
	}

	ARollBallEnemy* Enemy = AcquireEnemy();
	if (Enemy == nullptr)
	{
		return nullptr;
	}

	const float Multiplier = ScaleForRank(Rank);
	const float Size = (Rank == ERollBallSpawnRank::Boss) ? 3.0f
		: (Rank == ERollBallSpawnRank::Elite) ? 1.5f : 1.0f;

	Enemy->Activate(
		Kind,
		Setup.EnemyHealthScale * Multiplier,
		Setup.EnemySpeedScale,
		FMath::RoundToInt(Setup.GoldPerKill * Multiplier),
		Size,
		Location);

	AliveEnemies.Add(Enemy);

	return Enemy;
}

ARollBallEnemy* ARollBallGameModeBase::CreatePooledEnemy()
{
	UWorld* World = GetWorld();
	if (World == nullptr || EnemyClass == nullptr)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;

	ARollBallEnemy* Enemy = World->SpawnActor<ARollBallEnemy>(
		EnemyClass, FVector(0.0f, 0.0f, -100000.0f), FRotator::ZeroRotator, Params);
	if (Enemy == nullptr)
	{
		return nullptr;
	}

	Enemy->SetAutoActivate(false);
	Enemy->FinishSpawning(FTransform(FVector(0.0f, 0.0f, -100000.0f)));
	Enemy->Deactivate();
	Enemy->OnDied.AddDynamic(this, &ARollBallGameModeBase::HandleEnemyDied);

	AllEnemies.Add(Enemy);
	return Enemy;
}

ARollBallEnemy* ARollBallGameModeBase::AcquireEnemy()
{
	while (Pool.Num() > 0)
	{
		ARollBallEnemy* Enemy = Pool.Pop(EAllowShrinking::No);
		if (IsValid(Enemy))
		{
			return Enemy;
		}
	}

	++PoolMisses;
	return CreatePooledEnemy();
}

void ARollBallGameModeBase::ReleaseEnemy(ARollBallEnemy* Enemy)
{
	if (!IsValid(Enemy))
	{
		return;
	}

	if (Enemy->IsActive())
	{
		Enemy->Deactivate();
	}

	Pool.AddUnique(Enemy);
}

void ARollBallGameModeBase::PrewarmPool()
{
	const int32 Target = FMath::Max(0, PoolPrewarmCount);
	Pool.Reserve(Target);
	AllEnemies.Reserve(Target);

	for (int32 i = 0; i < Target; ++i)
	{
		if (ARollBallEnemy* Enemy = CreatePooledEnemy())
		{
			Pool.Add(Enemy);
		}
	}
}

bool ARollBallGameModeBase::FindSpawnLocation(ERollBallEnemyKind Kind, const ARollBallPlayer* Player,
	FVector& OutLocation) const
{
	if (Player == nullptr)
	{
		return false;
	}

	const FVector Centre = Player->GetActorLocation();

	if (Kind == ERollBallEnemyKind::Meteor)
	{
		const float Spread = 400.0f;
		OutLocation = Centre + FVector(
			FMath::FRandRange(-Spread, Spread),
			FMath::FRandRange(-Spread, Spread),
			1800.0f);
		return true;
	}

	const UWorld* World = GetWorld();

	FCollisionQueryParams Query;
	Query.AddIgnoredActor(Player);

	// 바위 위에 내려앉으면 적이 공중에 떠서 움직이므로, 바위가 아닌 바닥이 나올 때까지 방향을 바꿔 본다.
	const int32 MaxAttempts = 8;
	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		OutLocation = Centre + FVector(FMath::Cos(Angle) * SpawnRadius, FMath::Sin(Angle) * SpawnRadius, 0.0f);

		if (Arena != nullptr)
		{
			const float Limit = Arena->GetArenaRadius() - 300.0f;
			OutLocation.X = FMath::Clamp(OutLocation.X, -Limit, Limit);
			OutLocation.Y = FMath::Clamp(OutLocation.Y, -Limit, Limit);
		}

		if (World == nullptr)
		{
			break;
		}

		FHitResult Hit;
		const FVector TraceStart = OutLocation + FVector(0.0f, 0.0f, 2000.0f);
		const FVector TraceEnd = OutLocation - FVector(0.0f, 0.0f, 4000.0f);

		if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Query))
		{
			break;
		}

		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		const bool bOnRock = HitComponent != nullptr && HitComponent->ComponentHasTag(ARollBallArena::RockTag);

		if (!bOnRock || Attempt == MaxAttempts - 1)
		{
			OutLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, 60.0f);
			break;
		}
	}

	return true;
}

void ARollBallGameModeBase::CullDistantEnemies()
{
	const ARollBallPlayer* Player = GetBallPlayer();
	if (Player == nullptr)
	{
		return;
	}

	const FVector Centre = Player->GetActorLocation();
	const float CullSquared = FMath::Square(CullDistance);

	for (int32 i = AliveEnemies.Num() - 1; i >= 0; --i)
	{
		ARollBallEnemy* Enemy = AliveEnemies[i];

		if (!IsValid(Enemy))
		{
			AliveEnemies.RemoveAtSwap(i);
			continue;
		}

		if (FVector::DistSquared(Enemy->GetActorLocation(), Centre) > CullSquared)
		{
			AliveEnemies.RemoveAtSwap(i);
			ReleaseEnemy(Enemy);
		}
	}
}

void ARollBallGameModeBase::BuildArenaIfMissing()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (TActorIterator<ARollBallArena> It(World); It; ++It)
	{
		Arena = *It;
		return;
	}

	if (!bAutoBuildArena)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Arena = World->SpawnActor<ARollBallArena>(
		ARollBallArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

	if (Arena != nullptr)
	{
		Arena->Build(Setup.StageNumber);
	}
}

void ARollBallGameModeBase::HandleEnemyDied(ARollBallEnemy* Enemy, int32 GoldReward)
{
	AliveEnemies.RemoveSwap(Enemy);
	Pool.AddUnique(Enemy);

	if (bStageFinished || GoldReward <= 0)
	{
		return;
	}

	++KillCount;

	if (URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>())
	{
		GoldEarned += GameInstance->AddEarnedGold(GoldReward);
	}
	else
	{
		GoldEarned += GoldReward;
	}

	UpdateHud();
}

void ARollBallGameModeBase::HandlePlayerDied()
{
	FinishStage(ERollBallStageResult::Failed);
}

void ARollBallGameModeBase::FinishStage(ERollBallStageResult Result)
{
	if (bStageFinished)
	{
		return;
	}
	bStageFinished = true;
	StageResult = Result;

	GetWorldTimerManager().ClearTimer(ClockHandle);
	GetWorldTimerManager().ClearTimer(SpawnHandle);

	const float Survived = FMath::Max(0.0f, Setup.SurviveSeconds - RemainingTime);
	const int32 SurvivalGold = FMath::RoundToInt(Survived / 60.0f * Setup.SurvivalGoldPerMinute);

	if (SurvivalGold > 0)
	{
		if (URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>())
		{
			GoldEarned += GameInstance->AddEarnedGold(SurvivalGold);
		}
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetCinematicMode(true, false, false, true, true);
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	if (URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>())
	{
		GameInstance->SaveAccount();
	}

	UE_LOG(LogRollBallStage, Log,
		TEXT("스테이지 %d 끝. %s, %.0f초 버팀, 처치 %d, 골드 %d, 남은 적 %d"),
		Setup.StageNumber,
		(Result == ERollBallStageResult::Cleared) ? TEXT("생존") : TEXT("사망"),
		Survived, KillCount, GoldEarned, AliveEnemies.Num());

	if (GameWidget != nullptr)
	{
		GameWidget->SetResultSummary(Result, KillCount, GoldEarned);
		GameWidget->ShowResult(Result);
	}

	// 시계가 멈춰 자동 스크린샷도 멈추므로, 테스트 중이면 결과 창을 한 장 따로 찍는다.
	if (CVarAutoShotSeconds.GetValueOnGameThread() > 0)
	{
		FScreenshotRequest::RequestScreenshot(true);
	}

	if (ResultAutoAdvanceSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(AdvanceHandle, this,
			&ARollBallGameModeBase::HandleAutoAdvance, ResultAutoAdvanceSeconds, false);
	}
}

void ARollBallGameModeBase::HandleAutoAdvance()
{
	const bool bAdvance = (StageResult == ERollBallStageResult::Cleared) || bAdvanceOnFailure;

	if (bAdvance)
	{
		RequestNextStage();
	}
	else
	{
		RequestRetryStage();
	}
}

void ARollBallGameModeBase::UpdateHud()
{
	if (GameWidget == nullptr)
	{
		return;
	}

	GameWidget->SetTimeText(RemainingTime);
	GameWidget->SetStageText(Setup.StageNumber, Setup.bBossStage);
	GameWidget->SetGoldText(GoldEarned, KillCount);
	GameWidget->SetPhase(Phase);

	if (const ARollBallPlayer* Player = GetBallPlayer())
	{
		GameWidget->SetHealth(Player->GetHealth(), Player->GetMaxHealth());
	}
}

ARollBallPlayer* ARollBallGameModeBase::GetBallPlayer() const
{
	return Cast<ARollBallPlayer>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void ARollBallGameModeBase::RequestNextStage()
{
	URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>();
	if (GameInstance == nullptr)
	{
		UGameplayStatics::OpenLevel(this, MainMenuLevelName);
		return;
	}

	const int32 Next = Setup.StageNumber + 1;

	if (Next > GameInstance->StageRules.MaxStage)
	{
		UGameplayStatics::OpenLevel(this, MainMenuLevelName);
		return;
	}

	GameInstance->TravelToStage(Next);
}

void ARollBallGameModeBase::RequestRetryStage()
{
	if (URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>())
	{
		GameInstance->TravelToStage(Setup.StageNumber);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
}
