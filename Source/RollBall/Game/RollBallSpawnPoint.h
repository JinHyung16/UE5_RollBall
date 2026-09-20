#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RollBallSpawnPoint.generated.h"

UCLASS()
class ROLLBALL_API ARollBallSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ARollBallSpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall")
	float SelectionWeight = 1.0f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBillboardComponent* Billboard;
};
