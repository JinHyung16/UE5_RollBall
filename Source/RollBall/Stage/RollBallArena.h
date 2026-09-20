#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RollBallArena.generated.h"

class UStaticMeshComponent;

UCLASS()
class ROLLBALL_API ARollBallArena : public AActor
{
	GENERATED_BODY()

public:
	ARollBallArena();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Arena")
	void Build(int32 Seed);

	UFUNCTION(BlueprintPure, Category = "RollBall|Arena")
	float GetArenaRadius() const { return ArenaRadius; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<UStaticMeshComponent*> Boxes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나")
	bool bBuildOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나", meta = (ClampMin = "500"))
	float ArenaRadius = 4200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나", meta = (ClampMin = "100"))
	float CentreHalfSize = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나", meta = (ClampMin = "1"))
	int32 PlateCount = 9;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나", meta = (ClampMin = "10"))
	float StepHeight = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나", meta = (ClampMin = "1"))
	int32 MaxStep = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나", meta = (ClampMin = "50"))
	float PlateHalfMin = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나", meta = (ClampMin = "50"))
	float PlateHalfMax = 620.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나", meta = (ClampMin = "10"))
	float WallHeight = 400.0f;

private:

	static constexpr float CubeSize = 100.0f;

	enum class EBoxKind : uint8
	{
		Floor,
		Ramp,
		Wall,
	};

	UStaticMeshComponent* AddBox(EBoxKind Kind, const FVector& Centre,
		const FVector& Extent, const FRotator& Rotation);

	void SpawnLightingIfMissing();

	void AddFloorBox(const FVector& Centre, const FVector& Extent);
	void AddWallBox(const FVector& Centre, const FVector& Extent);

	void AddRamp(const FVector& From, const FVector& To, float Width);
};
