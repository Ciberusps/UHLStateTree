// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "StateTreeTaskBase.h"
#include "UObject/NameTypes.h"
#include "UHLSTTask_PlayAnimMontage.generated.h"

class ACharacter;
class USkeletalMeshComponent;
class UAnimMontage;
class UUHLStateTreeAIComponent;

enum class EStateTreeRunStatus : uint8;
struct FStateTreeTransitionResult;

USTRUCT()
struct UHLSTATETREE_API FUHLSTTask_PlayAnimMontageInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character = nullptr;

	/** Optional override skeletal mesh to play montage on. If not set, character's mesh will be used. */
	UPROPERTY(EditAnywhere, Category = "Components", meta=(Optional))
	TObjectPtr<USkeletalMeshComponent> CustomMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UAnimMontage> AnimMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float StartingPosition = 0.0f;

	/** Optional starting section name. If none, montage will play from default or position. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName StartingSection = NAME_None;

	/** If true, will stop all montages on the mesh before playing. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bShouldStopAllMontages = false;

	/** If true, finish task when montage completes naturally. */
	UPROPERTY(EditAnywhere, Category = "Finish")
	bool bFinishTaskOnCompleted = true;

	/** If true, finish task when montage is interrupted. */
	UPROPERTY(EditAnywhere, Category = "Finish")
	bool bFinishTaskOnInterrupted = true;

	/** If true, finish task when montage starts blending out. */
	UPROPERTY(EditAnywhere, Category = "Finish")
	bool bFinishTaskOnBlendOut = true;

	/** Result status when finishing (Succeeded if true, otherwise Failed). */
	UPROPERTY(EditAnywhere, Category = "Finish")
	bool bSucceededResult = true;

	// Runtime flags set by delegates
	UPROPERTY(Transient)
	bool bCompletedTriggered = false;
	UPROPERTY(Transient)
	bool bInterruptedTriggered = false;
	UPROPERTY(Transient)
	bool bBlendOutTriggered = false;

	/** Delegates are bound on this AnimInstance to be cleared on exit */
	UPROPERTY(Transient)
	TWeakObjectPtr<UAnimInstance> BoundAnimInstance;

	/** Owning StateTree component for RequestTransition. */
	UPROPERTY(Transient)
	TWeakObjectPtr<UUHLStateTreeAIComponent> StateTreeComponent;
};

/**
 * Play Anim Montage on Character (or custom mesh).
 */
USTRUCT(meta = (DisplayName = "Play AnimMontage", Category = "UHLStateTree"))
struct UHLSTATETREE_API FUHLSTTask_PlayAnimMontage : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FUHLSTTask_PlayAnimMontageInstanceData;

	FUHLSTTask_PlayAnimMontage() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	// Lightweight tick: only checks flags set by montage delegates to finish the task.
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
	virtual FName GetIconName() const override { return FName("StateTreeEditorStyle|Node.Animation"); }
	virtual FColor GetIconColor() const override { return FColor(92, 0, 148); }
#endif

private:
	USkeletalMeshComponent* ResolveMesh(const FInstanceDataType& InstanceData) const;
	bool IsMontagePlaying(USkeletalMeshComponent* Mesh, const UAnimMontage* Montage) const;
};


