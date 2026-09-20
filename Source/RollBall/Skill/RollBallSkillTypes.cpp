#include "RollBallSkillTypes.h"

#define LOCTEXT_NAMESPACE "RollBallSkill"

namespace
{

	bool IsMultiplierStat(ERollBallSkillStat Stat)
	{
		switch (Stat)
		{
		case ERollBallSkillStat::GoldGain:
		case ERollBallSkillStat::RebirthMultiplier:
		case ERollBallSkillStat::SawSize:
		case ERollBallSkillStat::HammerKnockback:
		case ERollBallSkillStat::BulldozerWidth:
		case ERollBallSkillStat::BulldozerPush:
			return true;
		default:
			return false;
		}
	}

	bool IsUnlockStat(ERollBallSkillStat Stat)
	{
		switch (Stat)
		{
		case ERollBallSkillStat::SawUnlock:
		case ERollBallSkillStat::HammerUnlock:
		case ERollBallSkillStat::BulldozerUnlock:
		case ERollBallSkillStat::DrillUnlock:
		case ERollBallSkillStat::DrillTopMount:
			return true;
		default:
			return false;
		}
	}
}

float FRollBallSkillStats::BaseAmountOf(ERollBallSkillStat Stat)
{
	switch (Stat)
	{
	case ERollBallSkillStat::MaxHealth:         return 1.0f;
	case ERollBallSkillStat::MoveForce:         return 500.0f;
	case ERollBallSkillStat::MaxSpeed:          return 1200.0f;
	case ERollBallSkillStat::KnockbackResist:   return 0.0f;
	case ERollBallSkillStat::PickupRadius:      return 200.0f;

	case ERollBallSkillStat::GoldGain:          return 1.0f;
	case ERollBallSkillStat::RebirthMultiplier: return 1.0f;

	case ERollBallSkillStat::SawSize:           return 1.0f;
	case ERollBallSkillStat::SawDamage:         return 1.0f;

	case ERollBallSkillStat::HammerCount:       return 1.0f;
	case ERollBallSkillStat::HammerKnockback:   return 1.0f;
	case ERollBallSkillStat::HammerSpinSpeed:   return 90.0f;
	case ERollBallSkillStat::HammerDamage:      return 1.0f;

	case ERollBallSkillStat::BulldozerWidth:    return 1.0f;
	case ERollBallSkillStat::BulldozerPush:     return 1.0f;

	case ERollBallSkillStat::DrillDamage:       return 1.0f;
	case ERollBallSkillStat::DrillPierce:       return 0.0f;

	default:                                    return 0.0f;
	}
}

float FRollBallSkillStats::ClampAmount(ERollBallSkillStat Stat, float Amount)
{
	switch (Stat)
	{
	case ERollBallSkillStat::MaxHealth:       return FMath::Clamp(Amount, 1.0f, 7.0f);
	case ERollBallSkillStat::KnockbackResist: return FMath::Clamp(Amount, 0.0f, 0.8f);
	case ERollBallSkillStat::SawSize:         return FMath::Clamp(Amount, 1.0f, 2.0f);
	case ERollBallSkillStat::HammerCount:     return FMath::Clamp(Amount, 1.0f, 4.0f);
	case ERollBallSkillStat::BulldozerWidth:  return FMath::Clamp(Amount, 1.0f, 3.0f);
	default:
		if (IsUnlockStat(Stat))
		{
			return FMath::Clamp(Amount, 0.0f, 1.0f);
		}
		return FMath::Max(0.0f, Amount);
	}
}

FText FRollBallSkillStats::DescribeAmount(ERollBallSkillStat Stat, float Amount)
{
	if (Stat == ERollBallSkillStat::None)
	{
		return FText::GetEmpty();
	}

	const FText StatName = StaticEnum<ERollBallSkillStat>()->GetDisplayNameTextByValue(static_cast<int64>(Stat));

	if (IsUnlockStat(Stat))
	{
		return StatName;
	}

	if (IsMultiplierStat(Stat))
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 1;

		return FText::Format(
			LOCTEXT("SkillAmountPercent", "{0} +{1}%"),
			StatName,
			FText::AsNumber(Amount * 100.0f, &Options));
	}

	if (Stat == ERollBallSkillStat::KnockbackResist)
	{
		return FText::Format(
			LOCTEXT("SkillAmountResist", "{0} +{1}%"),
			StatName,
			FText::AsNumber(FMath::RoundToInt(Amount * 100.0f)));
	}

	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = 1;

	return FText::Format(
		LOCTEXT("SkillAmountFlat", "{0} +{1}"),
		StatName,
		FText::AsNumber(Amount, &Options));
}

#undef LOCTEXT_NAMESPACE
