#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RollBallItemBase.generated.h"

UCLASS()
class ROLLBALL_API ARollBallItemBase : public AActor
{
	GENERATED_BODY()

public:

	ARollBallItemBase();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

public:

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintNativeEvent)
	void Collected();
};
