#include "RollBallEnemy.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#include "RollBall/Game/RollBallPlayer.h"

namespace
{
	FLinearColor ColorOf(ERollBallEnemyKind Kind)
	{
		switch (Kind)
		{
		case ERollBallEnemyKind::Roller:  return FLinearColor(0.25f, 0.55f, 0.95f);
		case ERollBallEnemyKind::Blocker: return FLinearColor(0.35f, 0.65f, 0.30f);
		case ERollBallEnemyKind::Meteor:  return FLinearColor(0.95f, 0.55f, 0.15f);
		default:                          return FLinearColor(0.90f, 0.25f, 0.30f);
		}
	}

	const FVector ParkedLocation(0.0f, 0.0f, -100000.0f);
}

ARollBallEnemy::ARollBallEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	KindMeshes.SetNum(4);
	KindMeshes[static_cast<int32>(ERollBallEnemyKind::Chaser)] = Cylinder.Object;
	KindMeshes[static_cast<int32>(ERollBallEnemyKind::Roller)] = Sphere.Object;
	KindMeshes[static_cast<int32>(ERollBallEnemyKind::Blocker)] = Cube.Object;
	KindMeshes[static_cast<int32>(ERollBallEnemyKind::Meteor)] = Cone.Object;
	BaseMaterial = Basic.Object;

	Mesh->SetStaticMesh(Cylinder.Object);
	Mesh->SetMaterial(0, BaseMaterial);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(true);
}

void ARollBallEnemy::BeginPlay()
{
	Super::BeginPlay();

	Mesh->OnComponentBeginOverlap.AddDynamic(this, &ARollBallEnemy::OnOverlap);
	Mesh->OnComponentHit.AddDynamic(this, &ARollBallEnemy::OnHit);

	Mesh->SetMaterial(0, BaseMaterial);
	MeshMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);

	if (bAutoActivate && !bActive)
	{
		Activate(Kind, 1.0f, 1.0f, GoldReward, 1.0f, GetActorLocation());
	}
}

void ARollBallEnemy::Activate(ERollBallEnemyKind InKind, float HealthScale, float SpeedScale,
	int32 InGoldReward, float SizeScale, const FVector& Location)
{
	Kind = InKind;
	GoldReward = FMath::Max(0, InGoldReward);
	bDying = false;

	ApplyStatsForKind(HealthScale, SpeedScale);

	Mesh->SetSimulatePhysics(false);
	SetActorLocationAndRotation(Location, FRotator::ZeroRotator, false, nullptr, ETeleportType::ResetPhysics);

	ApplyShapeForKind(SizeScale);
	ApplyCollisionForKind();

	if (Kind == ERollBallEnemyKind::Meteor)
	{
		MeteorTargetZ = Location;
		MeteorTargetZ.Z -= MeteorDropHeight;
	}

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	bActive = true;
}

void ARollBallEnemy::Deactivate()
{
	bActive = false;
	bDying = true;

	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);

	if (Mesh->IsSimulatingPhysics())
	{
		Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Mesh->SetSimulatePhysics(false);
	}

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorLocation(ParkedLocation, false, nullptr, ETeleportType::ResetPhysics);
}

void ARollBallEnemy::ApplyStatsForKind(float HealthScale, float SpeedScale)
{
	switch (Kind)
	{
	case ERollBallEnemyKind::Chaser:
		MaxHealth = 3.0f;
		MoveSpeed = 320.0f;
		ContactDamage = 1;
		break;

	case ERollBallEnemyKind::Roller:
		MaxHealth = 6.0f;
		MoveSpeed = 700.0f;
		ContactDamage = 0;
		break;

	case ERollBallEnemyKind::Blocker:
		MaxHealth = 10.0f;
		MoveSpeed = 0.0f;
		ContactDamage = 0;
		break;

	case ERollBallEnemyKind::Meteor:
		MaxHealth = 4.0f;
		MoveSpeed = 0.0f;
		ContactDamage = 1;
		break;
	}

	MaxHealth *= FMath::Max(0.01f, HealthScale);
	MoveSpeed *= FMath::Max(0.01f, SpeedScale);
	Health = MaxHealth;
}

void ARollBallEnemy::ApplyShapeForKind(float SizeScale)
{
	if (!bHasShape || ShapeKind != Kind)
	{
		const int32 Index = static_cast<int32>(Kind);
		if (KindMeshes.IsValidIndex(Index) && KindMeshes[Index] != nullptr)
		{
			Mesh->SetStaticMesh(KindMeshes[Index]);
		}

		if (MeshMaterial != nullptr)
		{
			Mesh->SetMaterial(0, MeshMaterial);
		}

		ShapeKind = Kind;
		bHasShape = true;
	}

	if (MeshMaterial != nullptr)
	{
		MeshMaterial->SetVectorParameterValue(TEXT("Color"), ColorOf(Kind));
	}

	SetActorScale3D(FVector(FMath::Max(0.1f, SizeScale)));
}

void ARollBallEnemy::ApplyCollisionForKind()
{
	switch (Kind)
	{
	case ERollBallEnemyKind::Roller:
		Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		Mesh->SetNotifyRigidBodyCollision(true);
		Mesh->SetSimulatePhysics(true);
		break;

	case ERollBallEnemyKind::Blocker:
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		break;

	case ERollBallEnemyKind::Meteor:
		Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		break;

	default:
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionObjectType(ECC_WorldDynamic);
		Mesh->SetCollisionResponseToAllChannels(ECR_Overlap);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		break;
	}

	Mesh->SetGenerateOverlapEvents(true);
}

void ARollBallEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bActive || bDying)
	{
		return;
	}

	switch (Kind)
	{
	case ERollBallEnemyKind::Chaser:
	{
		const ARollBallPlayer* Player = FindPlayer();
		if (Player == nullptr)
		{
			break;
		}

		const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
		const FVector Flat(ToPlayer.X, ToPlayer.Y, 0.0f);

		if (Flat.SizeSquared() > 1.0f)
		{
			AddActorWorldOffset(Flat.GetSafeNormal() * MoveSpeed * DeltaTime, true);
			SetActorRotation(Flat.Rotation());
		}
		break;
	}

	case ERollBallEnemyKind::Roller:
	{
		const ARollBallPlayer* Player = FindPlayer();
		if (Player == nullptr)
		{
			break;
		}

		const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
		const FVector Flat(ToPlayer.X, ToPlayer.Y, 0.0f);

		if (Flat.SizeSquared() > 1.0f)
		{
			Mesh->AddForce(Flat.GetSafeNormal() * MoveSpeed * Mesh->GetMass());
		}
		break;
	}

	case ERollBallEnemyKind::Meteor:
	{
		FVector Location = GetActorLocation();
		Location.Z -= MeteorFallSpeed * DeltaTime;
		SetActorLocation(Location, true);

		if (Location.Z <= MeteorTargetZ.Z)
		{
			Die(nullptr);
		}
		break;
	}

	default:
		break;
	}
}

bool ARollBallEnemy::ApplyWeaponDamage(float Amount, AActor* Causer)
{
	if (!bActive || bDying || Amount <= 0.0f)
	{
		return false;
	}

	Health -= Amount;
	if (Health > 0.0f)
	{
		return false;
	}

	Die(Causer);
	return true;
}

void ARollBallEnemy::Die(AActor* Causer)
{
	if (bDying)
	{
		return;
	}
	bDying = true;

	const int32 Payout = (Causer != nullptr) ? GoldReward : 0;

	OnDied.Broadcast(this, Payout);
	Deactivate();
}

void ARollBallEnemy::TouchPlayer(ARollBallPlayer* Player, const FVector& FromDirection)
{
	if (Player == nullptr || !bActive || bDying)
	{
		return;
	}

	if (Kind == ERollBallEnemyKind::Roller)
	{
		Player->ApplyKnockback(FromDirection * PushImpulse);
		return;
	}

	if (ContactDamage > 0)
	{
		Player->TakeHit(ContactDamage, this);
	}

	if (Kind == ERollBallEnemyKind::Meteor)
	{
		Die(nullptr);
	}
}

void ARollBallEnemy::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	if (ARollBallPlayer* Player = Cast<ARollBallPlayer>(OtherActor))
	{
		const FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		TouchPlayer(Player, Direction);
	}
}

void ARollBallEnemy::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ARollBallPlayer* Player = Cast<ARollBallPlayer>(OtherActor))
	{
		const FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		TouchPlayer(Player, Direction);
	}
}

ARollBallPlayer* ARollBallEnemy::FindPlayer()
{
	if (!CachedPlayer.IsValid())
	{
		CachedPlayer = Cast<ARollBallPlayer>(UGameplayStatics::GetPlayerPawn(this, 0));
	}
	return CachedPlayer.Get();
}
