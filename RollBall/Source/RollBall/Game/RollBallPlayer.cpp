// Fill out your copyright notice in the Description page of Project Settings.


#include "RollBallPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

// Sets default values
ARollBallPlayer::ARollBallPlayer()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	
	RootComponent = Mesh;
	SpringArm->SetupAttachment(Mesh);
	Camera->SetupAttachment(SpringArm);

	Mesh->SetSimulatePhysics(true);

	Mesh->OnComponentHit.AddDynamic(this, &ARollBallPlayer::OnHit);
}

// Called when the game starts or when spawned
void ARollBallPlayer::BeginPlay()
{
	Super::BeginPlay();

	MoveForce *= Mesh->GetMass();
	JumpImpulse *= Mesh->GetMass();
}

// Called every frame
void ARollBallPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
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

void ARollBallPlayer::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float HitDirection = Hit.Normal.Z;
	if (0 < HitDirection)
		JumpCount = 0;
}

