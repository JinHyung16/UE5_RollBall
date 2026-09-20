#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RollBallMenuWidget.generated.h"

class URollBallGameInstance;

UENUM()
enum class ERollBallMenuItem : uint8
{
	None,
	StartGame,
	SkillTree,
	Rebirth,
	DeleteAccount,
};

UCLASS()
class ROLLBALL_API URollBallMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URollBallMenuWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Menu")
	void RequestStartGame(const FString& Nickname);

protected:
	virtual void NativeConstruct() override;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "메뉴")
	FText GameTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "메뉴")
	TSubclassOf<UUserWidget> SkillTreeWidgetClass;

private:

	bool bDeleteArmed = false;

	ERollBallMenuItem HoveredItem = ERollBallMenuItem::None;

	FBox2D ItemBox(ERollBallMenuItem Item, const FVector2D& Canvas) const;

	ERollBallMenuItem HitTest(const FVector2D& Local, const FVector2D& Canvas) const;

	FString LabelOf(ERollBallMenuItem Item) const;

	void Activate(ERollBallMenuItem Item);

	URollBallGameInstance* GetRollBallGameInstance() const;
};
