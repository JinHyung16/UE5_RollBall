#include "RollBallHudWidget.h"

#include "RollBallPaint.h"

using namespace RollBallPaint;

URollBallHudWidget::URollBallHudWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URollBallHudWidget::SetTimeText_Implementation(float InRemainingSeconds)
{
	RemainingSeconds = InRemainingSeconds;

	LongestSeenSeconds = FMath::Max(LongestSeenSeconds, InRemainingSeconds);
}

void URollBallHudWidget::SetStageText_Implementation(int32 InStageNumber, bool bInBossStage)
{
	StageNumber = InStageNumber;
	bBossStage = bInBossStage;
}

void URollBallHudWidget::SetHealth_Implementation(int32 InHealth, int32 InMaxHealth)
{
	Health = InHealth;
	MaxHealth = FMath::Max(1, InMaxHealth);
}

void URollBallHudWidget::SetGoldText_Implementation(int32 InGoldEarned, int32 InKillCount)
{
	GoldEarned = InGoldEarned;
	KillCount = InKillCount;
}

void URollBallHudWidget::SetPhase_Implementation(ERollBallStagePhase InPhase)
{
	Phase = InPhase;
}

void URollBallHudWidget::SetResultSummary_Implementation(ERollBallStageResult InResult, int32 InKillCount, int32 InGoldEarned)
{
	Result = InResult;
	KillCount = InKillCount;
	GoldEarned = InGoldEarned;
}

void URollBallHudWidget::ShowResult_Implementation(ERollBallStageResult InResult)
{
	Result = InResult;
	bResultVisible = true;
}

FLinearColor URollBallHudWidget::PhaseColor() const
{
	switch (Phase)
	{
	case ERollBallStagePhase::Elite: return FLinearColor(0.95f, 0.55f, 0.20f);
	case ERollBallStagePhase::Boss:  return FLinearColor(0.90f, 0.20f, 0.30f);
	default:                         return FLinearColor(0.30f, 0.60f, 0.90f);
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

int32 URollBallHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	PaintTopBar(AllottedGeometry, OutDrawElements, LayerId);
	PaintHealth(AllottedGeometry, OutDrawElements, LayerId + 2);

	if (bResultVisible)
	{
		PaintResult(AllottedGeometry, OutDrawElements, LayerId + 4);
	}

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId + 8, InWidgetStyle, bParentEnabled);
}

float URollBallHudWidget::UiScale(const FVector2D& Canvas) const
{

	return FMath::Clamp(static_cast<float>(Canvas.Y) / 720.0f, 0.7f, 2.5f);
}

void URollBallHudWidget::PaintTopBar(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const
{
	const FVector2D Canvas = FVector2D(Geometry.GetLocalSize());
	const float Scale = UiScale(Canvas);
	const float BarHeight = 84.0f * Scale;
	const int32 TextLayer = Layer + 1;

	PaintBox(Elements, Layer, Geometry, FVector2D::ZeroVector, FVector2D(Canvas.X, BarHeight), PanelColor);

	PaintBox(Elements, Layer, Geometry, FVector2D(0.0, BarHeight), FVector2D(Canvas.X, 4.0 * Scale), PhaseColor());

	const FSlateFontInfo SmallFont = PaintFont(FMath::RoundToInt(15 * Scale));
	const FSlateFontInfo MediumFont = PaintFont(FMath::RoundToInt(22 * Scale), true);

	PaintText(Elements, TextLayer, Geometry, FVector2D(24.0 * Scale, 14.0 * Scale),
		FString::Printf(TEXT("스테이지 %d"), StageNumber), MediumFont, TextColor);

	const FString Subtitle = bBossStage
		? FString::Printf(TEXT("%s · 보스 판"), *PhaseLabel())
		: PhaseLabel();

	PaintText(Elements, TextLayer, Geometry, FVector2D(24.0 * Scale, 47.0 * Scale), Subtitle, SmallFont, PhaseColor());

	const FSlateFontInfo ClockFont = PaintFont(FMath::RoundToInt(42 * Scale), true);
	const FString Clock = FormatClock(RemainingSeconds);
	const FVector2D ClockSize = MeasureText(Clock, ClockFont);

	const FLinearColor ClockColor = (Phase == ERollBallStagePhase::Normal) ? TextColor : PhaseColor();

	PaintText(Elements, TextLayer, Geometry,
		FVector2D((Canvas.X - ClockSize.X) * 0.5, 10.0 * Scale), Clock, ClockFont, ClockColor);

	const float BarWidth = FMath::Min(420.0f * Scale, static_cast<float>(Canvas.X) * 0.35f);
	const float Filled = LongestSeenSeconds > 0.0f
		? FMath::Clamp(RemainingSeconds / LongestSeenSeconds, 0.0f, 1.0f)
		: 0.0f;

	const FVector2D BarPos((Canvas.X - BarWidth) * 0.5, 64.0 * Scale);
	const FVector2D BarSize(BarWidth, 7.0 * Scale);
	PaintBox(Elements, TextLayer, Geometry, BarPos, BarSize, FLinearColor(1, 1, 1, 0.14f));
	PaintBox(Elements, TextLayer, Geometry, BarPos, FVector2D(BarWidth * Filled, BarSize.Y), PhaseColor());

	const FString GoldText = FString::Printf(TEXT("$ %d"), GoldEarned);
	const FString KillText = FString::Printf(TEXT("처치 %d"), KillCount);

	const FVector2D GoldSize = MeasureText(GoldText, MediumFont);
	const FVector2D KillSize = MeasureText(KillText, SmallFont);

	PaintText(Elements, TextLayer, Geometry,
		FVector2D(Canvas.X - GoldSize.X - 24.0 * Scale, 14.0 * Scale), GoldText, MediumFont, GoldColor);
	PaintText(Elements, TextLayer, Geometry,
		FVector2D(Canvas.X - KillSize.X - 24.0 * Scale, 47.0 * Scale), KillText, SmallFont, DimTextColor);
}

void URollBallHudWidget::PaintHealth(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const
{

	const float Scale = UiScale(FVector2D(Geometry.GetLocalSize()));
	const float Size = 26.0f * Scale;
	const float Gap = 8.0f * Scale;
	const FVector2D Start(24.0 * Scale, 104.0 * Scale);

	for (int32 i = 0; i < MaxHealth; ++i)
	{
		const FVector2D Position = Start + FVector2D((Size + Gap) * i, 0.0);
		const bool bFilled = i < Health;

		PaintPanel(Elements, Layer, Geometry, Position, FVector2D(Size, Size),
			bFilled ? HealthColor : FLinearColor(0.10f, 0.11f, 0.14f, 0.9f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.55f),
			FMath::Max(2.0f, 2.0f * Scale));
	}
}

void URollBallHudWidget::PaintResult(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const
{
	const FVector2D Canvas = FVector2D(Geometry.GetLocalSize());
	const int32 TextLayer = Layer + 1;

	PaintBox(Elements, Layer, Geometry, FVector2D::ZeroVector, Canvas, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));

	const bool bSurvived = (Result == ERollBallStageResult::Cleared);
	const FLinearColor Accent = bSurvived
		? FLinearColor(0.40f, 0.95f, 0.50f)
		: FLinearColor(0.95f, 0.35f, 0.35f);

	const float Scale = UiScale(Canvas);
	const FVector2D PanelSize(520.0 * Scale, 300.0 * Scale);
	const FVector2D PanelPos = (Canvas - PanelSize) * 0.5;

	PaintPanel(Elements, Layer, Geometry, PanelPos, PanelSize,
		FLinearColor(0.04f, 0.045f, 0.06f, 0.96f), Accent, 4.0f * Scale);

	const FSlateFontInfo TitleFont = PaintFont(FMath::RoundToInt(40 * Scale), true);
	const FSlateFontInfo BodyFont = PaintFont(FMath::RoundToInt(20 * Scale));
	const FSlateFontInfo SmallFont = PaintFont(FMath::RoundToInt(15 * Scale));

	PaintTextCentered(Elements, TextLayer, Geometry,
		PanelPos + FVector2D(0.0, 38.0 * Scale), FVector2D(PanelSize.X, 50.0 * Scale),
		bSurvived ? TEXT("생존") : TEXT("사망"), TitleFont, Accent);

	PaintTextCentered(Elements, TextLayer, Geometry,
		PanelPos + FVector2D(0.0, 104.0 * Scale), FVector2D(PanelSize.X, 30.0 * Scale),
		FString::Printf(TEXT("스테이지 %d"), StageNumber), BodyFont, TextColor);

	PaintTextCentered(Elements, TextLayer, Geometry,
		PanelPos + FVector2D(0.0, 148.0 * Scale), FVector2D(PanelSize.X, 30.0 * Scale),
		FString::Printf(TEXT("처치 %d"), KillCount), BodyFont, TextColor);

	PaintTextCentered(Elements, TextLayer, Geometry,
		PanelPos + FVector2D(0.0, 186.0 * Scale), FVector2D(PanelSize.X, 30.0 * Scale),
		FString::Printf(TEXT("얻은 골드 %d"), GoldEarned), BodyFont, GoldColor);

	PaintTextCentered(Elements, TextLayer, Geometry,
		PanelPos + FVector2D(0.0, 238.0 * Scale), FVector2D(PanelSize.X, 24.0 * Scale),
		bSurvived ? TEXT("골드는 이미 계정에 들어갔다. 곧 다음 스테이지")
		          : TEXT("골드는 이미 계정에 들어갔다. 곧 이 스테이지 다시"),
		SmallFont, DimTextColor);
}
