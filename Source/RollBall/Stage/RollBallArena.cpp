#include "RollBallArena.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogRollBallArena, Log, All);

namespace
{
	FLinearColor ColorOf(int32 Kind)
	{
		switch (Kind)
		{
		case 1:  return FLinearColor(0.42f, 0.46f, 0.55f);
		case 2:  return FLinearColor(0.09f, 0.10f, 0.13f);
		default: return FLinearColor(0.18f, 0.20f, 0.26f);
		}
	}

	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* BasicMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
}

ARollBallArena::ARollBallArena()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void ARollBallArena::BeginPlay()
{
	Super::BeginPlay();

	SpawnLightingIfMissing();

	if (bBuildOnBeginPlay && Boxes.Num() == 0)
	{
		Build(1);
	}
}

UStaticMeshComponent* ARollBallArena::AddBox(EBoxKind Kind, const FVector& Centre,
	const FVector& Extent, const FRotator& Rotation)
{
	UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>(this,
		MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("Box")));

	if (Box == nullptr)
	{
		return nullptr;
	}

	Box->SetupAttachment(RootComponent);
	Box->RegisterComponent();

	if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, CubePath))
	{
		Box->SetStaticMesh(Cube);
	}

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, BasicMaterialPath))
	{
		Box->SetMaterial(0, Base);

		if (UMaterialInstanceDynamic* Dynamic = Box->CreateAndSetMaterialInstanceDynamic(0))
		{
			Dynamic->SetVectorParameterValue(TEXT("Color"), ColorOf(static_cast<int32>(Kind)));
		}
	}

	Box->SetMobility(EComponentMobility::Movable);
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->SetCollisionObjectType(ECC_WorldStatic);

	Box->SetWorldLocationAndRotation(GetActorLocation() + Centre, Rotation);
	Box->SetWorldScale3D(FVector(Extent.X * 2.0f, Extent.Y * 2.0f, Extent.Z * 2.0f) / CubeSize);

	Boxes.Add(Box);
	return Box;
}

void ARollBallArena::Build(int32 Seed)
{
	for (UStaticMeshComponent* Box : Boxes)
	{
		if (IsValid(Box))
		{
			Box->DestroyComponent();
		}
	}
	Boxes.Reset();

	FRandomStream Stream(Seed * 7919 + 13);

	const float FloorThickness = 40.0f;
	AddFloorBox(FVector(0.0f, 0.0f, -FloorThickness * 0.5f),
		FVector(ArenaRadius, ArenaRadius, FloorThickness * 0.5f));

	const float AngleStep = 2.0f * PI / FMath::Max(1, PlateCount);

	for (int32 i = 0; i < PlateCount; ++i)
	{
		const float Angle = AngleStep * i + Stream.FRandRange(-AngleStep * 0.25f, AngleStep * 0.25f);
		const float Distance = Stream.FRandRange(CentreHalfSize + 700.0f, ArenaRadius - 900.0f);

		const float HalfX = Stream.FRandRange(PlateHalfMin, PlateHalfMax);
		const float HalfY = Stream.FRandRange(PlateHalfMin, PlateHalfMax);

		const int32 Step = Stream.RandRange(1, MaxStep);
		const float Height = Step * StepHeight;

		const FVector Centre(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, Height);

		AddFloorBox(FVector(Centre.X, Centre.Y, Height * 0.5f),
			FVector(HalfX, HalfY, Height * 0.5f));

		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);

		const float ToEdgeX = FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER
			? HalfX / FMath::Abs(Direction.X) : BIG_NUMBER;
		const float ToEdgeY = FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER
			? HalfY / FMath::Abs(Direction.Y) : BIG_NUMBER;

		const float PlateHalf = FMath::Min(ToEdgeX, ToEdgeY);
		const float RampRun = FMath::Max(400.0f, Height * 3.0f);

		const FVector RampEnd = Centre - Direction * (PlateHalf - 30.0f);
		const FVector RampStart = RampEnd - Direction * RampRun;

		AddRamp(FVector(RampStart.X, RampStart.Y, 0.0f),
			FVector(RampEnd.X, RampEnd.Y, Height),
			320.0f);
	}

	const float WallThickness = 60.0f;

	for (int32 Side = 0; Side < 4; ++Side)
	{
		const float Angle = PI * 0.5f * Side;
		const FVector Outward(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		const FVector Centre = Outward * ArenaRadius + FVector(0.0f, 0.0f, WallHeight * 0.5f);

		const bool bAlongY = (Side % 2) == 0;
		const FVector Extent = bAlongY
			? FVector(WallThickness * 0.5f, ArenaRadius + WallThickness, WallHeight * 0.5f)
			: FVector(ArenaRadius + WallThickness, WallThickness * 0.5f, WallHeight * 0.5f);

		AddWallBox(Centre, Extent);
	}

}

void ARollBallArena::SpawnLightingIfMissing()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	bool bHasDirectional = false;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		bHasDirectional = true;
		break;
	}

	if (!bHasDirectional)
	{
		if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(),
			FVector(0.0f, 0.0f, 4000.0f), FRotator(-52.0f, 35.0f, 0.0f), Params))
		{
			Sun->SetMobility(EComponentMobility::Movable);
			Sun->GetLightComponent()->SetIntensity(2.2f);
			Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.97f, 0.9f));
		}

	}

	bool bHasSky = false;
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		bHasSky = true;
		break;
	}

	if (!bHasSky)
	{
		if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(
			ASkyLight::StaticClass(), FVector(0.0f, 0.0f, 1500.0f), FRotator::ZeroRotator, Params))
		{
			USkyLightComponent* Component = Sky->GetLightComponent();
			Component->SetMobility(EComponentMobility::Movable);

			Component->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
			Component->Cubemap = LoadObject<UTextureCube>(nullptr,
				TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"));
			Component->bLowerHemisphereIsBlack = false;
			Component->SetLightColor(FLinearColor(0.70f, 0.78f, 0.95f));
			Component->SetIntensity(1.0f);
			Component->RecaptureSky();
		}
	}
}

void ARollBallArena::AddFloorBox(const FVector& Centre, const FVector& Extent)
{
	AddBox(EBoxKind::Floor, Centre, Extent, FRotator::ZeroRotator);
}

void ARollBallArena::AddWallBox(const FVector& Centre, const FVector& Extent)
{
	AddBox(EBoxKind::Wall, Centre, Extent, FRotator::ZeroRotator);
}

void ARollBallArena::AddRamp(const FVector& From, const FVector& To, float Width)
{
	const FVector Delta = To - From;
	const float Run = FVector(Delta.X, Delta.Y, 0.0f).Size();

	if (Run < 1.0f)
	{
		return;
	}

	const float Length = Delta.Size();
	const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Delta.Z, Run));

	FRotator Rotation = FVector(Delta.X, Delta.Y, 0.0f).Rotation();
	Rotation.Pitch = Pitch;

	const FVector Centre = From + Delta * 0.5f;
	const float Thickness = 30.0f;

	AddBox(EBoxKind::Ramp, Centre, FVector(Length, Width, Thickness) * 0.5f, Rotation);
}
