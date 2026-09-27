#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RollBallEnemy.generated.h"

class ARollBallPlayer;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

UENUM(BlueprintType)
enum class ERollBallEnemyKind : uint8
{
	Chaser  UMETA(DisplayName = "돌진 원통"),
	Roller  UMETA(DisplayName = "굴러오는 공"),
	Blocker UMETA(DisplayName = "나무"),
	Meteor  UMETA(DisplayName = "운석"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRollBallEnemyDied, ARollBallEnemy*, Enemy, int32, GoldReward);

UCLASS()
class ROLLBALL_API ARollBallEnemy : public AActor
{
	GENERATED_BODY()

public:
	ARollBallEnemy();

	UPROPERTY(BlueprintAssignable, Category = "RollBall|Enemy")
	FRollBallEnemyDied OnDied;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Enemy")
	void Activate(ERollBallEnemyKind InKind, float HealthScale, float SpeedScale, int32 InGoldReward,
		float SizeScale, const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Enemy")
	void Deactivate();

	UFUNCTION(BlueprintPure, Category = "RollBall|Enemy")
	bool IsActive() const { return bActive; }

	void SetAutoActivate(bool bInAutoActivate) { bAutoActivate = bInAutoActivate; }

	UFUNCTION(BlueprintCallable, Category = "RollBall|Enemy")
	bool ApplyWeaponDamage(float Amount, AActor* Causer);

	UFUNCTION(BlueprintPure, Category = "RollBall|Enemy")
	bool IsAlive() const { return bActive && Health > 0.0f; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	ERollBallEnemyKind Kind = ERollBallEnemyKind::Chaser;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	bool bAutoActivate = true;

	UPROPERTY(BlueprintReadOnly, Category = "적")
	float Health = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "적")
	float MaxHealth = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "적")
	float MoveSpeed = 300.0f;

	UPROPERTY(BlueprintReadOnly, Category = "적")
	int32 ContactDamage = 1;

	UPROPERTY(BlueprintReadOnly, Category = "적")
	int32 GoldReward = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	float PushImpulse = 90000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	float MeteorDropHeight = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	float MeteorFallSpeed = 1400.0f;

private:
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> KindMeshes;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MeshMaterial = nullptr;

	/** 운석용 Fab 바위 머티리얼. 없으면 운석도 기본 도형 머티리얼을 쓴다 */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> RockMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MeteorMaterial = nullptr;

	FRotator MeteorSpin = FRotator::ZeroRotator;

	TWeakObjectPtr<ARollBallPlayer> CachedPlayer;

	FVector MeteorTargetZ = FVector::ZeroVector;

	bool bActive = false;
	bool bDying = false;
	bool bHasShape = false;
	ERollBallEnemyKind ShapeKind = ERollBallEnemyKind::Chaser;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void ApplyStatsForKind(float HealthScale, float SpeedScale);
	void ApplyShapeForKind(float SizeScale);
	void ApplyCollisionForKind();

	void TouchPlayer(ARollBallPlayer* Player, const FVector& FromDirection);

	void Die(AActor* Causer);

	ARollBallPlayer* FindPlayer();
};
