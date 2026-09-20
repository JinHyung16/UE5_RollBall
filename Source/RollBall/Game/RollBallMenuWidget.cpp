#include "RollBallMenuWidget.h"

#include "Kismet/GameplayStatics.h"

#include "RollBallGameInstance.h"
#include "RollBallMenuGameModeBase.h"
#include "RollBall/UI/RollBallPaint.h"
#include "RollBall/UI/RollBallSkillGraphWidget.h"

using namespace RollBallPaint;

#define LOCTEXT_NAMESPACE "RollBallMenu"

namespace
{
	const ERollBallMenuItem MenuOrder[] = {
		ERollBallMenuItem::StartGame,
		ERollBallMenuItem::SkillTree,
		ERollBallMenuItem::Rebirth,
		ERollBallMenuItem::DeleteAccount,
	};

	constexpr float ButtonWidth = 420.0f;
	constexpr float ButtonHeight = 66.0f;
	constexpr float ButtonGap = 16.0f;

	constexpr float TitleHeight = 92.0f;
	constexpr float AccountHeight = 34.0f;
	constexpr float BlockGap = 46.0f;

	float ScaleFor(const FVector2D& Canvas)
	{
		return FMath::Clamp(static_cast<float>(Canvas.Y) / 720.0f, 0.7f, 2.5f);
	}

	float BlockHeight(float Scale)
	{
		const int32 Count = 4;
		return (TitleHeight + AccountHeight + BlockGap
			+ Count * ButtonHeight + (Count - 1) * ButtonGap) * Scale;
	}

	float BlockTop(const FVector2D& Canvas)
	{
		return (static_cast<float>(Canvas.Y) - BlockHeight(ScaleFor(Canvas))) * 0.5f;
	}
}

URollBallMenuWidget::URollBallMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameTitle = LOCTEXT("GameTitle", "ROLL BALL");
	SkillTreeWidgetClass = URollBallSkillGraphWidget::StaticClass();

	SetVisibility(ESlateVisibility::Visible);
}

void URollBallMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (URollBallGameInstance* GameInstance = GetRollBallGameInstance())
	{
		GameInstance->EnsureSkillGraph();
	}
}

void URollBallMenuWidget::RequestStartGame(const FString& Nickname)
{
	if (URollBallGameInstance* GameInstance = GetRollBallGameInstance())
	{
		GameInstance->PlayerNickname = Nickname;
	}

	Activate(ERollBallMenuItem::StartGame);
}

FBox2D URollBallMenuWidget::ItemBox(ERollBallMenuItem Item, const FVector2D& Canvas) const
{
	const float Scale = ScaleFor(Canvas);
	const float FirstButtonY = BlockTop(Canvas) + (TitleHeight + AccountHeight + BlockGap) * Scale;

	for (int32 i = 0; i < UE_ARRAY_COUNT(MenuOrder); ++i)
	{
		if (MenuOrder[i] != Item)
		{
			continue;
		}

		const FVector2D Size(ButtonWidth * Scale, ButtonHeight * Scale);
		const FVector2D Position(
			(Canvas.X - Size.X) * 0.5,
			FirstButtonY + (ButtonHeight + ButtonGap) * Scale * i);

		return FBox2D(Position, Position + Size);
	}

	return FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
}

ERollBallMenuItem URollBallMenuWidget::HitTest(const FVector2D& Local, const FVector2D& Canvas) const
{
	for (const ERollBallMenuItem Item : MenuOrder)
	{
		if (ItemBox(Item, Canvas).IsInside(Local))
		{
			return Item;
		}
	}
	return ERollBallMenuItem::None;
}

FString URollBallMenuWidget::LabelOf(ERollBallMenuItem Item) const
{
	const URollBallGameInstance* GameInstance = GetRollBallGameInstance();

	switch (Item)
	{
	case ERollBallMenuItem::StartGame:
		return TEXT("게임 시작");

	case ERollBallMenuItem::SkillTree:
		return TEXT("스킬 트리");

	case ERollBallMenuItem::Rebirth:
		return GameInstance != nullptr
			? FString::Printf(TEXT("환생  (+%d 골드)"), GameInstance->GetRebirthReward())
			: TEXT("환생");

	case ERollBallMenuItem::DeleteAccount:
		return bDeleteArmed ? TEXT("정말 지운다. 한 번 더") : TEXT("계정 삭제");

	default:
		return FString();
	}
}

int32 URollBallMenuWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Canvas = FVector2D(AllottedGeometry.GetLocalSize());
	const int32 TextLayer = LayerId + 1;

	PaintBox(OutDrawElements, LayerId, AllottedGeometry, FVector2D::ZeroVector, Canvas,
		FLinearColor(0.035f, 0.04f, 0.055f, 1.0f));

	const float Scale = ScaleFor(Canvas);
	const float Top = BlockTop(Canvas);

	const FSlateFontInfo TitleFont = PaintFont(FMath::RoundToInt(64 * Scale), true, 2);
	PaintTextCentered(OutDrawElements, TextLayer, AllottedGeometry,
		FVector2D(0.0, Top), FVector2D(Canvas.X, TitleHeight * Scale),
		GameTitle.ToString(), TitleFont, FLinearColor(0.95f, 0.96f, 1.0f));

	const URollBallGameInstance* GameInstance = GetRollBallGameInstance();
	const FSlateFontInfo SmallFont = PaintFont(FMath::RoundToInt(18 * Scale));

	if (GameInstance != nullptr)
	{
		const FString Account = FString::Printf(
			TEXT("골드 %d      최고 스테이지 %d      환생 %d회"),
			GameInstance->GetGold(),
			GameInstance->GetBestStage(),
			GameInstance->GetRebirthCount());

		PaintTextCentered(OutDrawElements, TextLayer, AllottedGeometry,
			FVector2D(0.0, Top + TitleHeight * Scale), FVector2D(Canvas.X, AccountHeight * Scale),
			Account, SmallFont, FLinearColor(0.62f, 0.68f, 0.80f));
	}

	const FSlateFontInfo ButtonFont = PaintFont(FMath::RoundToInt(26 * Scale), true);

	for (const ERollBallMenuItem Item : MenuOrder)
	{
		const FBox2D BoxRect = ItemBox(Item, Canvas);
		const FVector2D Size = BoxRect.Max - BoxRect.Min;
		const bool bHovered = (Item == HoveredItem);

		const bool bDanger = (Item == ERollBallMenuItem::DeleteAccount) && bDeleteArmed;

		const FLinearColor Border = bDanger
			? FLinearColor(0.95f, 0.30f, 0.30f)
			: (bHovered ? FLinearColor(0.95f, 0.96f, 1.0f) : FLinearColor(0.26f, 0.30f, 0.38f));

		const FLinearColor Fill = bHovered
			? FLinearColor(0.12f, 0.14f, 0.18f)
			: FLinearColor(0.07f, 0.08f, 0.11f);

		PaintPanel(OutDrawElements, LayerId, AllottedGeometry, BoxRect.Min, Size, Fill, Border,
			FMath::Max(2.0f, 2.5f * Scale));

		PaintTextCentered(OutDrawElements, TextLayer, AllottedGeometry, BoxRect.Min, Size,
			LabelOf(Item), ButtonFont,
			bDanger ? FLinearColor(1.0f, 0.55f, 0.55f) : FLinearColor(0.90f, 0.93f, 0.98f));
	}

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId + 4, InWidgetStyle, bParentEnabled);
}

FReply URollBallMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	const FVector2D Local = FVector2D(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));
	const ERollBallMenuItem Item = HitTest(Local, FVector2D(InGeometry.GetLocalSize()));

	if (Item == ERollBallMenuItem::None)
	{

		bDeleteArmed = false;
		return FReply::Handled();
	}

	Activate(Item);
	return FReply::Handled();
}

FReply URollBallMenuWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D Local = FVector2D(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));
	HoveredItem = HitTest(Local, FVector2D(InGeometry.GetLocalSize()));
	return FReply::Handled();
}

void URollBallMenuWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	HoveredItem = ERollBallMenuItem::None;
}

void URollBallMenuWidget::Activate(ERollBallMenuItem Item)
{
	URollBallGameInstance* GameInstance = GetRollBallGameInstance();

	if (Item != ERollBallMenuItem::DeleteAccount)
	{
		bDeleteArmed = false;
	}

	switch (Item)
	{
	case ERollBallMenuItem::StartGame:
	{
		if (ARollBallMenuGameModeBase* MenuMode =
			Cast<ARollBallMenuGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			MenuMode->StartGame(GameInstance != nullptr ? GameInstance->PlayerNickname : FString());
		}
		else if (GameInstance != nullptr)
		{
			GameInstance->TravelToStage(1);
		}
		break;
	}

	case ERollBallMenuItem::SkillTree:
	{
		if (SkillTreeWidgetClass != nullptr)
		{
			if (UUserWidget* Tree = CreateWidget(GetWorld(), SkillTreeWidgetClass))
			{
				Tree->AddToViewport(10);
			}
		}
		break;
	}

	case ERollBallMenuItem::Rebirth:
	{
		if (GameInstance != nullptr)
		{
			GameInstance->Rebirth();
		}
		break;
	}

	case ERollBallMenuItem::DeleteAccount:
	{
		if (!bDeleteArmed)
		{
			bDeleteArmed = true;
			break;
		}

		bDeleteArmed = false;

		if (GameInstance != nullptr)
		{
			GameInstance->DeleteAccount();
		}

		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
		break;
	}

	default:
		break;
	}
}

URollBallGameInstance* URollBallMenuWidget::GetRollBallGameInstance() const
{
	return GetGameInstance<URollBallGameInstance>();
}

#undef LOCTEXT_NAMESPACE
