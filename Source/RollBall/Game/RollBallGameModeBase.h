#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RollBall/Data/RollBallStageRow.h"
#include "RollBallGameModeBase.generated.h"

class URollBallWidget;
class ARollBallItemBase;
class ARollBallSpawnPoint;

UENUM(BlueprintType)
enum class ERollBallStageResult : uint8
{
	None        UMETA(DisplayName = "None"),
	Cleared     UMETA(DisplayName = "Cleared"),
	Failed      UMETA(DisplayName = "Failed"),
};

UCLASS()
class ROLLBALL_API ARollBallGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARollBallGameModeBase();

	// Called by items when the player overlaps them.
	UFUNCTION(BlueprintCallable, Category = "RollBall")
	void ItemCollected();

	// Buttons on the Result widget should call these.
	UFUNCTION(BlueprintCallable, Category = "RollBall|Result")
	void RequestNextStage();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Result")
	void RequestRetryStage();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Result")
	void RequestExitToMenu();

protected:
	virtual void BeginPlay() override;

	// --- Configurable in the BP subclass ---------------------------------

	// Item class to spawn at each picked spawn point.
	UPROPERTY(EditAnywhere, Category = "RollBall|Spawning")
	TSubclassOf<ARollBallItemBase> ItemClass;

	// HUD shown during play (timer + item counter).
	UPROPERTY(EditAnywhere, Category = "RollBall|Widgets")
	TSubclassOf<class UUserWidget> GameWidgetClass;

	// Map level name to return to on Exit. Usually the main menu map.
	UPROPERTY(EditAnywhere, Category = "RollBall|Flow")
	FName MainMenuLevelName = TEXT("L_MainMenu");

	// --- Runtime state ---------------------------------------------------

	UPROPERTY()
	URollBallWidget* GameWidget = nullptr;

	UPROPERTY()
	int32 ItemsCollected = 0;

	UPROPERTY()
	int32 RequiredItemCount = 0;

	UPROPERTY()
	float RemainingTime = 0.0f;

	UPROPERTY()
	bool bStageFinished = false;

	FTimerHandle TickHandle;

	// --- Internals -------------------------------------------------------

	void ApplyStageData(const FRollBallStageRow& Row);
	void SpawnItemsFromSpawnPoints();
	void TickTimer();
	void FinishStage(ERollBallStageResult Result);
	void UpdateHud();
};
