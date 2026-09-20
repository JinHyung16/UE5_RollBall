#pragma once

#include "CoreMinimal.h"
#include "RollBall/Game/RollBallWidget.h"
#include "RollBallHudWidget.generated.h"

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
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor PanelColor = FLinearColor(0.03f, 0.035f, 0.05f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor TextColor = FLinearColor(0.92f, 0.94f, 0.98f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor DimTextColor = FLinearColor(0.60f, 0.66f, 0.76f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor HealthColor = FLinearColor(0.95f, 0.30f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor GoldColor = FLinearColor(0.98f, 0.80f, 0.30f);

private:

	float RemainingSeconds = 0.0f;
	float LongestSeenSeconds = 1.0f;

	int32 StageNumber = 1;
	bool bBossStage = false;

	int32 Health = 1;
	int32 MaxHealth = 1;

	int32 GoldEarned = 0;
	int32 KillCount = 0;

	ERollBallStagePhase Phase = ERollBallStagePhase::Normal;

	bool bResultVisible = false;
	ERollBallStageResult Result = ERollBallStageResult::None;

	float UiScale(const FVector2D& Canvas) const;

	FLinearColor PhaseColor() const;
	FString PhaseLabel() const;

	void PaintTopBar(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
	void PaintHealth(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
	void PaintResult(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
};
