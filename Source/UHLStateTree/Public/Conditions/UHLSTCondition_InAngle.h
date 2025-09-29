// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "GameFramework/Character.h"
#include "UHLSTCondition_InAngle.generated.h"

USTRUCT()
struct UHLSTATETREE_API FUHLSTCondition_InAngleInstanceData
{
	GENERATED_BODY()

	// Context character to measure angles from (forward vector reference).
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character = nullptr;

	// Optional target character. If not valid, Location will be used instead.
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<ACharacter> OtherCharacter = nullptr;

	// Fallback location when OtherCharacter is not provided/invalid.
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector Location = FVector::ZeroVector;

	// Set of angle ranges in degrees relative to Character forward (+right is positive).
	// If any of the ranges contains the signed angle, the test passes.
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TArray<FFloatRange> Ranges;

	// If true, result is inverted.
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInverse = false;

	// Enable debug visualization.
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebug = false;

	// Debug draw duration in seconds. Active only if bDebug is true.
	UPROPERTY(EditAnywhere, Category = "Debug", meta=(ClampMin="0.0", EditCondition="bDebug"))
	float DebugDuration = 2.5f;

	// Optional editor-only description suffix.
	UPROPERTY(EditAnywhere, Category = "Parameter", meta=(MultiLine=true))
	FString Comment;
};

/**
 * InAngle condition. Tests if signed yaw to OtherCharacter or Location is within any given ranges.
 */
USTRUCT(meta = (DisplayName="In Angles", Category = "UHLStateTree"))
struct UHLSTATETREE_API FUHLSTCondition_InAngle : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FUHLSTCondition_InAngleInstanceData;

	FUHLSTCondition_InAngle() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FUHLSTCondition_InAngleInstanceData::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
	virtual FName GetIconName() const override { return FName("StateTreeEditorStyle|Node.Movement"); }
	virtual FColor GetIconColor() const override { return UE::StateTree::Colors::DarkYellow; }
#endif
};



