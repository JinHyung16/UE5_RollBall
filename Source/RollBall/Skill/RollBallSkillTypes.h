#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RollBallSkillTypes.generated.h"

UENUM(BlueprintType)
enum class ERollBallSkillStat : uint8
{
	None                UMETA(DisplayName = "없음"),

	MaxHealth           UMETA(DisplayName = "최대 체력"),
	MoveForce           UMETA(DisplayName = "가속력"),
	MaxSpeed            UMETA(DisplayName = "최고 속도"),
	KnockbackResist     UMETA(DisplayName = "밀림 저항"),
	PickupRadius        UMETA(DisplayName = "획득 범위"),

	GoldGain            UMETA(DisplayName = "골드 획득량"),
	RebirthMultiplier   UMETA(DisplayName = "환생 배수"),

	SawUnlock           UMETA(DisplayName = "전기톱 장착"),
	SawSize             UMETA(DisplayName = "전기톱 크기"),
	SawDamage           UMETA(DisplayName = "전기톱 피해"),

	HammerUnlock        UMETA(DisplayName = "헤머 장착"),
	HammerCount         UMETA(DisplayName = "헤머 개수"),
	HammerKnockback     UMETA(DisplayName = "헤머 넉백"),
	HammerSpinSpeed     UMETA(DisplayName = "헤머 회전 속도"),
	HammerDamage        UMETA(DisplayName = "헤머 피해"),

	BulldozerUnlock     UMETA(DisplayName = "불도저 장착"),
	BulldozerWidth      UMETA(DisplayName = "불도저 폭"),
	BulldozerPush       UMETA(DisplayName = "불도저 밀어내기"),

	DrillUnlock         UMETA(DisplayName = "드릴 장착"),
	DrillDamage         UMETA(DisplayName = "드릴 피해"),
	DrillPierce         UMETA(DisplayName = "드릴 관통"),
	DrillTopMount       UMETA(DisplayName = "드릴 추가 장착"),

	Count               UMETA(Hidden),
};

UENUM(BlueprintType)
enum class ERollBallSkillNodeKind : uint8
{
	Core      UMETA(DisplayName = "시작점"),
	Small     UMETA(DisplayName = "작은 노드"),
	Notable   UMETA(DisplayName = "큰 노드"),
	Gate      UMETA(DisplayName = "관문"),
	Keystone  UMETA(DisplayName = "키스톤"),
};

UENUM(BlueprintType)
enum class ERollBallEdgeRoute : uint8
{
	HorizontalFirst UMETA(DisplayName = "가로 먼저"),
	VerticalFirst   UMETA(DisplayName = "세로 먼저"),
	Straight        UMETA(DisplayName = "직선"),
};

UENUM(BlueprintType)
enum class ERollBallSkillBranch : uint8
{
	Core       UMETA(DisplayName = "중앙"),
	Survival   UMETA(DisplayName = "생존"),
	Mobility   UMETA(DisplayName = "기동"),
	Weapon     UMETA(DisplayName = "무기"),
	Economy    UMETA(DisplayName = "경제"),
};

USTRUCT(BlueprintType)
struct FRollBallSkillDefRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "표시")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "표시")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "표시")
	ERollBallSkillBranch Branch = ERollBallSkillBranch::Survival;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "표시")
	ERollBallSkillNodeKind NodeKind = ERollBallSkillNodeKind::Small;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "표시", meta = (ClampMin = "0.1"))
	float ScaleX = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "표시", meta = (ClampMin = "0.1"))
	float ScaleY = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "효과")
	ERollBallSkillStat GrantStat = ERollBallSkillStat::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "효과")
	float AmountPerLevel = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "비용", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "비용", meta = (ClampMin = "0"))
	int32 BaseCost = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "비용", meta = (ClampMin = "1.0"))
	float CostGrowth = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "비용", meta = (ClampMin = "0"))
	int32 RequiredBestStage = 0;
};

USTRUCT(BlueprintType)
struct FRollBallSkillPlacementRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "배치")
	int32 PlacementId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "배치")
	FName DefinitionRowName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "배치")
	float CellX = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "배치")
	float CellY = 0.0f;
};

USTRUCT(BlueprintType)
struct FRollBallSkillEdgeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "연결")
	int32 FromPlacementId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "연결")
	int32 ToPlacementId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "연결")
	ERollBallEdgeRoute RouteType = ERollBallEdgeRoute::HorizontalFirst;
};

USTRUCT(BlueprintType)
struct FRollBallSkillNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "노드")
	int32 NodeIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "노드")
	int32 PlacementId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "노드")
	FName DefinitionRowName;

	UPROPERTY(BlueprintReadOnly, Category = "노드")
	FVector2D Cell = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "노드")
	FRollBallSkillDefRow Definition;

	UPROPERTY(BlueprintReadOnly, Category = "노드")
	TArray<int32> Adjacent;
};

USTRUCT(BlueprintType)
struct FRollBallSkillEdge
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "간선")
	int32 NodeIndexA = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "간선")
	int32 NodeIndexB = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "간선")
	ERollBallEdgeRoute RouteType = ERollBallEdgeRoute::HorizontalFirst;
};

USTRUCT(BlueprintType)
struct ROLLBALL_API FRollBallSkillStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "스탯")
	TMap<ERollBallSkillStat, float> Amounts;

	float Get(ERollBallSkillStat Stat) const
	{
		const float* Found = Amounts.Find(Stat);
		return Found != nullptr ? *Found : BaseAmountOf(Stat);
	}

	int32 GetInt(ERollBallSkillStat Stat) const
	{
		return FMath::RoundToInt(Get(Stat));
	}

	bool IsUnlocked(ERollBallSkillStat UnlockStat) const
	{
		return Get(UnlockStat) >= 1.0f;
	}

	static float BaseAmountOf(ERollBallSkillStat Stat);

	static float ClampAmount(ERollBallSkillStat Stat, float Amount);

	static FText DescribeAmount(ERollBallSkillStat Stat, float Amount);
};
