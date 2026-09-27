#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RollBallWidgetBuilderCommandlet.generated.h"

/**
 * /Game/UI 의 WBP_MainMenu · WBP_Hud · WBP_SkillTree 초안을 만드는 도구.
 * 예전에 NativePaint 로 그리던 배치를 그대로 옮긴다. 만든 뒤에는 디자이너에서 고치면 된다.
 *
 * 이미 있는 에셋은 건드리지 않는다. -force 를 주면 트리를 비우고 다시 짠다(디자이너에서 고친 배치가 사라진다).
 *
 *   UnrealEditor-Cmd.exe RollBall.uproject -run=RollBallWidgetBuilder [-force]
 */
UCLASS()
class URollBallWidgetBuilderCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URollBallWidgetBuilderCommandlet();

	virtual int32 Main(const FString& Params) override;
};
