#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "RollBallMenuWidget.generated.h"

class UButton;
class UTextBlock;
class URollBallGameInstance;

/**
 * 메인 메뉴의 동작 부분. 화면 배치와 모양은 /Game/UI/WBP_MainMenu 디자이너에서 고친다.
 * 아래 BindWidget 변수와 이름이 같은 위젯이 WBP 에 있어야 컴파일된다.
 */
UCLASS()
class ROLLBALL_API URollBallMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URollBallMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 스킬 트리 버튼이 여는 위젯. WBP_MainMenu 의 클래스 기본값에서 WBP_SkillTree 로 지정돼 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "메뉴")
	TSubclassOf<UUserWidget> SkillTreeWidgetClass;

	/** 계정 삭제를 한 번 누른 상태의 버튼 테두리·글자색 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "메뉴")
	FLinearColor DangerColor = FLinearColor(0.95f, 0.30f, 0.30f);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AccountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SkillTreeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RebirthButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RebirthLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DeleteButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DeleteLabel;

private:
	bool bDeleteArmed = false;

	/** 디자이너에서 잡아 둔 계정 삭제 버튼 모양. 위험 표시를 풀 때 되돌린다 */
	FButtonStyle DeleteIdleStyle;
	FSlateColor DeleteIdleLabelColor;
	FText DeleteIdleLabel;

	FString ShownAccount;
	FString ShownRebirth;

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleSkillTreeClicked();

	UFUNCTION()
	void HandleRebirthClicked();

	UFUNCTION()
	void HandleDeleteClicked();

	void SetDeleteArmed(bool bArmed);

	/** 골드·최고 스테이지·환생 보상은 스킬 트리나 환생으로 바뀌므로 매 틱 비교해서 바뀐 것만 고친다 */
	void RefreshTexts();

	URollBallGameInstance* GetRollBallGameInstance() const;
};
