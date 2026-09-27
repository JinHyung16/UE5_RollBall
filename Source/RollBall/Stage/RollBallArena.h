#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RollBallArena.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
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

	/** 바위 인스턴스에 붙는 컴포넌트 태그. 적 스폰 위치가 바위 위로 잡히는 걸 거를 때 쓴다. */
	static const FName RockTag;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나|바위")
	bool bPlaceRocks = true;

	/** 벽을 따라 늘어놓는 큰 바위 사이 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나|바위", meta = (ClampMin = "100"))
	float BorderRockSpacing = 480.0f;

	/** 바닥에 놓는 장애물 바위 수. 스테이지 시드로 이 범위에서 고른다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나|바위", meta = (ClampMin = "0"))
	int32 ObstacleRockMin = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나|바위", meta = (ClampMin = "0"))
	int32 ObstacleRockMax = 12;

	/** 장애물 바위끼리, 그리고 발판·경사로와 떨어뜨리는 거리. 공이 끼는 틈을 막는다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나|바위", meta = (ClampMin = "0"))
	float ObstacleClearance = 450.0f;

	/** 충돌 없는 장식용 자갈 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "아레나|바위", meta = (ClampMin = "0"))
	int32 PebbleCount = 45;

private:
	static constexpr float CubeSize = 100.0f;

	struct FFootprint
	{
		FVector2D Min;
		FVector2D Max;
	};

	struct FRampFootprint
	{
		FVector2D From;
		FVector2D To;
		float HalfWidth = 0.0f;
	};

	struct FRockPick
	{
		UStaticMesh* Mesh = nullptr;
		float Scale = 1.0f;
		float Radius = 0.0f;
		float Height = 0.0f;
	};

	/** 벽을 두르는 큰 바위 */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> BorderRocks;

	/** 바닥에 놓는 장애물 바위. 공보다 두세 배 큰 정도로 줄여 쓴다 */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> ObstacleRocks;

	/** 충돌 없는 자갈 */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> PebbleRocks;

	UPROPERTY()
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> SolidRockInstances;

	UPROPERTY()
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> DecorRockInstances;

	TArray<FFootprint> PlateFootprints;
	TArray<FRampFootprint> RampFootprints;

	void ClearRocks();
	void PlaceBorderRocks(FRandomStream& Stream);
	void PlaceObstacleRocks(FRandomStream& Stream, TArray<FVector>& OutObstacles);
	void PlacePebbles(FRandomStream& Stream, const TArray<FVector>& Obstacles);

	/** 바위 하나를 고르고 크기를 정한다. 발 넓이는 TargetRadius, 높이는 MaxHeight 를 넘지 않는다 */
	FRockPick PickRock(FRandomStream& Stream, const TArray<TObjectPtr<UStaticMesh>>& Meshes,
		float TargetRadius, float MaxHeight) const;

	/** 바닥(z=0)에 바위를 놓는다. 메시 피벗은 바닥 중앙이다 */
	void AddRock(FRandomStream& Stream, const FRockPick& Pick, const FVector2D& Position, bool bCollision);

	UInstancedStaticMeshComponent* GetRockComponent(UStaticMesh* Mesh, bool bCollision);

	/** 발판이나 경사로와 Radius 안쪽으로 겹치면 true */
	bool IsBlockedByLayout(const FVector2D& Position, float Radius) const;

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
