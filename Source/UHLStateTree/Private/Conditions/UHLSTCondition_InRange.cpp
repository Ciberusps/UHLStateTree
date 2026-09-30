// Pavel Penkov 2025 All Rights Reserved.

#include "Conditions/UHLSTCondition_InRange.h"

#include "StateTreeExecutionContext.h"
#include "StateTreeNodeDescriptionHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Internationalization/Internationalization.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UHLSTCondition_InRange)

#define LOCTEXT_NAMESPACE "UHLSTCondition_InRange"

namespace UHLSTConditionInRange_Private
{
	static float GetCapsuleRadiusSafe(const ACharacter* Character)
	{
		if (!Character)
		{
			return 0.0f;
		}
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			return Capsule->GetScaledCapsuleRadius();
		}
		return 0.0f;
	}

	/**
	 * Keeps CachedCharacter aligned with SourceActor: repeats Cast<ACharacter> only when the bound actor pointer changes.
	 */
	static void SyncCharacterCache(
		AActor* const SourceActor,
		TObjectPtr<AActor>& CachedKey,
		TObjectPtr<ACharacter>& CachedCharacter)
	{
		if (CachedKey.Get() == SourceActor)
		{
			return;
		}
		CachedKey = SourceActor;
		CachedCharacter = IsValid(SourceActor) ? Cast<ACharacter>(SourceActor) : nullptr;
	}
}

bool FUHLSTCondition_InRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.Character))
	{
		return false;
	}

	UHLSTConditionInRange_Private::SyncCharacterCache(
		InstanceData.Character,
		InstanceData.CachedSelfActorKey,
		InstanceData.CachedSelfCharacter);

	const bool bUsesOtherActor = IsValid(InstanceData.OtherCharacter);
	if (bUsesOtherActor)
	{
		UHLSTConditionInRange_Private::SyncCharacterCache(
			InstanceData.OtherCharacter,
			InstanceData.CachedOtherActorKey,
			InstanceData.CachedOtherCharacter);
	}
	else
	{
		InstanceData.CachedOtherActorKey = nullptr;
		InstanceData.CachedOtherCharacter = nullptr;
	}

	const FVector SelfLocation = InstanceData.Character->GetActorLocation();

	FVector TargetLocation = InstanceData.Location;
	if (bUsesOtherActor)
	{
		TargetLocation = InstanceData.OtherCharacter->GetActorLocation();
	}

	float Distance = FVector::Dist(SelfLocation, TargetLocation);

	ACharacter* const SelfAsCharacter = InstanceData.CachedSelfCharacter.Get();
	if (InstanceData.bIncludeSelfCapsuleRadius && SelfAsCharacter)
	{
		Distance -= UHLSTConditionInRange_Private::GetCapsuleRadiusSafe(SelfAsCharacter);
	}

	ACharacter* const OtherAsCharacter = InstanceData.CachedOtherCharacter.Get();
	if (bUsesOtherActor && InstanceData.bIncludeTargetCapsuleRadius && OtherAsCharacter)
	{
		Distance -= UHLSTConditionInRange_Private::GetCapsuleRadiusSafe(OtherAsCharacter);
	}

	Distance = FMath::Max(0.0f, Distance);

	// Evaluate against range with open/closed bounds respected.
	bool bInRange = true;

	if (InstanceData.Range.HasLowerBound())
	{
		const FFloatRangeBound& Lower = InstanceData.Range.GetLowerBound();
		const float Min = Lower.GetValue();
		bInRange &= (Lower.IsInclusive()) ? (Distance >= Min) : (Distance > Min);
	}

	if (InstanceData.Range.HasUpperBound())
	{
		const FFloatRangeBound& Upper = InstanceData.Range.GetUpperBound();
		const float Max = Upper.GetValue();
		bInRange &= (Upper.IsInclusive()) ? (Distance <= Max) : (Distance < Max);
	}

	const bool bFinal = InstanceData.bInverse ? !bInRange : bInRange;

	// Debug visualization
	if (InstanceData.bDebug && InstanceData.DebugDuration > 0.0f)
	{
		UWorld* World = InstanceData.Character->GetWorld();
		if (World)
		{
			// Use actor locations (capsule centers) to avoid vertical offset above the head
			const FVector SelfCenter = SelfLocation;
			FVector OtherCenter = TargetLocation;
			if (bUsesOtherActor)
			{
				OtherCenter = InstanceData.OtherCharacter->GetActorLocation();
			}

			// Compute arrow endpoints on capsule edges along the direction vector
			FVector Dir = (OtherCenter - SelfCenter);
			const float DirLen = Dir.Length();
			if (DirLen > KINDA_SMALL_NUMBER)
			{
				Dir /= DirLen;
			}

			const float SelfOffset = (InstanceData.bIncludeSelfCapsuleRadius && SelfAsCharacter)
				? UHLSTConditionInRange_Private::GetCapsuleRadiusSafe(SelfAsCharacter)
				: 0.0f;
			const float OtherOffset = (bUsesOtherActor && InstanceData.bIncludeTargetCapsuleRadius && OtherAsCharacter)
				? UHLSTConditionInRange_Private::GetCapsuleRadiusSafe(OtherAsCharacter)
				: 0.0f;

			const FVector Start = SelfCenter + Dir * SelfOffset;
			const FVector End = OtherCenter - Dir * OtherOffset;

			const FColor LineColor = bFinal ? FColor::Green : FColor::Red;
			DrawDebugDirectionalArrow(World, Start, End, 25.0f, LineColor, false, InstanceData.DebugDuration, 0, 2.0f);

			// Text with distance and parameters near the middle point
			const FVector Mid = (Start + End) * 0.5f + FVector(0, 0, 20.0f);
			const FString Params = FString::Printf(
				TEXT("dist=%.0f cm  range[%s%s, %s%s]  selfCaps=%s  targetCaps=%s  inverse=%s"),
				Distance,
				InstanceData.Range.HasLowerBound() ? (InstanceData.Range.GetLowerBound().IsInclusive() ? TEXT("[") : TEXT("(")) : TEXT("("),
				InstanceData.Range.HasLowerBound() ? *FString::SanitizeFloat(InstanceData.Range.GetLowerBoundValue()) : TEXT("-"),
				InstanceData.Range.HasUpperBound() ? *FString::SanitizeFloat(InstanceData.Range.GetUpperBoundValue()) : TEXT("-"),
				InstanceData.Range.HasUpperBound() ? (InstanceData.Range.GetUpperBound().IsInclusive() ? TEXT("]") : TEXT(")")) : TEXT(")"),
				InstanceData.bIncludeSelfCapsuleRadius ? TEXT("on") : TEXT("off"),
				InstanceData.bIncludeTargetCapsuleRadius ? TEXT("on") : TEXT("off"),
				InstanceData.bInverse ? TEXT("on") : TEXT("off")
			);
			DrawDebugString(World, Mid, Params, nullptr, LineColor, InstanceData.DebugDuration, true);

			// Visualize min/max effective ranges as circles around self center
			const float SelfOffsetForRing = (InstanceData.bIncludeSelfCapsuleRadius && SelfAsCharacter)
				? UHLSTConditionInRange_Private::GetCapsuleRadiusSafe(SelfAsCharacter)
				: 0.0f;
			const float OtherOffsetForRing = (bUsesOtherActor && InstanceData.bIncludeTargetCapsuleRadius && OtherAsCharacter)
				? UHLSTConditionInRange_Private::GetCapsuleRadiusSafe(OtherAsCharacter)
				: 0.0f;
			const int32 Segments = 48;
			if (InstanceData.Range.HasLowerBound())
			{
				const float MinAdj = FMath::Max(0.0f, InstanceData.Range.GetLowerBoundValue() + SelfOffsetForRing + OtherOffsetForRing);
				DrawDebugCircle(World, SelfCenter, MinAdj, Segments, FColor::Yellow, false, InstanceData.DebugDuration, 0, 1.5f, FVector(1,0,0), FVector(0,1,0), false);
			}
			if (InstanceData.Range.HasUpperBound())
			{
				const float MaxAdj = FMath::Max(0.0f, InstanceData.Range.GetUpperBoundValue() + SelfOffsetForRing + OtherOffsetForRing);
				DrawDebugCircle(World, SelfCenter, MaxAdj, Segments, FColor::Cyan, false, InstanceData.DebugDuration, 0, 1.5f, FVector(1,0,0), FVector(0,1,0), false);
			}

			// Mark the arrow endpoints
			DrawDebugSphere(World, Start, 6.0f, 8, LineColor, false, InstanceData.DebugDuration, 0, 1.0f);
			DrawDebugSphere(World, End, 6.0f, 8, LineColor, false, InstanceData.DebugDuration, 0, 1.0f);
		}
	}

	return bFinal;
}

#if WITH_EDITOR
static FText FormatRangeText(const FFloatRange& Range)
{
	const bool bHasLower = Range.HasLowerBound();
	const bool bHasUpper = Range.HasUpperBound();

	FNumberFormattingOptions NumberFmt;
	NumberFmt.MinimumFractionalDigits = 1;
	NumberFmt.MaximumFractionalDigits = 1;

	FString MinStr = bHasLower ? (FText::AsNumber(Range.GetLowerBoundValue() / 100.0f, &NumberFmt).ToString() + TEXT("m")) : TEXT("-");
	FString MaxStr = bHasUpper ? (FText::AsNumber(Range.GetUpperBoundValue() / 100.0f, &NumberFmt).ToString() + TEXT("m")) : TEXT("-");

	return FText::FromString(FString::Printf(TEXT("[%s, %s]"), *MinStr, *MaxStr));
}

FText FUHLSTCondition_InRange::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// Treat property as set if either a direct value is provided or there is a binding
	const FPropertyBindingPath TargetPath(ID, GET_MEMBER_NAME_CHECKED(FUHLSTCondition_InRangeInstanceData, OtherCharacter));
	const bool bIsOtherActorBound = !BindingLookup.GetBindingSourceDisplayName(TargetPath).IsEmpty();
	const bool bHasTargetActor = InstanceData->OtherCharacter != nullptr || bIsOtherActorBound;

	FText Prefix;
	if (bHasTargetActor)
	{
		Prefix = InstanceData->bInverse
			? LOCTEXT("NotInRangeEnemyPrefix", "Enemy is NOT in range ")
			: LOCTEXT("InRangeEnemyPrefix", "Enemy is in range ");
	}
	else
	{
		Prefix = InstanceData->bInverse
			? LOCTEXT("NotInRangeLocationPrefix", "Location is NOT in range ")
			: LOCTEXT("InRangeLocationPrefix", "Location is in range ");
	}

	FText RangeText = FormatRangeText(InstanceData->Range);

	FText Text = FText::Format(FText::FromString(TEXT("{0}{1}")), Prefix, RangeText);
	return Text;
}
#endif


#undef LOCTEXT_NAMESPACE
