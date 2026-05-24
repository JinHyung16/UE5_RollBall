// Place these in each stage level wherever an item could spawn. The GameMode
// picks N of them (RequiredItemCount) at random and spawns items at runtime.

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

	// Optional weight if you want to bias random selection. Default 1.0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RollBall")
	float SelectionWeight = 1.0f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBillboardComponent* Billboard;
};
