#include "RollBallGameModeBase.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "RollBallWidget.h"
#include "RollBallGameInstance.h"
#include "RollBallSpawnPoint.h"
#include "RollBall/Items/RollBallItemBase.h"

ARollBallGameModeBase::ARollBallGameModeBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARollBallGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// Pull stage info from the GameInstance (set up by the main menu flow).
	URollBallGameInstance* GI = GetGameInstance<URollBallGameInstance>();
	FRollBallStageRow Row;
	bool bHasStageData = (GI != nullptr) && GI->GetCurrentStage(Row);

	if (bHasStageData)
	{
		ApplyStageData(Row);
	}
	else
	{
		// Fallback for testing a level directly without going through the menu:
		// count whatever items the designer placed in the level and use a
		// default time limit.
		TArray<AActor*> ExistingItems;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARollBallItemBase::StaticClass(), ExistingItems);
		RequiredItemCount = FMath::Max(1, ExistingItems.Num());
		RemainingTime = 60.0f;
	}

	// Create the HUD.
	if (GameWidgetClass != nullptr)
	{
		GameWidget = Cast<URollBallWidget>(CreateWidget(GetWorld(), GameWidgetClass));
		if (GameWidget != nullptr)
		{
			GameWidget->AddToViewport();
		}
	}
	UpdateHud();

	// Start ticking the timer at 0.1s for smooth countdown UI without using Tick().
	if (RemainingTime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TickHandle, this, &ARollBallGameModeBase::TickTimer, 0.1f, true);
	}
}

void ARollBallGameModeBase::ApplyStageData(const FRollBallStageRow& Row)
{
	RequiredItemCount = FMath::Max(1, Row.RequiredItemCount);
	RemainingTime = Row.TimeLimitSeconds;

	SpawnItemsFromSpawnPoints();
}

void ARollBallGameModeBase::SpawnItemsFromSpawnPoints()
{
	if (ItemClass == nullptr)
	{
		// Designer hasn't picked an item class; fall back to whatever items
		// are already placed in the level so the stage is still playable.
		return;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARollBallSpawnPoint::StaticClass(), Found);

	if (Found.Num() == 0)
	{
		return;
	}

	// Random selection without replacement. If we ask for more items than we
	// have spawn points, just use them all.
	const int32 ToSpawn = FMath::Min(RequiredItemCount, Found.Num());

	// Shuffle in place using a Fisher–Yates-style swap.
	for (int32 i = Found.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Found.Swap(i, j);
	}

	for (int32 i = 0; i < ToSpawn; ++i)
	{
		AActor* SpawnPoint = Found[i];
		const FVector Loc = SpawnPoint->GetActorLocation();
		const FRotator Rot = SpawnPoint->GetActorRotation();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		GetWorld()->SpawnActor<ARollBallItemBase>(ItemClass, Loc, Rot, Params);
	}

	// Update the required count to whatever we actually spawned, so the player
	// can always achieve the goal.
	RequiredItemCount = ToSpawn;
}

void ARollBallGameModeBase::TickTimer()
{
	if (bStageFinished)
	{
		return;
	}

	RemainingTime = FMath::Max(0.0f, RemainingTime - 0.1f);
	UpdateHud();

	if (RemainingTime <= 0.0f)
	{
		FinishStage(ERollBallStageResult::Failed);
	}
}

void ARollBallGameModeBase::ItemCollected()
{
	if (bStageFinished)
	{
		return;
	}

	++ItemsCollected;
	UpdateHud();

	if (ItemsCollected >= RequiredItemCount)
	{
		FinishStage(ERollBallStageResult::Cleared);
	}
}

void ARollBallGameModeBase::FinishStage(ERollBallStageResult Result)
{
	if (bStageFinished)
	{
		return;
	}
	bStageFinished = true;

	GetWorldTimerManager().ClearTimer(TickHandle);

	// Freeze the player by disabling input. The Pawn keeps physics so the ball
	// just rolls to a stop naturally.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetCinematicMode(true, false, false, true, true);
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	if (GameWidget != nullptr)
	{
		GameWidget->ShowResult(Result);
	}
}

void ARollBallGameModeBase::UpdateHud()
{
	if (GameWidget != nullptr)
	{
		GameWidget->SetItemText(ItemsCollected, RequiredItemCount);
		GameWidget->SetTimeText(RemainingTime);
	}
}

// --- Result-button entry points ------------------------------------------

void ARollBallGameModeBase::RequestNextStage()
{
	URollBallGameInstance* GI = GetGameInstance<URollBallGameInstance>();
	if (GI == nullptr)
	{
		return;
	}

	if (GI->HasNextStage())
	{
		GI->AdvanceToNextStage();
		GI->OpenCurrentStageLevel();
	}
	else
	{
		// No more stages — go back to menu.
		UGameplayStatics::OpenLevel(this, MainMenuLevelName);
	}
}

void ARollBallGameModeBase::RequestRetryStage()
{
	if (URollBallGameInstance* GI = GetGameInstance<URollBallGameInstance>())
	{
		GI->OpenCurrentStageLevel();
	}
	else
	{
		// No GameInstance configured — just reload the current map.
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
}

void ARollBallGameModeBase::RequestExitToMenu()
{
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
