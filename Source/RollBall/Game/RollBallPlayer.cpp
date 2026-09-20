#include "RollBallPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include "RollBallGameInstance.h"
#include "RollBall/Enemy/RollBallEnemy.h"
#include "RollBall/Weapon/RollBallWeaponComponent.h"

static TAutoConsoleVariable<int32> CVarAutoPlay(
	TEXT("rollball.AutoPlay"),
	0,
	TEXT("1 이면 공이 알아서 적을 피해 움직인다. 테스트용."),
	ECVF_Cheat);

ARollBallPlayer::ARollBallPlayer()
{

	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Weapons = CreateDefaultSubobject<URollBallWeaponComponent>("Weapons");

	RootComponent = Mesh;
	SpringArm->SetupAttachment(Mesh);
	Camera->SetupAttachment(SpringArm);
	Weapons->SetupAttachment(Mesh);

	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->SetRelativeRotation(FRotator(-48.0f, 0.0f, 0.0f));
	SpringArm->TargetArmLength = 1100.0f;

	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 8.0f;

	Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	Camera->PostProcessSettings.AutoExposureMinBrightness = 1.0f;
	Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	Camera->PostProcessSettings.AutoExposureMaxBrightness = 1.0f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Mesh->SetStaticMesh(Sphere.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterial> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, BasicMaterial.Object);
	}

	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetNotifyRigidBodyCollision(true);

	Mesh->OnComponentHit.AddDynamic(this, &ARollBallPlayer::OnHit);
}

void ARollBallPlayer::BeginPlay()
{
	Super::BeginPlay();

	if (GetActorLocation().Z < 150.0)
	{
		SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, 250.0f), false, nullptr,
			ETeleportType::ResetPhysics);
	}

	JumpImpulse *= Mesh->GetMass();

	ApplySkillStats();
}

void ARollBallPlayer::ApplySkillStats()
{
	const URollBallGameInstance* GameInstance = GetGameInstance<URollBallGameInstance>();
	if (GameInstance == nullptr)
	{
		return;
	}

	const FRollBallSkillStats Stats = GameInstance->GetSkillStats();

	MoveForce = Stats.Get(ERollBallSkillStat::MoveForce) * Mesh->GetMass();
	MaxSpeed = Stats.Get(ERollBallSkillStat::MaxSpeed);
	KnockbackResist = Stats.Get(ERollBallSkillStat::KnockbackResist);
	MaxHealth = Stats.GetInt(ERollBallSkillStat::MaxHealth);
	Health = MaxHealth;

	if (Weapons != nullptr)
	{
		Weapons->RebuildFromStats(Stats);
	}

	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void ARollBallPlayer::ApplyKnockback(const FVector& Impulse)
{
	Mesh->AddImpulse(Impulse * (1.0f - KnockbackResist));
}

bool ARollBallPlayer::IsInvulnerable() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && World->GetTimeSeconds() < InvulnerableUntil;
}

void ARollBallPlayer::TakeHit(int32 Damage, AActor* Causer)
{
	if (Damage <= 0 || !IsAlive() || IsInvulnerable())
	{
		return;
	}

	Health = FMath::Max(0, Health - Damage);

	if (const UWorld* World = GetWorld())
	{
		InvulnerableUntil = World->GetTimeSeconds() + InvulnerableSeconds;
	}

	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0)
	{

		OnDied.Broadcast();
	}
}

void ARollBallPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CVarAutoPlay.GetValueOnGameThread() != 0)
	{
		TickAutoPlay(DeltaTime);
	}

	FVector Velocity = Mesh->GetPhysicsLinearVelocity();
	const FVector Flat(Velocity.X, Velocity.Y, 0.0f);

	if (Flat.SizeSquared() > FMath::Square(MaxSpeed))
	{
		const FVector Capped = Flat.GetSafeNormal() * MaxSpeed;
		Mesh->SetPhysicsLinearVelocity(FVector(Capped.X, Capped.Y, Velocity.Z));
	}
}

void ARollBallPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAxis("MoveForward", this, &ARollBallPlayer::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ARollBallPlayer::MoveRight);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ARollBallPlayer::Jump);

}

void ARollBallPlayer::MoveForward(float Value)
{
	const FVector Forward = Camera->GetForwardVector() * MoveForce * Value;
	Mesh->AddForce(Forward);
}

void ARollBallPlayer::MoveRight(float Value)
{
	const FVector Right = Camera->GetRightVector() * MoveForce * Value;
	Mesh->AddForce(Right);
}

void ARollBallPlayer::Jump()
{
	if (MaxJumpCount <= JumpCount)
		return;

	Mesh->AddImpulse(FVector(0, 0, JumpImpulse));
	JumpCount++;
}

void ARollBallPlayer::TickAutoPlay(float DeltaTime)
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FVector Here = GetActorLocation();

	FVector Away = FVector::ZeroVector;
	int32 Seen = 0;

	for (TActorIterator<ARollBallEnemy> It(World); It; ++It)
	{
		const ARollBallEnemy* Enemy = *It;
		if (!IsValid(Enemy) || !Enemy->IsAlive())
		{
			continue;
		}

		const FVector Delta = Here - Enemy->GetActorLocation();
		const float Distance = Delta.Size2D();

		if (Distance > AutoPlayAwareness || Distance < 1.0f)
		{
			continue;
		}

		Away += FVector(Delta.X, Delta.Y, 0.0f).GetSafeNormal() * (AutoPlayAwareness / Distance);
		++Seen;
	}

	FVector Desired = (Seen > 0)
		? Away.GetSafeNormal()
		: FVector(-Here.X, -Here.Y, 0.0f).GetSafeNormal();

	const float DistanceFromCentre = Here.Size2D();
	if (DistanceFromCentre > AutoPlayHomeRadius)
	{
		Desired = (Desired + FVector(-Here.X, -Here.Y, 0.0f).GetSafeNormal() * 2.0f).GetSafeNormal();
	}

	if (!Desired.IsNearlyZero())
	{
		Mesh->AddForce(Desired * MoveForce);
	}
}

void ARollBallPlayer::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float HitDirection = Hit.Normal.Z;
	if (0 < HitDirection)
		JumpCount = 0;
}
