// Pavel Penkov 2025 All Rights Reserved.

#include "Conditions/UHLSTCondition_TagCooldown.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeNodeDescriptionHelpers.h"
#include "Components/UHLStateTreeAIComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UHLSTCondition_TagCooldown)

#define LOCTEXT_NAMESPACE "UHLSTCondition_TagCooldown"

bool FUHLSTCondition_TagCooldown::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	if (!AIController) return false;
	
	if (UUHLStateTreeAIComponent* Cmp = Cast<UUHLStateTreeAIComponent>(AIController->GetBrainComponent()))
	{
		bool bResult = Cmp->TagCooldowns.HasCooldownFinished(Context.GetOwner(), InstanceData.CooldownTag);
		return InstanceData.bInverse ? !bResult : bResult;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, TEXT("[UHLStateTreeSetCooldownTask] using UUHLStateTreeAIComponent required to use SetCooldownTask"));
	}
	
	return false;
}

#if WITH_EDITOR
FText FUHLSTCondition_TagCooldown::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText Format = (Formatting == EStateTreeNodeFormatting::RichText)
		? LOCTEXT("GameplayTagMatchRich", "Has {NO }cooldown for {CooldownTag}")
		: LOCTEXT("GameplayTagMatch", "No {NO }cooldown for {CooldownTag}");

	return FText::FormatNamed(Format,
		TEXT("CooldownTag"), FText::FromString(InstanceData->CooldownTag.ToString()),
		TEXT("NO "), FText::FromString(InstanceData->bInverse ? "" : "NO "));
}
#endif

#undef LOCTEXT_NAMESPACE
