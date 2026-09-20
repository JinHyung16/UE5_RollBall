#include "RollBallEnemy.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#include "RollBall/Game/RollBallPlayer.h"

namespace
{

	const TCHAR* MeshPathOf(ERollBallEnemyKind Kind)
	{
		switch (Kind)
		{
		case ERollBallEnemyKind::Roller:  return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		case ERollBallEnemyKind::Blocker: return TEXT("/Engine/BasicShapes/Cube.Cube");
		case ERollBallEnemyKind::Meteor:  return TEXT("/Engine/BasicShapes/Cone.Cone");
		default:                          return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		}
	}

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
}

ARollBallEnemy::ARollBallEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (DefaultMesh.Succeeded())
	{
		Mesh->SetStaticMesh(DefaultMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (DefaultMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, DefaultMaterial.Object);
	}

	Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Mesh->SetGenerateOverlapEvents(true);
}

void ARollBallEnemy::BeginPlay()
{
	Super::BeginPlay();

	Mesh->OnComponentBeginOverlap.AddDynamic(this, &ARollBallEnemy::OnOverlap);
	Mesh->OnComponentHit.AddDynamic(this, &ARollBallEnemy::OnHit);

	if (MeshMaterial == nullptr)
	{
		ConfigureForKind(1.0f);
	}
}

void ARollBallEnemy::Setup(ERollBallEnemyKind InKind, float HealthScale, float SpeedScale,
	int32 InGoldReward, float SizeScale)
{
	Kind = InKind;
	GoldReward = FMath::Max(0, InGoldReward);

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

	ConfigureForKind(SizeScale);

	if (Kind == ERollBallEnemyKind::Meteor)
	{

		MeteorTargetZ = GetActorLocation();
		MeteorTargetZ.Z -= MeteorDropHeight;
	}
}

void ARollBallEnemy::ConfigureForKind(float SizeScale)
{
	if (UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr, MeshPathOf(Kind)))
	{
		Mesh->SetStaticMesh(Shape);
	}

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		Mesh->SetMaterial(0, Base);
	}

	MeshMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	if (MeshMaterial != nullptr)
	{
		MeshMaterial->SetVectorParameterValue(TEXT("Color"), ColorOf(Kind));
	}

	SetActorScale3D(FVector(FMath::Max(0.1f, SizeScale)));

	switch (Kind)
	{
	case ERollBallEnemyKind::Roller:

		Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		Mesh->SetSimulatePhysics(true);
		Mesh->SetNotifyRigidBodyCollision(true);
		break;

	case ERollBallEnemyKind::Blocker:

		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		break;

	case ERollBallEnemyKind::Meteor:

		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		break;

	default:

		Mesh->SetSimulatePhysics(false);
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

	if (bDying)
	{
		return;
	}

	ARollBallPlayer* Player = FindPlayer();

	switch (Kind)
	{
	case ERollBallEnemyKind::Chaser:
	{
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
	if (bDying || Amount <= 0.0f)
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
	Destroy();
}

void ARollBallEnemy::TouchPlayer(ARollBallPlayer* Player, const FVector& FromDirection)
{
	if (Player == nullptr || bDying)
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

ARollBallPlayer* ARollBallEnemy::FindPlayer() const
{
	return Cast<ARollBallPlayer>(UGameplayStatics::GetPlayerPawn(this, 0));
}
