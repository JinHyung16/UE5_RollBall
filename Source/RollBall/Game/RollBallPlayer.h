#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RollBallPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class URollBallWeaponComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRollBallHealthChanged, int32, Health, int32, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRollBallPlayerDied);

UCLASS()
class ROLLBALL_API ARollBallPlayer : public APawn
{
	GENERATED_BODY()

public:

	ARollBallPlayer();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	URollBallWeaponComponent* Weapons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveForce = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpImpulse = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxJumpCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RollBall|Stats")
	int32 MaxHealth = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RollBall|Stats")
	int32 Health = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RollBall|Stats")
	float MaxSpeed = 1200.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RollBall|Stats")
	float KnockbackResist = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RollBall|Stats", meta = (ClampMin = "0"))
	float InvulnerableSeconds = 0.8f;

public:
	UPROPERTY(BlueprintAssignable, Category = "RollBall|Stats")
	FRollBallHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "RollBall|Stats")
	FRollBallPlayerDied OnDied;

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stats")
	void ApplySkillStats();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stats")
	void ApplyKnockback(const FVector& Impulse);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Stats")
	void TakeHit(int32 Damage, AActor* Causer);

	UFUNCTION(BlueprintPure, Category = "RollBall|Stats")
	int32 GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Stats")
	int32 GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Stats")
	bool IsAlive() const { return Health > 0; }

	UFUNCTION(BlueprintPure, Category = "RollBall|Stats")
	bool IsInvulnerable() const;

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	int32 JumpCount = 0;

	float InvulnerableUntil = 0.0f;

	UPROPERTY(EditAnywhere, Category = "RollBall|Debug")
	float AutoPlayAwareness = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "RollBall|Debug")
	float AutoPlayHomeRadius = 2000.0f;

	void TickAutoPlay(float DeltaTime);

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Jump();

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
