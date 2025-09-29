// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "UHLSTTask_SetCooldown.generated.h"

enum class EStateTreeRunStatus : uint8;
struct FStateTreeTransitionResult;

USTRUCT()
struct UHLSTATETREE_API FUHLSTTask_SetCooldownInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag CooldownTag;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Duration = 0.0f;

	/** True if we are adding to any existing duration, false if we are setting the duration (potentially invalidating an existing end time). */
	UPROPERTY(EditAnywhere, Category = "Parameter", DisplayName = AddToExistingDuration)
	bool bAddToExistingDuration = false;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bFinishTask = true;

	// /** Optional actor where to draw the text at. */
	// UPROPERTY(EditAnywhere, Category = "Input", meta=(Optional))
	// TObjectPtr<AActor> ReferenceActor = nullptr;
};

/**
 * Draws debug text on the HUD associated to the player controller.
 */
USTRUCT(meta = (DisplayName = "Set Cooldown", Category="UHLStateTree"))
struct UHLSTATETREE_API FUHLSTTask_SetCooldown : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FUHLSTTask_SetCooldownInstanceData;

	FUHLSTTask_SetCooldown() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
	virtual FName GetIconName() const override { return FName("StateTreeEditorStyle|Node.Time"); }
	virtual FColor GetIconColor() const override { return UE::StateTree::Colors::DarkCyan; }
#endif
};
