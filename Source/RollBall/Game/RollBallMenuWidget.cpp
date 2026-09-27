#include "RollBallMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

#include "RollBallGameInstance.h"
#include "RollBallMenuGameModeBase.h"
#include "RollBall/UI/RollBallSkillGraphWidget.h"

#define LOCTEXT_NAMESPACE "RollBallMenu"

URollBallMenuWidget::URollBallMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SkillTreeWidgetClass = URollBallSkillGraphWidget::StaticClass();

	// 버튼 밖 빈 곳을 누르면 계정 삭제 확인을 푼다. 그러려면 이 위젯이 클릭을 받아야 한다.
	SetVisibility(ESlateVisibility::Visible);
}

void URollBallMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	StartButton->OnClicked.AddDynamic(this, &URollBallMenuWidget::HandleStartClicked);
	SkillTreeButton->OnClicked.AddDynamic(this, &URollBallMenuWidget::HandleSkillTreeClicked);
	RebirthButton->OnClicked.AddDynamic(this, &URollBallMenuWidget::HandleRebirthClicked);
	DeleteButton->OnClicked.AddDynamic(this, &URollBallMenuWidget::HandleDeleteClicked);

	DeleteIdleStyle = DeleteButton->GetStyle();
	DeleteIdleLabelColor = DeleteLabel->GetColorAndOpacity();
	DeleteIdleLabel = DeleteLabel->GetText();
}

void URollBallMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (URollBallGameInstance* GameInstance = GetRollBallGameInstance())
	{
		GameInstance->EnsureSkillGraph();
	}

	SetDeleteArmed(false);
	RefreshTexts();
}

void URollBallMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshTexts();
}

FReply URollBallMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	SetDeleteArmed(false);
	return FReply::Handled();
}

void URollBallMenuWidget::RefreshTexts()
{
	const URollBallGameInstance* GameInstance = GetRollBallGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	const FString Account = FString::Printf(TEXT("골드 %d      최고 스테이지 %d      환생 %d회"),
		GameInstance->GetGold(), GameInstance->GetBestStage(), GameInstance->GetRebirthCount());

	if (Account != ShownAccount)
	{
		ShownAccount = Account;
		AccountText->SetText(FText::FromString(Account));
	}

	const FString Rebirth = FString::Printf(TEXT("환생  (+%d 골드)"), GameInstance->GetRebirthReward());

	if (Rebirth != ShownRebirth)
	{
		ShownRebirth = Rebirth;
		RebirthLabel->SetText(FText::FromString(Rebirth));
	}
}

void URollBallMenuWidget::SetDeleteArmed(bool bArmed)
{
	bDeleteArmed = bArmed;

	if (!bArmed)
	{
		DeleteButton->SetStyle(DeleteIdleStyle);
		DeleteLabel->SetColorAndOpacity(DeleteIdleLabelColor);
		DeleteLabel->SetText(DeleteIdleLabel);
		return;
	}

	FButtonStyle Danger = DeleteIdleStyle;
	Danger.Normal.OutlineSettings.Color = FSlateColor(DangerColor);
	Danger.Hovered.OutlineSettings.Color = FSlateColor(DangerColor);
	Danger.Pressed.OutlineSettings.Color = FSlateColor(DangerColor);

	DeleteButton->SetStyle(Danger);
	DeleteLabel->SetColorAndOpacity(FSlateColor(FMath::Lerp(DangerColor, FLinearColor::White, 0.35f)));
	DeleteLabel->SetText(LOCTEXT("DeleteConfirm", "정말 지운다. 한 번 더"));
}

void URollBallMenuWidget::HandleStartClicked()
{
	SetDeleteArmed(false);

	if (ARollBallMenuGameModeBase* MenuMode = Cast<ARollBallMenuGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		MenuMode->StartGame();
	}
	else if (URollBallGameInstance* GameInstance = GetRollBallGameInstance())
	{
		GameInstance->TravelToStage(1);
	}
}

void URollBallMenuWidget::HandleSkillTreeClicked()
{
	SetDeleteArmed(false);

	if (SkillTreeWidgetClass != nullptr)
	{
		if (UUserWidget* Tree = CreateWidget(GetWorld(), SkillTreeWidgetClass))
		{
			Tree->AddToViewport(10);
		}
	}
}

void URollBallMenuWidget::HandleRebirthClicked()
{
	SetDeleteArmed(false);

	if (URollBallGameInstance* GameInstance = GetRollBallGameInstance())
	{
		GameInstance->Rebirth();
	}
}

void URollBallMenuWidget::HandleDeleteClicked()
{
	if (!bDeleteArmed)
	{
		SetDeleteArmed(true);
		return;
	}

	SetDeleteArmed(false);

	if (URollBallGameInstance* GameInstance = GetRollBallGameInstance())
	{
		GameInstance->DeleteAccount();
	}

	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

URollBallGameInstance* URollBallMenuWidget::GetRollBallGameInstance() const
{
	return GetGameInstance<URollBallGameInstance>();
}

#undef LOCTEXT_NAMESPACE
