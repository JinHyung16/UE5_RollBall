#pragma once

#include "CoreMinimal.h"
#include "RollBall/Game/RollBallWidget.h"
#include "RollBallHudWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UProgressBar;
class UTextBlock;

/**
 * 스테이지 HUD 의 동작 부분. 배치와 모양은 /Game/UI/WBP_Hud 디자이너에서 고친다.
 * 게임모드가 Set* 를 부를 때 값만 위젯에 넣는다.
 */
UCLASS()
class ROLLBALL_API URollBallHudWidget : public URollBallWidget
{
	GENERATED_BODY()

public:
	URollBallHudWidget(const FObjectInitializer& ObjectInitializer);

	virtual void SetTimeText_Implementation(float RemainingSeconds) override;
	virtual void SetStageText_Implementation(int32 StageNumber, bool bBossStage) override;
	virtual void SetHealth_Implementation(int32 Health, int32 MaxHealth) override;
	virtual void SetGoldText_Implementation(int32 GoldEarned, int32 KillCount) override;
	virtual void SetPhase_Implementation(ERollBallStagePhase Phase) override;
	virtual void SetResultSummary_Implementation(ERollBallStageResult Result, int32 KillCount, int32 GoldEarned) override;
	virtual void ShowResult_Implementation(ERollBallStageResult Result) override;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor TextColor = FLinearColor(0.92f, 0.94f, 0.98f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor HealthColor = FLinearColor(0.95f, 0.30f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor EmptyHealthColor = FLinearColor(0.10f, 0.11f, 0.14f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|페이즈")
	FLinearColor NormalPhaseColor = FLinearColor(0.30f, 0.60f, 0.90f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|페이즈")
	FLinearColor ElitePhaseColor = FLinearColor(0.95f, 0.55f, 0.20f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|페이즈")
	FLinearColor BossPhaseColor = FLinearColor(0.90f, 0.20f, 0.30f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|결과")
	FLinearColor ClearedColor = FLinearColor(0.40f, 0.95f, 0.50f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|결과")
	FLinearColor FailedColor = FLinearColor(0.95f, 0.35f, 0.35f);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ClockText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> TimeBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KillText;

	/** 상단 바 아래 페이즈 색 줄 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> PhaseLine;

	/** 체력 칸 줄. 디자이너에 둔 첫 칸(SizeBox > Border)의 크기·모양을 본떠 최대 체력만큼 다시 만든다 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HealthBox;

	/** 결과 창 전체. 평소에는 Collapsed */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> ResultOverlay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> ResultPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultTitle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultStageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultKillText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultGoldText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultHintText;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> HealthPips;

	FSlateBrush PipBrush;
	FVector2D PipSize = FVector2D(39.0, 39.0);
	float PipGap = 12.0f;

	float LongestSeenSeconds = 1.0f;

	int32 StageNumber = 1;
	bool bBossStage = false;

	int32 Health = 1;
	int32 MaxHealth = 1;

	int32 GoldEarned = 0;
	int32 KillCount = 0;

	ERollBallStagePhase Phase = ERollBallStagePhase::Normal;
	ERollBallStageResult Result = ERollBallStageResult::None;

	FLinearColor PhaseColor() const;
	FString PhaseLabel() const;

	void ApplyPhase();
	void ApplyStage();
	void ApplyGold();
	void ApplyResult();

	void RebuildHealthPips();
	void ApplyHealth();
};
