// Pavel Penkov 2025 All Rights Reserved.

#include "Conditions/UHLSTCondition_InAngle.h"

#include "StateTreeExecutionContext.h"
#include "StateTreeNodeDescriptionHelpers.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UHLSTCondition_InAngle)

#define LOCTEXT_NAMESPACE "UHLSTCondition_InAngle"

namespace
{
	static float ComputeSignedYawDegrees(const FVector& FromLocation, const FRotator& FromRotation, const FVector& ToLocation)
	{
		const FVector ToDir = (ToLocation - FromLocation).GetSafeNormal();
		const FVector Forward = FromRotation.Vector();
		const FVector Right = FRotationMatrix(FromRotation).GetUnitAxis(EAxis::Y);
		const float X = FVector::DotProduct(ToDir, Forward);
		const float Y = FVector::DotProduct(ToDir, Right);
		const float AngleRad = FMath::Atan2(Y, X);
		return FMath::RadiansToDegrees(AngleRad);
	}

	static bool IsAngleInRange(float AngleDeg, const FFloatRange& Range)
	{
		// Normalize to [-180, 180)
		AngleDeg = FMath::UnwindDegrees(AngleDeg);

		// Range values are assumed in degrees as given by user.
		bool bOk = true;
		if (Range.HasLowerBound())
		{
			const FFloatRangeBound& L = Range.GetLowerBound();
			const float Min = L.GetValue();
			bOk &= L.IsInclusive() ? (AngleDeg >= Min) : (AngleDeg > Min);
		}
		if (Range.HasUpperBound())
		{
			const FFloatRangeBound& U = Range.GetUpperBound();
			const float Max = U.GetValue();
			bOk &= U.IsInclusive() ? (AngleDeg <= Max) : (AngleDeg < Max);
		}
		return bOk;
	}
}

bool FUHLSTCondition_InAngle::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.Character))
	{
		return false;
	}

	const FVector SelfLocation = InstanceData.Character->GetActorLocation();
	const FRotator SelfRotation = InstanceData.Character->GetActorRotation();

	FVector TargetLocation = InstanceData.Location;
	if (IsValid(InstanceData.OtherActor))
	{
		TargetLocation = InstanceData.OtherActor->GetActorLocation();
	}

	const float SignedYaw = ComputeSignedYawDegrees(SelfLocation, SelfRotation, TargetLocation);

	bool bInAny = false;
	if (InstanceData.Ranges.Num() == 0)
	{
		// No ranges configured is considered failure for clarity
		bInAny = false;
	}
	else
	{
		for (const FFloatRange& Range : InstanceData.Ranges)
		{
			if (IsAngleInRange(SignedYaw, Range))
			{
				bInAny = true;
				break;
			}
		}
	}

	const bool bFinal = InstanceData.bInverse ? !bInAny : bInAny;

	if (InstanceData.bDebug && InstanceData.DebugDuration > 0.0f)
	{
		if (UWorld* World = InstanceData.Character->GetWorld())
		{
			const FColor Col = bFinal ? FColor::Green : FColor::Red;
			// Draw character forward and to-target direction
			const FVector Forward = InstanceData.Character->GetActorForwardVector();
			const FVector ToTarget = (TargetLocation - SelfLocation).GetSafeNormal();
			const float Len = 150.0f;
			DrawDebugDirectionalArrow(World, SelfLocation, SelfLocation + Forward * Len, 20.0f, FColor::Cyan, false, InstanceData.DebugDuration, 0, 2.0f);
			DrawDebugDirectionalArrow(World, SelfLocation, SelfLocation + ToTarget * Len, 20.0f, Col, false, InstanceData.DebugDuration, 0, 2.0f);

			// Compose ranges text
			FString RangesStr;
			for (int32 i = 0; i < InstanceData.Ranges.Num(); ++i)
			{
				const FFloatRange& R = InstanceData.Ranges[i];
				RangesStr += FString::Printf(TEXT("%s%s, %s%s"),
					R.HasLowerBound() ? (R.GetLowerBound().IsInclusive() ? TEXT("[") : TEXT("(")) : TEXT("("),
					R.HasLowerBound() ? *FString::SanitizeFloat(R.GetLowerBoundValue()) : TEXT("-"),
					R.HasUpperBound() ? *FString::SanitizeFloat(R.GetUpperBoundValue()) : TEXT("-"),
					R.HasUpperBound() ? (R.GetUpperBound().IsInclusive() ? TEXT("]") : TEXT(")")) : TEXT(")"));
				if (i + 1 < InstanceData.Ranges.Num())
				{
					RangesStr += TEXT("; ");
				}
			}
			const FString Msg = FString::Printf(TEXT("yaw=%.1f inAny=%s ranges=%s%s"), SignedYaw, bInAny ? TEXT("true") : TEXT("false"), *RangesStr, InstanceData.bInverse ? TEXT(" inverted") : TEXT(""));
			DrawDebugString(World, SelfLocation + FVector(0, 0, 120.0f), Msg, nullptr, Col, InstanceData.DebugDuration, true);
		}
	}

	return bFinal;
}

#if WITH_EDITOR
static FText FormatAngleRangeText(const FFloatRange& Range)
{
	const bool bHasLower = Range.HasLowerBound();
	const bool bHasUpper = Range.HasUpperBound();
	FString MinStr = bHasLower ? (FText::AsNumber(Range.GetLowerBoundValue()).ToString() + TEXT("°")) : TEXT("-");
	FString MaxStr = bHasUpper ? (FText::AsNumber(Range.GetUpperBoundValue()).ToString() + TEXT("°")) : TEXT("-");
	return FText::FromString(FString::Printf(TEXT("[%s, %s]"), *MinStr, *MaxStr));
}

FText FUHLSTCondition_InAngle::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FPropertyBindingPath TargetPath(ID, GET_MEMBER_NAME_CHECKED(FUHLSTCondition_InAngleInstanceData, OtherActor));
	const bool bIsOtherCharacterBound = !BindingLookup.GetBindingSourceDisplayName(TargetPath).IsEmpty();
	const bool bHasTargetCharacter = InstanceData->OtherActor != nullptr || bIsOtherCharacterBound;

	FText Prefix;
	if (bHasTargetCharacter)
	{
		Prefix = InstanceData->bInverse
			? LOCTEXT("NotInAnglesEnemyPrefix", "Enemy is NOT in angles ")
			: LOCTEXT("InAnglesEnemyPrefix", "Enemy is in angles ");
	}
	else
	{
		Prefix = InstanceData->bInverse
			? LOCTEXT("NotInAnglesLocationPrefix", "Location is NOT in angles ")
			: LOCTEXT("InAnglesLocationPrefix", "Location is in angles ");
	}

	FString Combined;
	for (int32 i = 0; i < InstanceData->Ranges.Num(); ++i)
	{
		Combined += FormatAngleRangeText(InstanceData->Ranges[i]).ToString();
		if (i + 1 < InstanceData->Ranges.Num())
		{
			Combined += TEXT(" ");
		}
	}

	FText Text = FText::Format(FText::FromString(TEXT("{0}{1}")), Prefix, FText::FromString(Combined));
	return Text;
}
#endif



#undef LOCTEXT_NAMESPACE
