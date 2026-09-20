#include "RollBallWeaponComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "RollBall/Enemy/RollBallEnemy.h"

URollBallWeaponComponent::URollBallWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetUsingAbsoluteRotation(true);
}

void URollBallWeaponComponent::RebuildFromStats(const FRollBallSkillStats& Stats)
{
	ClearParts();

	HammerSpinSpeed = Stats.Get(ERollBallSkillStat::HammerSpinSpeed);
	HammerDamage = Stats.Get(ERollBallSkillStat::HammerDamage);
	HammerKnockback = Stats.Get(ERollBallSkillStat::HammerKnockback);
	SawDamage = Stats.Get(ERollBallSkillStat::SawDamage);
	DrillDamage = Stats.Get(ERollBallSkillStat::DrillDamage);
	DrillPierce = Stats.Get(ERollBallSkillStat::DrillPierce);
	BulldozerPush = Stats.Get(ERollBallSkillStat::BulldozerPush);

	if (Stats.IsUnlocked(ERollBallSkillStat::HammerUnlock))
	{
		const int32 Count = FMath::Max(1, Stats.GetInt(ERollBallSkillStat::HammerCount));

		for (int32 i = 0; i < Count; ++i)
		{
			UStaticMeshComponent* Head = CreatePart(
				TEXT("/Engine/BasicShapes/Cube.Cube"),
				FLinearColor(0.85f, 0.80f, 0.35f),
				*FString::Printf(TEXT("Hammer_%d"), i));

			if (Head != nullptr)
			{
				Head->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.45f));
				HammerHeads.Add(Head);
			}
		}
	}

	if (Stats.IsUnlocked(ERollBallSkillStat::SawUnlock))
	{
		Saw = CreatePart(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
			FLinearColor(0.85f, 0.85f, 0.90f), TEXT("Saw"));

		if (Saw != nullptr)
		{

			const float Size = Stats.Get(ERollBallSkillStat::SawSize);
			Saw->SetRelativeScale3D(FVector(Size * 0.9f, Size * 0.15f, Size * 0.9f));
			Saw->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
		}
	}

	if (Stats.IsUnlocked(ERollBallSkillStat::BulldozerUnlock))
	{
		Bulldozer = CreatePart(TEXT("/Engine/BasicShapes/Cube.Cube"),
			FLinearColor(0.95f, 0.70f, 0.20f), TEXT("Bulldozer"));

		if (Bulldozer != nullptr)
		{
			const float Width = Stats.Get(ERollBallSkillStat::BulldozerWidth);
			Bulldozer->SetRelativeScale3D(FVector(0.18f, Width * 1.3f, 0.7f));
		}
	}

	if (Stats.IsUnlocked(ERollBallSkillStat::DrillUnlock))
	{
		DrillFront = CreatePart(TEXT("/Engine/BasicShapes/Cone.Cone"),
			FLinearColor(0.70f, 0.35f, 0.90f), TEXT("DrillFront"));

		if (DrillFront != nullptr)
		{
			DrillFront->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));
			DrillFront->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		}

		if (Stats.IsUnlocked(ERollBallSkillStat::DrillTopMount))
		{
			DrillTop = CreatePart(TEXT("/Engine/BasicShapes/Cone.Cone"),
				FLinearColor(0.70f, 0.35f, 0.90f), TEXT("DrillTop"));

			if (DrillTop != nullptr)
			{
				DrillTop->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));
				DrillTop->SetRelativeLocation(FVector(0.0f, 0.0f, FrontOffset));
			}
		}
	}
}

UStaticMeshComponent* URollBallWeaponComponent::CreatePart(const TCHAR* MeshPath,
	const FLinearColor& Color, FName Name)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	const FName UniqueName = MakeUniqueObjectName(Owner, UStaticMeshComponent::StaticClass(), Name);

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner, UniqueName);
	if (Part == nullptr)
	{
		return nullptr;
	}

	Part->SetupAttachment(this);
	Part->RegisterComponent();

	if (UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		Part->SetStaticMesh(Shape);
	}

	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		Part->SetMaterial(0, Material);
	}

	if (UMaterialInstanceDynamic* Dynamic = Part->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dynamic->SetVectorParameterValue(TEXT("Color"), Color);
	}

	Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Part->SetCollisionObjectType(ECC_WorldDynamic);
	Part->SetCollisionResponseToAllChannels(ECR_Overlap);
	Part->SetGenerateOverlapEvents(true);

	return Part;
}

void URollBallWeaponComponent::ClearParts()
{
	for (UStaticMeshComponent* Head : HammerHeads)
	{
		if (IsValid(Head))
		{
			Head->DestroyComponent();
		}
	}
	HammerHeads.Reset();

	UStaticMeshComponent* Mounted[] = { Saw, Bulldozer, DrillFront, DrillTop };
	for (UStaticMeshComponent* Part : Mounted)
	{
		if (IsValid(Part))
		{
			Part->DestroyComponent();
		}
	}

	Saw = nullptr;
	Bulldozer = nullptr;
	DrillFront = nullptr;
	DrillTop = nullptr;

	NextDamageTime.Reset();
}

void URollBallWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateAim(DeltaTime);
	UpdateHammers(DeltaTime);

	for (UStaticMeshComponent* Head : HammerHeads)
	{
		StrikeWithPart(Head, HammerDamage, 60000.0f * HammerKnockback);
	}

	StrikeWithPart(Saw, SawDamage, 0.0f);
	StrikeWithPart(DrillFront, DrillDamage + DrillPierce, 0.0f);
	StrikeWithPart(DrillTop, DrillDamage + DrillPierce, 0.0f);

	StrikeWithPart(Bulldozer, 0.0f, 45000.0f * BulldozerPush);
}

void URollBallWeaponComponent::UpdateAim(float DeltaTime)
{
	const AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	const FVector Velocity = Owner->GetVelocity();
	const FVector Flat(Velocity.X, Velocity.Y, 0.0f);

	if (Flat.Size() >= MinAimSpeed)
	{
		AimDirection = Flat.GetSafeNormal();
	}

	SetWorldRotation(AimDirection.Rotation());

	const FVector Front = AimDirection * FrontOffset;

	if (IsValid(Saw))
	{
		Saw->SetWorldLocation(GetComponentLocation() + Front);
	}
	if (IsValid(Bulldozer))
	{
		Bulldozer->SetWorldLocation(GetComponentLocation() + Front);
	}
	if (IsValid(DrillFront))
	{
		DrillFront->SetWorldLocation(GetComponentLocation() + Front);
	}
}

void URollBallWeaponComponent::UpdateHammers(float DeltaTime)
{
	if (HammerHeads.Num() == 0)
	{
		return;
	}

	HammerAngle = FMath::Fmod(HammerAngle + HammerSpinSpeed * DeltaTime, 360.0f);

	const float Step = 360.0f / HammerHeads.Num();
	const FVector Centre = GetComponentLocation();

	for (int32 i = 0; i < HammerHeads.Num(); ++i)
	{
		UStaticMeshComponent* Head = HammerHeads[i];
		if (!IsValid(Head))
		{
			continue;
		}

		const float Radians = FMath::DegreesToRadians(HammerAngle + Step * i);
		const FVector Offset(FMath::Cos(Radians) * HammerRadius, FMath::Sin(Radians) * HammerRadius, 0.0f);

		Head->SetWorldLocation(Centre + Offset);
	}
}

void URollBallWeaponComponent::StrikeWithPart(UStaticMeshComponent* Part, float Damage, float PushStrength)
{
	if (!IsValid(Part))
	{
		return;
	}

	TArray<AActor*> Touching;
	Part->GetOverlappingActors(Touching, ARollBallEnemy::StaticClass());

	for (AActor* Actor : Touching)
	{
		ARollBallEnemy* Enemy = Cast<ARollBallEnemy>(Actor);
		if (Enemy == nullptr || !Enemy->IsAlive() || !CanDamageNow(Part, Enemy))
		{
			continue;
		}

		MarkDamaged(Part, Enemy);

		if (PushStrength > 0.0f)
		{

			const FVector Away = (Enemy->GetActorLocation() - GetComponentLocation()).GetSafeNormal();
			if (UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Enemy->GetRootComponent()))
			{
				if (Body->IsSimulatingPhysics())
				{
					Body->AddImpulse(Away * PushStrength);
				}
			}
		}

		if (Damage > 0.0f)
		{
			Enemy->ApplyWeaponDamage(Damage, GetOwner());
		}
	}
}

namespace
{

	TPair<uint32, uint32> HitKey(const UStaticMeshComponent* Part, const AActor* Enemy)
	{
		return TPair<uint32, uint32>(Part->GetUniqueID(), Enemy->GetUniqueID());
	}
}

bool URollBallWeaponComponent::CanDamageNow(const UStaticMeshComponent* Part, const AActor* Enemy) const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const float* Next = NextDamageTime.Find(HitKey(Part, Enemy));
	return Next == nullptr || World->GetTimeSeconds() >= *Next;
}

void URollBallWeaponComponent::MarkDamaged(const UStaticMeshComponent* Part, const AActor* Enemy)
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	NextDamageTime.Add(HitKey(Part, Enemy), World->GetTimeSeconds() + DamageInterval);

	if (NextDamageTime.Num() > 512)
	{
		ForgetDeadEntries();
	}
}

void URollBallWeaponComponent::ForgetDeadEntries()
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		NextDamageTime.Reset();
		return;
	}

	const float Now = World->GetTimeSeconds();

	for (auto It = NextDamageTime.CreateIterator(); It; ++It)
	{
		if (It.Value() <= Now)
		{
			It.RemoveCurrent();
		}
	}
}
