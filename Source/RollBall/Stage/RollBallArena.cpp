#include "RollBallArena.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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

	// Fab "Rocks Stylized". 피벗은 바닥 중앙으로 옮겨 두었다.
	// 01 46x52x32  02 62x66x56  03 218x160x88  04 190x158x170  05 90x90x114  06 28x36x24
	// 07 58x58x40  08 56x36x98  09 88x74x160   10 18x18x32    11 36x30x72
	const int32 BorderRockIds[] = { 3, 4, 5, 9 };
	const int32 ObstacleRockIds[] = { 2, 3, 4, 5, 7 };
	const int32 PebbleRockIds[] = { 1, 6, 7, 8, 10, 11 };

	double DistanceToSegment(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double LengthSquared = AB.SizeSquared();
		const double T = LengthSquared > KINDA_SMALL_NUMBER
			? FMath::Clamp(FVector2D::DotProduct(Point - A, AB) / LengthSquared, 0.0, 1.0)
			: 0.0;
		return FVector2D::Distance(Point, A + AB * T);
	}
}

const FName ARollBallArena::RockTag(TEXT("RollBallRock"));

ARollBallArena::ARollBallArena()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto LoadRocks = [](const int32* Ids, int32 Count, TArray<TObjectPtr<UStaticMesh>>& Out)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			const FString Path = FString::Printf(
				TEXT("/Game/Fab/RocksStylized/SM_Rocks_%02d.SM_Rocks_%02d"), Ids[i], Ids[i]);
			ConstructorHelpers::FObjectFinder<UStaticMesh> Rock(*Path);
			if (Rock.Succeeded())
			{
				Out.Add(Rock.Object);
			}
		}
	};

	LoadRocks(BorderRockIds, UE_ARRAY_COUNT(BorderRockIds), BorderRocks);
	LoadRocks(ObstacleRockIds, UE_ARRAY_COUNT(ObstacleRockIds), ObstacleRocks);
	LoadRocks(PebbleRockIds, UE_ARRAY_COUNT(PebbleRockIds), PebbleRocks);
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
	PlateFootprints.Reset();
	RampFootprints.Reset();
	ClearRocks();

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
		PlateFootprints.Add({ FVector2D(Centre.X - HalfX, Centre.Y - HalfY), FVector2D(Centre.X + HalfX, Centre.Y + HalfY) });

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
		RampFootprints.Add({ FVector2D(RampStart.X, RampStart.Y), FVector2D(RampEnd.X, RampEnd.Y), 160.0f });
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

	if (bPlaceRocks)
	{
		// 발판 배치와 다른 흐름을 써서, 바위를 바꿔도 스테이지 지형은 그대로 둔다.
		FRandomStream RockStream(Seed * 104729 + 7);
		TArray<FVector> Obstacles;

		PlaceBorderRocks(RockStream);
		PlaceObstacleRocks(RockStream, Obstacles);
		PlacePebbles(RockStream, Obstacles);

		int32 SolidCount = 0;
		for (const auto& Pair : SolidRockInstances)
		{
			SolidCount += IsValid(Pair.Value) ? Pair.Value->GetInstanceCount() : 0;
		}

		int32 DecorCount = 0;
		for (const auto& Pair : DecorRockInstances)
		{
			DecorCount += IsValid(Pair.Value) ? Pair.Value->GetInstanceCount() : 0;
		}

		UE_LOG(LogRollBallArena, Log, TEXT("아레나 시드 %d: 바위 %d개 (장애물 %d), 자갈 %d개"),
			Seed, SolidCount, Obstacles.Num(), DecorCount);
	}
}

void ARollBallArena::ClearRocks()
{
	for (const auto& Pair : SolidRockInstances)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->ClearInstances();
		}
	}

	for (const auto& Pair : DecorRockInstances)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->ClearInstances();
		}
	}
}

void ARollBallArena::PlaceBorderRocks(FRandomStream& Stream)
{
	if (BorderRocks.Num() == 0)
	{
		return;
	}

	// 벽 선 바깥쪽에 중심을 두어 안쪽으로는 반지름의 절반 남짓만 튀어나오게 한다.
	// 적 스폰 한계(ArenaRadius - 300)보다 안으로 들어오지 않는다.
	const float Spacing = FMath::Max(100.0f, BorderRockSpacing);

	for (int32 Side = 0; Side < 4; ++Side)
	{
		const float Angle = PI * 0.5f * Side;
		const FVector2D Outward(FMath::Cos(Angle), FMath::Sin(Angle));
		const FVector2D Along(-Outward.Y, Outward.X);

		for (float T = -ArenaRadius; T <= ArenaRadius + 1.0f; T += Spacing)
		{
			const FRockPick Pick = PickRock(Stream, BorderRocks,
				Stream.FRandRange(220.0f, 340.0f), Stream.FRandRange(320.0f, 520.0f));

			const float Offset = Pick.Radius * Stream.FRandRange(0.45f, 0.8f);
			const FVector2D Position = Outward * (ArenaRadius + Offset)
				+ Along * (T + Stream.FRandRange(-Spacing * 0.25f, Spacing * 0.25f));

			AddRock(Stream, Pick, Position, true);
		}
	}
}

void ARollBallArena::PlaceObstacleRocks(FRandomStream& Stream, TArray<FVector>& OutObstacles)
{
	if (ObstacleRocks.Num() == 0)
	{
		return;
	}

	const int32 Count = Stream.RandRange(FMath::Min(ObstacleRockMin, ObstacleRockMax),
		FMath::Max(ObstacleRockMin, ObstacleRockMax));

	// 플레이어가 시작하는 가운데는 비워 둔다.
	const float StartClearRadius = 900.0f;

	for (int32 i = 0; i < Count; ++i)
	{
		const FRockPick Pick = PickRock(Stream, ObstacleRocks,
			Stream.FRandRange(110.0f, 170.0f), 240.0f);

		const float Limit = ArenaRadius - 350.0f - Pick.Radius - ObstacleClearance * 0.5f;
		if (Limit <= StartClearRadius)
		{
			break;
		}

		for (int32 Attempt = 0; Attempt < 40; ++Attempt)
		{
			const FVector2D Position(Stream.FRandRange(-Limit, Limit), Stream.FRandRange(-Limit, Limit));

			if (Position.Size() < StartClearRadius + Pick.Radius)
			{
				continue;
			}

			if (IsBlockedByLayout(Position, Pick.Radius + ObstacleClearance))
			{
				continue;
			}

			bool bTooClose = false;
			for (const FVector& Other : OutObstacles)
			{
				if (FVector2D::Distance(Position, FVector2D(Other.X, Other.Y)) < Pick.Radius + Other.Z + ObstacleClearance)
				{
					bTooClose = true;
					break;
				}
			}

			if (bTooClose)
			{
				continue;
			}

			AddRock(Stream, Pick, Position, true);
			OutObstacles.Add(FVector(Position, Pick.Radius));
			break;
		}
	}
}

void ARollBallArena::PlacePebbles(FRandomStream& Stream, const TArray<FVector>& Obstacles)
{
	if (PebbleRocks.Num() == 0 || PebbleCount <= 0)
	{
		return;
	}

	int32 Placed = 0;

	// 장애물 바위 발치에 두세 개씩 모아 두면 떨어진 조각처럼 보인다.
	for (const FVector& Obstacle : Obstacles)
	{
		const int32 Cluster = Stream.RandRange(2, 3);
		for (int32 i = 0; i < Cluster && Placed < PebbleCount; ++i)
		{
			const FRockPick Pick = PickRock(Stream, PebbleRocks, Stream.FRandRange(18.0f, 40.0f), 60.0f);
			const float Angle = Stream.FRandRange(0.0f, 2.0f * PI);
			const float Distance = Obstacle.Z + Pick.Radius + Stream.FRandRange(10.0f, 90.0f);
			const FVector2D Position = FVector2D(Obstacle.X, Obstacle.Y)
				+ FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;

			if (IsBlockedByLayout(Position, Pick.Radius))
			{
				continue;
			}

			AddRock(Stream, Pick, Position, false);
			++Placed;
		}
	}

	const float Limit = ArenaRadius - 250.0f;

	for (int32 Attempt = 0; Placed < PebbleCount && Attempt < PebbleCount * 6; ++Attempt)
	{
		const FRockPick Pick = PickRock(Stream, PebbleRocks, Stream.FRandRange(15.0f, 45.0f), 60.0f);
		const FVector2D Position(Stream.FRandRange(-Limit, Limit), Stream.FRandRange(-Limit, Limit));

		if (IsBlockedByLayout(Position, Pick.Radius + 20.0f))
		{
			continue;
		}

		AddRock(Stream, Pick, Position, false);
		++Placed;
	}
}

ARollBallArena::FRockPick ARollBallArena::PickRock(FRandomStream& Stream,
	const TArray<TObjectPtr<UStaticMesh>>& Meshes, float TargetRadius, float MaxHeight) const
{
	FRockPick Pick;

	if (Meshes.Num() == 0)
	{
		return Pick;
	}

	Pick.Mesh = Meshes[Stream.RandRange(0, Meshes.Num() - 1)];
	if (Pick.Mesh == nullptr)
	{
		return Pick;
	}

	const FVector Extent = Pick.Mesh->GetBounds().BoxExtent;
	const float MeshRadius = FMath::Max(static_cast<float>(FMath::Max(Extent.X, Extent.Y)), 1.0f);
	const float MeshHeight = FMath::Max(static_cast<float>(Extent.Z * 2.0), 1.0f);

	Pick.Scale = FMath::Min(TargetRadius / MeshRadius, MaxHeight / MeshHeight);
	Pick.Radius = MeshRadius * Pick.Scale;
	Pick.Height = MeshHeight * Pick.Scale;
	return Pick;
}

void ARollBallArena::AddRock(FRandomStream& Stream, const FRockPick& Pick, const FVector2D& Position, bool bCollision)
{
	if (Pick.Mesh == nullptr)
	{
		return;
	}

	UInstancedStaticMeshComponent* Component = GetRockComponent(Pick.Mesh, bCollision);
	if (Component == nullptr)
	{
		return;
	}

	// 살짝 묻고 기울여야 바닥에 얹힌 게 아니라 박혀 있는 것처럼 보인다.
	const FRotator Rotation(Stream.FRandRange(-6.0f, 6.0f), Stream.FRandRange(0.0f, 360.0f), Stream.FRandRange(-6.0f, 6.0f));
	const FVector Location(Position.X, Position.Y, -Pick.Height * Stream.FRandRange(0.05f, 0.12f));

	Component->AddInstance(FTransform(Rotation, Location, FVector(Pick.Scale)));
}

UInstancedStaticMeshComponent* ARollBallArena::GetRockComponent(UStaticMesh* Mesh, bool bCollision)
{
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Instances =
		bCollision ? SolidRockInstances : DecorRockInstances;

	if (const TObjectPtr<UInstancedStaticMeshComponent>* Found = Instances.Find(Mesh))
	{
		if (IsValid(*Found))
		{
			return *Found;
		}
	}

	UInstancedStaticMeshComponent* Component = NewObject<UInstancedStaticMeshComponent>(this,
		MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(),
			bCollision ? TEXT("Rocks") : TEXT("Pebbles")));

	if (Component == nullptr)
	{
		return nullptr;
	}

	Component->SetupAttachment(RootComponent);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetStaticMesh(Mesh);

	if (bCollision)
	{
		Component->SetCollisionProfileName(TEXT("BlockAll"));
		Component->SetCollisionObjectType(ECC_WorldStatic);
		Component->ComponentTags.Add(RockTag);
	}
	else
	{
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
	}

	Component->RegisterComponent();

	Instances.Add(Mesh, Component);
	return Component;
}

bool ARollBallArena::IsBlockedByLayout(const FVector2D& Position, float Radius) const
{
	for (const FFootprint& Plate : PlateFootprints)
	{
		if (Position.X > Plate.Min.X - Radius && Position.X < Plate.Max.X + Radius
			&& Position.Y > Plate.Min.Y - Radius && Position.Y < Plate.Max.Y + Radius)
		{
			return true;
		}
	}

	for (const FRampFootprint& Ramp : RampFootprints)
	{
		if (DistanceToSegment(Position, Ramp.From, Ramp.To) < Ramp.HalfWidth + Radius)
		{
			return true;
		}
	}

	return false;
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
