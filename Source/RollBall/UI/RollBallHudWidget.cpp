#include "RollBallHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

#include "RollBallPaint.h"

URollBallHudWidget::URollBallHudWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URollBallHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 디자이너의 첫 체력 칸을 견본으로 쓴다. 모양을 바꾸려면 WBP_Hud 에서 그 칸만 고치면 된다.
	if (HealthBox->GetChildrenCount() > 0)
	{
		UWidget* Sample = HealthBox->GetChildAt(0);

		if (const USizeBox* SampleBox = Cast<USizeBox>(Sample))
		{
			PipSize = FVector2D(SampleBox->GetWidthOverride(), SampleBox->GetHeightOverride());

			if (const UBorder* SampleFill = Cast<UBorder>(SampleBox->GetContent()))
			{
				PipBrush = SampleFill->Background;
			}
		}

		if (const UHorizontalBoxSlot* SampleSlot = Cast<UHorizontalBoxSlot>(Sample->Slot))
		{
			PipGap = SampleSlot->GetPadding().Right;
		}
	}

	ResultOverlay->SetVisibility(ESlateVisibility::Collapsed);

	RebuildHealthPips();
	ApplyStage();
	ApplyPhase();
	ApplyGold();
}

void URollBallHudWidget::SetTimeText_Implementation(float InRemainingSeconds)
{
	LongestSeenSeconds = FMath::Max(LongestSeenSeconds, InRemainingSeconds);

	ClockText->SetText(FText::FromString(RollBallPaint::FormatClock(InRemainingSeconds)));
	TimeBar->SetPercent(LongestSeenSeconds > 0.0f
		? FMath::Clamp(InRemainingSeconds / LongestSeenSeconds, 0.0f, 1.0f)
		: 0.0f);
}

void URollBallHudWidget::SetStageText_Implementation(int32 InStageNumber, bool bInBossStage)
{
	StageNumber = InStageNumber;
	bBossStage = bInBossStage;

	ApplyStage();
	ApplyPhase();
}

void URollBallHudWidget::SetHealth_Implementation(int32 InHealth, int32 InMaxHealth)
{
	Health = InHealth;

	const int32 NewMax = FMath::Max(1, InMaxHealth);
	if (NewMax != MaxHealth || HealthPips.Num() != NewMax)
	{
		MaxHealth = NewMax;
		RebuildHealthPips();
	}

	ApplyHealth();
}

void URollBallHudWidget::SetGoldText_Implementation(int32 InGoldEarned, int32 InKillCount)
{
	GoldEarned = InGoldEarned;
	KillCount = InKillCount;

	ApplyGold();
}

void URollBallHudWidget::SetPhase_Implementation(ERollBallStagePhase InPhase)
{
	Phase = InPhase;

	ApplyPhase();
}

void URollBallHudWidget::SetResultSummary_Implementation(ERollBallStageResult InResult, int32 InKillCount, int32 InGoldEarned)
{
	Result = InResult;
	KillCount = InKillCount;
	GoldEarned = InGoldEarned;

	ApplyGold();
	ApplyResult();
}

void URollBallHudWidget::ShowResult_Implementation(ERollBallStageResult InResult)
{
	Result = InResult;

	ApplyResult();
	ResultOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
}

FLinearColor URollBallHudWidget::PhaseColor() const
{
	switch (Phase)
	{
	case ERollBallStagePhase::Elite: return ElitePhaseColor;
	case ERollBallStagePhase::Boss:  return BossPhaseColor;
	default:                         return NormalPhaseColor;
	}
}

FString URollBallHudWidget::PhaseLabel() const
{
	switch (Phase)
	{
	case ERollBallStagePhase::Elite: return TEXT("정예");
	case ERollBallStagePhase::Boss:  return TEXT("보스");
	default:                         return TEXT("일반");
	}
}

void URollBallHudWidget::ApplyPhase()
{
	const FLinearColor Color = PhaseColor();

	const FString Subtitle = bBossStage
		? FString::Printf(TEXT("%s · 보스 판"), *PhaseLabel())
		: PhaseLabel();

	PhaseText->SetText(FText::FromString(Subtitle));
	PhaseText->SetColorAndOpacity(FSlateColor(Color));

	PhaseLine->SetBrushColor(Color);
	TimeBar->SetFillColorAndOpacity(Color);
	ClockText->SetColorAndOpacity(FSlateColor(Phase == ERollBallStagePhase::Normal ? TextColor : Color));
}

void URollBallHudWidget::ApplyStage()
{
	StageText->SetText(FText::FromString(FString::Printf(TEXT("스테이지 %d"), StageNumber)));
}

void URollBallHudWidget::ApplyGold()
{
	GoldText->SetText(FText::FromString(FString::Printf(TEXT("$ %d"), GoldEarned)));
	KillText->SetText(FText::FromString(FString::Printf(TEXT("처치 %d"), KillCount)));
}

void URollBallHudWidget::ApplyResult()
{
	const bool bSurvived = (Result == ERollBallStageResult::Cleared);
	const FLinearColor Accent = bSurvived ? ClearedColor : FailedColor;

	FSlateBrush Panel = ResultPanel->Background;
	Panel.OutlineSettings.Color = FSlateColor(Accent);
	ResultPanel->SetBrush(Panel);

	ResultTitle->SetText(FText::FromString(bSurvived ? TEXT("생존") : TEXT("사망")));
	ResultTitle->SetColorAndOpacity(FSlateColor(Accent));

	ResultStageText->SetText(FText::FromString(FString::Printf(TEXT("스테이지 %d"), StageNumber)));
	ResultKillText->SetText(FText::FromString(FString::Printf(TEXT("처치 %d"), KillCount)));
	ResultGoldText->SetText(FText::FromString(FString::Printf(TEXT("얻은 골드 %d"), GoldEarned)));
	ResultHintText->SetText(FText::FromString(bSurvived ? TEXT("곧 다음 스테이지") : TEXT("곧 이 스테이지 다시")));
}

void URollBallHudWidget::RebuildHealthPips()
{
	HealthBox->ClearChildren();
	HealthPips.Reset();

	for (int32 i = 0; i < MaxHealth; ++i)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(PipSize.X);
		Box->SetHeightOverride(PipSize.Y);

		UBorder* Pip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Pip->SetBrush(PipBrush);
		Box->AddChild(Pip);

		if (UHorizontalBoxSlot* PipSlot = HealthBox->AddChildToHorizontalBox(Box))
		{
			PipSlot->SetPadding(FMargin(0.0f, 0.0f, i + 1 < MaxHealth ? PipGap : 0.0f, 0.0f));
		}

		HealthPips.Add(Pip);
	}

	ApplyHealth();
}

void URollBallHudWidget::ApplyHealth()
{
	for (int32 i = 0; i < HealthPips.Num(); ++i)
	{
		if (HealthPips[i] != nullptr)
		{
			HealthPips[i]->SetBrushColor(i < Health ? HealthColor : EmptyHealthColor);
		}
	}
}
