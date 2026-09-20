#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RollBall/Skill/RollBallSkillTypes.h"
#include "RollBallWeaponComponent.generated.h"

class ARollBallEnemy;
class UStaticMeshComponent;

UCLASS(ClassGroup = (RollBall), meta = (BlueprintSpawnableComponent))
class ROLLBALL_API URollBallWeaponComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	URollBallWeaponComponent();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Weapon")
	void RebuildFromStats(const FRollBallSkillStats& Stats);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "무기", meta = (ClampMin = "10"))
	float HammerRadius = 190.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "무기", meta = (ClampMin = "10"))
	float FrontOffset = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "무기", meta = (ClampMin = "0.02"))
	float DamageInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "무기", meta = (ClampMin = "0"))
	float MinAimSpeed = 40.0f;

private:

	UPROPERTY()
	TArray<UStaticMeshComponent*> HammerHeads;

	UPROPERTY()
	UStaticMeshComponent* Saw = nullptr;

	UPROPERTY()
	UStaticMeshComponent* Bulldozer = nullptr;

	UPROPERTY()
	UStaticMeshComponent* DrillFront = nullptr;

	UPROPERTY()
	UStaticMeshComponent* DrillTop = nullptr;

	float HammerSpinSpeed = 90.0f;
	float HammerDamage = 1.0f;
	float HammerKnockback = 1.0f;

	float SawDamage = 1.0f;
	float DrillDamage = 1.0f;
	float DrillPierce = 0.0f;
	float BulldozerPush = 1.0f;

	float HammerAngle = 0.0f;

	FVector AimDirection = FVector::ForwardVector;

	TMap<TPair<uint32, uint32>, float> NextDamageTime;

	void ClearParts();

	UStaticMeshComponent* CreatePart(const TCHAR* MeshPath, const FLinearColor& Color, FName Name);

	void UpdateAim(float DeltaTime);
	void UpdateHammers(float DeltaTime);

	void StrikeWithPart(UStaticMeshComponent* Part, float Damage, float PushStrength);

	bool CanDamageNow(const UStaticMeshComponent* Part, const AActor* Enemy) const;
	void MarkDamaged(const UStaticMeshComponent* Part, const AActor* Enemy);

	void ForgetDeadEntries();
};
