// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "StateTreeConditionBase.h"
#include "UHLSTCondition_TagCooldown.generated.h"

USTRUCT()
struct UHLSTATETREE_API FUHLSTCondition_TagCooldownInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag CooldownTag;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInverse = false;
};

/**
 * HasTagCooldown condition
 */
USTRUCT(meta = (DisplayName="Has NO Tag Cooldown", Category="UHLStateTree"))
struct UHLSTATETREE_API FUHLSTCondition_TagCooldown : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FUHLSTCondition_TagCooldownInstanceData;

	FUHLSTCondition_TagCooldown() = default;
	
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
	virtual FName GetIconName() const override { return FName("StateTreeEditorStyle|Node.Time"); }
	virtual FColor GetIconColor() const override { return UE::StateTree::Colors::DarkCyan; }
#endif
};
