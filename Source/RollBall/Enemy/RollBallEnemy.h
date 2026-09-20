#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RollBallEnemy.generated.h"

class ARollBallPlayer;
class UMaterialInstanceDynamic;

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
	void Setup(ERollBallEnemyKind InKind, float HealthScale, float SpeedScale, int32 InGoldReward, float SizeScale);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Enemy")
	bool ApplyWeaponDamage(float Amount, AActor* Causer);

	UFUNCTION(BlueprintPure, Category = "RollBall|Enemy")
	ERollBallEnemyKind GetKind() const { return Kind; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Enemy")
	bool IsAlive() const { return Health > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Enemy")
	float GetDespawnDistance() const { return DespawnDistance; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	ERollBallEnemyKind Kind = ERollBallEnemyKind::Chaser;

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
	float DespawnDistance = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	float PushImpulse = 90000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	float MeteorDropHeight = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "적")
	float MeteorFallSpeed = 1400.0f;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* MeshMaterial = nullptr;

	FVector MeteorTargetZ = FVector::ZeroVector;

	bool bDying = false;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void ConfigureForKind(float SizeScale);

	void TouchPlayer(ARollBallPlayer* Player, const FVector& FromDirection);

	void Die(AActor* Causer);

	ARollBallPlayer* FindPlayer() const;
};
