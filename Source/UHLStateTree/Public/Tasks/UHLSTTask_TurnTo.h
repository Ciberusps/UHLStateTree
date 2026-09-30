// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "Core/UHLAIActorSettings.h"
#include "Data/TurnSettings.h"
#include "UHLSTTask_TurnTo.generated.h"

enum class EStateTreeRunStatus : uint8;
struct FStateTreeTransitionResult;

USTRUCT()
struct UHLSTATETREE_API FUHLSTTask_TurnToInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	// The query will be run with this actor has the owner object.
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(config, Category="Parameter", EditAnywhere, meta = (ClampMin = "0.0", Units="Degrees"))
    float Precision = 1.0f;

    UPROPERTY(EditAnywhere, Category="Parameter")
    bool bUseTurnAnimations = true;
    UPROPERTY(EditAnywhere, Category="Parameter", meta=(EditCondition="bUseTurnAnimations", EditConditionHides))
    EUHLSettingsSource SettingsSource = EUHLSettingsSource::Actor;

    UPROPERTY(EditAnywhere, Category="Parameter", meta=(EditCondition="bUseTurnAnimations && SettingsSource==EUHLSettingsSource::Node", EditConditionHides))
    FTurnSettings TurnSettings;
    UPROPERTY(EditAnywhere, Category="Parameter", meta=(EditCondition="bUseTurnAnimations && SettingsSource==EUHLSettingsSource::DataAsset", EditConditionHides))
    UTurnSettingsDataAsset* RotateToAnimationsDataAsset = nullptr;

    UPROPERTY(EditAnywhere, Category="Parameter")
    bool bDebug = false;
	
	// Anti-overshoot hysteresis: if a turn animation overshoots the goal and the
	// next required turn would reverse direction while the remaining angle is within
	// this tolerance, accept the current heading as "reached" instead of playing an
	// opposite turn (prevents the +-15deg ping-pong near the goal). 0 = disabled.
	UPROPERTY(EditAnywhere, Category="Parameter", meta = (ClampMin = "0.0", Units="Degrees"))
	float OvershootTolerance = 15.0f;

	// TODO don't woks for now
	UPROPERTY()
	bool bInfinite = false;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bClearFocusOnSucceed = true;

	// UPROPERTY(EditAnywhere, Category = "Parameter")
	// bool bFinishTask = true;

    /** cached Precision tangent value */
	UPROPERTY(Transient)
	float PrecisionDot = 0.0f;
	UPROPERTY(Transient)
	FTurnSettings CurrentTurnSettings;
	UPROPERTY(Transient)
	FTurnRange CurrentTurnRange;

	// --- anti-overshoot hysteresis state ---
	// sign (+1 / -1) of the last turn animation that was actually started; 0 = none yet
	UPROPERTY(Transient)
	int32 LastTurnSign = 0;

	// --- best-single-montage state ---
	// true after we committed to & started the single best montage for the whole turn;
	// prevents chaining another (smaller) montage on later ticks while it plays / warps.
	UPROPERTY(Transient)
	bool bBestSingleMontagePlayed = false;

	// --- restore CMC desired-rotation after anim-only turn ---
	UPROPERTY(Transient)
	bool bDesiredRotationDisabled = false;
	UPROPERTY(Transient)
	bool bCachedUseControllerDesiredRotation = false;

	// /** Optional actor where to draw the text at. */
	// UPROPERTY(EditAnywhere, Category = "Input", meta=(Optional))
	// TObjectPtr<AActor> ReferenceActor = nullptr;
};

/**
 * Draws debug text on the HUD associated to the player controller.
 */
USTRUCT(meta = (DisplayName = "Turn To", Category="UHLStateTree"))
struct UHLSTATETREE_API FUHLSTTask_TurnTo : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FUHLSTTask_TurnToInstanceData;

	FUHLSTTask_TurnTo() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
	virtual FName GetIconName() const override
	{
		return FName("StateTreeEditorStyle|Node.Text");
	}
	virtual FColor GetIconColor() const override
	{
		return UE::StateTree::Colors::Grey;
	}
#endif

private:
	FTurnSettings GetTurnSettings(FStateTreeExecutionContext& Context, AActor* Actor) const;
	// returns true if a turn montage was selected & played this call
	bool TryPlayTurnAnimation(FStateTreeExecutionContext& Context, ACharacter* AICharacter, float DeltaAngle) const;
};
