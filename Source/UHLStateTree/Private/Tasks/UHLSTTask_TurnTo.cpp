// Pavel Penkov 2025 All Rights Reserved.

#include "Tasks/UHLSTTask_TurnTo.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "UHLAIBlueprintLibrary.h"
#include "Core/UHLAIActorSettings.h"
#include "UHLStateTree.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UHLSTTask_TurnTo)

#define LOCTEXT_NAMESPACE "UHLSTTask_TurnTo"

namespace TurnToStatics
{
	static FString GetSettingsSourceName(EUHLSettingsSource Source)
	{
		switch (Source)
		{
			case EUHLSettingsSource::Actor:     return TEXT("Actor");
			case EUHLSettingsSource::DataAsset: return TEXT("DataAsset");
			case EUHLSettingsSource::Node:      return TEXT("Node");
			default:                            return TEXT("Unknown");
		}
	}

	static int32 CountTurnRanges(const FTurnSettings& TurnSettings)
	{
		int32 Count = 0;
		for (const TTuple<FString, FTurnRanges>& Group : TurnSettings.TurnRangesGroups)
		{
			Count += Group.Value.TurnRanges.Num();
		}
		return Count;
	}

	// Always logs to LogUHLStateTree; prints on screen only when bDebug (stable key => updates in place).
	static void Report(bool bDebug, const AActor* Pawn, const FString& Key, const FColor& Color, const FString& Message)
	{
		const FString PawnName = Pawn ? Pawn->GetName() : TEXT("NoPawn");
		UE_LOG(LogUHLStateTree, Log, TEXT("[TurnTo][%s] %s"), *PawnName, *Message);
		if (bDebug && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				(uint64)GetTypeHash(Key + PawnName), 5.0f, Color,
				FString::Printf(TEXT("[TurnTo][%s] %s"), *PawnName, *Message));
		}
	}
}

EStateTreeRunStatus FUHLSTTask_TurnTo::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	// fresh activation - forget previous turn direction (anti-oscillation state)
	InstanceData.LastTurnSign = 0;
	// fresh activation - allow a new best-single montage to be chosen
	InstanceData.bBestSingleMontagePlayed = false;
	EStateTreeRunStatus Result = InstanceData.bInfinite
									? EStateTreeRunStatus::Running
									: EStateTreeRunStatus::Failed;

	const UWorld* World = Context.GetWorld();
	// if (World == nullptr && InstanceData.ReferenceActor != nullptr)
	// {
	// 	World = InstanceData.ReferenceActor->GetWorld();
	// }

	// Reference actor is not required (offset will be used as a global world location)
	// but a valid world is required.
	if (World == nullptr)
	{
		return InstanceData.bInfinite
				? EStateTreeRunStatus::Running
				: EStateTreeRunStatus::Failed;
	}

	AAIController* AIController = InstanceData.AIController;
	if (!AIController) return InstanceData.bInfinite
						? EStateTreeRunStatus::Running
						: EStateTreeRunStatus::Failed;

	// GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, TEXT("[UHLStateTreeTaskTurnTo] using UUHLStateTreeAIComponent required to use SetCooldownTask"));

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return InstanceData.bInfinite
						? EStateTreeRunStatus::Running
						: EStateTreeRunStatus::Failed;

	const FVector PawnLocation = Pawn->GetActorLocation();
	InstanceData.PrecisionDot = FMath::Cos(FMath::DegreesToRadians(InstanceData.Precision));

	if (InstanceData.TargetActor)
	{
		AActor* ActorValue = InstanceData.TargetActor;

		if (ActorValue != NULL)
		{
			const FVector::FReal AngleDifference = TurnToStatics::CalculateAngleDifferenceDot(Pawn->GetActorForwardVector()
				, (ActorValue->GetActorLocation() - PawnLocation));

			if (AngleDifference >= InstanceData.PrecisionDot)
			{
				Result = InstanceData.bInfinite
						? EStateTreeRunStatus::Running
						: EStateTreeRunStatus::Succeeded;
			}
			else
			{
				AIController->SetFocus(ActorValue, EAIFocusPriority::Gameplay);
				if (Pawn->GetClass()->ImplementsInterface(UUHLAIActorSettings::StaticClass()))
				{
					InstanceData.CurrentTurnSettings = GetTurnSettings(Context, Pawn);
				}
				// start the turn montage NOW, before CMC desired-rotation eats the angle on the enter hitch
				if (ACharacter* EnterCharacter = AIController->GetCharacter())
				{
					const float EnterDeltaAngle = UUHLAIBlueprintLibrary::RelativeAngleToActor(EnterCharacter, ActorValue);
					TryPlayTurnAnimation(Context, EnterCharacter, EnterDeltaAngle);
					if (InstanceData.CurrentTurnSettings.bTurnOnlyWithAnims)
					{
						if (UCharacterMovementComponent* Move = EnterCharacter->GetCharacterMovement())
						{
							InstanceData.bCachedUseControllerDesiredRotation = Move->bUseControllerDesiredRotation;
							Move->bUseControllerDesiredRotation = false;
							InstanceData.bDesiredRotationDisabled = true;
						}
					}
				}
				Result = EStateTreeRunStatus::Running;
			}
		}
	}
    // TODO add support for Vectors/Rotators
	else
	{
		if (FAISystem::IsValidLocation(InstanceData.TargetLocation))
		{
			const FVector::FReal AngleDifference = TurnToStatics::CalculateAngleDifferenceDot(Pawn->GetActorForwardVector()
				, (InstanceData.TargetLocation - PawnLocation));

			if (AngleDifference >= InstanceData.PrecisionDot)
			{
				Result = InstanceData.bInfinite
						? EStateTreeRunStatus::Running
						: EStateTreeRunStatus::Succeeded;
			}
			else
			{
				AIController->SetFocalPoint(InstanceData.TargetLocation, EAIFocusPriority::Gameplay);
				if (Pawn->GetClass()->ImplementsInterface(UUHLAIActorSettings::StaticClass()))
				{
					InstanceData.CurrentTurnSettings = GetTurnSettings(Context, Pawn);
				}
				if (ACharacter* EnterCharacter = AIController->GetCharacter())
				{
					const float EnterDeltaAngle = UUHLAIBlueprintLibrary::RelativeAngleToVector(EnterCharacter, InstanceData.TargetLocation);
					TryPlayTurnAnimation(Context, EnterCharacter, EnterDeltaAngle);
					if (InstanceData.CurrentTurnSettings.bTurnOnlyWithAnims)
					{
						if (UCharacterMovementComponent* Move = EnterCharacter->GetCharacterMovement())
						{
							InstanceData.bCachedUseControllerDesiredRotation = Move->bUseControllerDesiredRotation;
							Move->bUseControllerDesiredRotation = false;
							InstanceData.bDesiredRotationDisabled = true;
						}
					}
				}
				Result = EStateTreeRunStatus::Running;
			}
		}
	}
	// else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Rotator::StaticClass())
	// {
	// 	const FRotator KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Rotator>(BlackboardKey.GetSelectedKeyID());
	//
	// 	if (FAISystem::IsValidRotation(KeyValue))
	// 	{
	// 		const FVector DirectionVector = KeyValue.Vector();
	// 		const FVector::FReal AngleDifference = CalculateAngleDifferenceDot(Pawn->GetActorForwardVector(), DirectionVector);
	//
	// 		if (AngleDifference >= PrecisionDot)
	// 		{
	// 			Result = EBTNodeResult::Succeeded;
	// 		}
	// 		else
	// 		{
	// 			const FVector FocalPoint = PawnLocation + DirectionVector * 10000.0f;
	// 			// set focal somewhere far in the indicated direction
	// 			AIController->SetFocalPoint(FocalPoint, EAIFocusPriority::Gameplay);
	// 			MyMemory->FocusLocationSet = FocalPoint;
	// 			Result = EBTNodeResult::InProgress;
	// 		}
	// 	}
	// }

	TurnToStatics::Report(InstanceData.bDebug, Pawn, TEXT("Enter"), FColor::White,
		FString::Printf(TEXT("Enter: Target=%s | Source=%s | DataAsset=%s | ImplementsActorSettings=%s | TurnRanges=%d | bTurnOnlyWithAnims=%d | bStopMontageOnGoalReached=%d | Result=%d"),
			InstanceData.TargetActor ? *InstanceData.TargetActor->GetName() : *InstanceData.TargetLocation.ToString(),
			*TurnToStatics::GetSettingsSourceName(InstanceData.SettingsSource),
			InstanceData.RotateToAnimationsDataAsset ? *InstanceData.RotateToAnimationsDataAsset->GetName() : TEXT("None"),
			Pawn->GetClass()->ImplementsInterface(UUHLAIActorSettings::StaticClass()) ? TEXT("true") : TEXT("false"),
			TurnToStatics::CountTurnRanges(InstanceData.CurrentTurnSettings),
			InstanceData.CurrentTurnSettings.bTurnOnlyWithAnims,
			InstanceData.CurrentTurnSettings.bStopMontageOnGoalReached,
			(int32)Result));

	return Result;
}

EStateTreeRunStatus FUHLSTTask_TurnTo::Tick(
	FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const UWorld* World = Context.GetWorld();
	// if (World == nullptr && InstanceData.ReferenceActor != nullptr)
	// {
	// 	World = InstanceData.ReferenceActor->GetWorld();
	// }

	// Reference actor is not required (offset will be used as a global world location)
	// but a valid world is required.
	if (World == nullptr)
	{
		return InstanceData.bInfinite
				? EStateTreeRunStatus::Running
				: EStateTreeRunStatus::Failed;
	}

	AAIController* AIController = InstanceData.AIController;
	if (!AIController || !AIController->GetPawn())
	{
		return InstanceData.bInfinite
				? EStateTreeRunStatus::Running
				: EStateTreeRunStatus::Failed;
	}

	// target enemy if its infinite task
	if (InstanceData.bInfinite
		&& InstanceData.TargetActor
		&& AIController->GetFocusActorForPriority(EAIFocusPriority::Gameplay) != InstanceData.TargetActor)
	{
		AIController->SetFocus(InstanceData.TargetActor, EAIFocusPriority::Gameplay);
	}
	const FVector PawnDirection = AIController->GetPawn()->GetActorForwardVector();
   	const FVector FocalPoint = AIController->GetFocalPointForPriority(EAIFocusPriority::Gameplay);
    ACharacter* AICharacter = AIController->GetCharacter();

	if (FocalPoint != FAISystem::InvalidLocation)
	{
	    float DeltaAngleRad = TurnToStatics::CalculateAngleDifferenceDot(PawnDirection, FocalPoint - AIController->GetPawn()->GetActorLocation());
	    // float DeltaAngle = FMath::RadiansToDegrees(FMath::Acos(DeltaAngleRad));
	    float DeltaAngle = InstanceData.TargetActor
			? UUHLAIBlueprintLibrary::RelativeAngleToActor(AICharacter, InstanceData.TargetActor)
			: UUHLAIBlueprintLibrary::RelativeAngleToVector(AICharacter, InstanceData.TargetLocation);

		UCharacterMovementComponent* Move = AICharacter ? AICharacter->GetCharacterMovement() : nullptr;
		TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("TickAngle"), FColor::Magenta,
			FString::Printf(TEXT("DeltaAngle=%.1f | RotRate.Yaw=%.0f | bUseCtrlDesiredRot=%d | bOrientToMove=%d | bUseCtrlRotYaw=%d | RootMotion=%d"),
				DeltaAngle,
				Move ? Move->RotationRate.Yaw : -1.f,
				Move ? (int32)Move->bUseControllerDesiredRotation : -1,
				Move ? (int32)Move->bOrientRotationToMovement : -1,
				AICharacter ? (int32)AICharacter->bUseControllerRotationYaw : -1,
				AICharacter ? (int32)AICharacter->IsPlayingRootMotion() : -1));

		if (InstanceData.bDebug)
		{
			FString Message = FString::Printf(TEXT("DeltaAngle %f"), DeltaAngle);
			UKismetSystemLibrary::PrintString(nullptr, Message, true, true, FColor::Green, 5.0f, "DeltaAngle");
			FVector CurrentLocation = InstanceData.TargetActor
				? InstanceData.TargetActor->GetActorLocation()
				: InstanceData.TargetLocation;
			DrawDebugSphere(AIController->GetWorld(), CurrentLocation,
				50.0f, 12, FColor::Blue, false, -1);
		}

		if (DeltaAngleRad >= InstanceData.PrecisionDot)
		{
			if (InstanceData.bDebug && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("TurnRange->bOverrideStopMontageOnGoalReached %hhd"), InstanceData.CurrentTurnRange.bOverrideStopMontageOnGoalReached));
			}
		    bool bCanStopMontage = false;
		    if (InstanceData.CurrentTurnRange.bOverrideStopMontageOnGoalReached)
		    {
		        bCanStopMontage = InstanceData.CurrentTurnRange.bStopMontageOnGoalReached;
		    }
		    else
		    {
		        bCanStopMontage = InstanceData.CurrentTurnSettings.bStopMontageOnGoalReached;
		    }

			TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("GoalReached"), FColor::Cyan,
				FString::Printf(TEXT("goal reached: DeltaAngle=%.1f | bStopMontageOnGoalReached=%d (overridden=%d) -> %s"),
					DeltaAngle, bCanStopMontage, InstanceData.CurrentTurnRange.bOverrideStopMontageOnGoalReached,
					bCanStopMontage ? TEXT("StopAnimMontage") : TEXT("let montage finish")));

		    if (bCanStopMontage)
		    {
		        AICharacter->StopAnimMontage();
			    AIController->ClearFocus(EAIFocusPriority::Gameplay);
		        // CleanUp(*AIController, NodeMemory);
			    return InstanceData.bInfinite
			    	? EStateTreeRunStatus::Running
			    	: EStateTreeRunStatus::Succeeded;
		    }
		    else
		    {
			    AIController->ClearFocus(EAIFocusPriority::Gameplay);
		        // CleanUp(*AIController, NodeMemory);
			    return InstanceData.bInfinite
					? EStateTreeRunStatus::Running
					: EStateTreeRunStatus::Succeeded;
		    }
		}
	    else
	    {
	    	if (TurnToStatics::IsTurnWithAnimationRequired(AICharacter))
	    	{
	    		const bool bPlayed = TryPlayTurnAnimation(Context, AICharacter, DeltaAngle);

	    		// finish if no turn animation found and "bTurnOnlyWithAnims"
	    		if (!bPlayed && InstanceData.CurrentTurnSettings.bTurnOnlyWithAnims)
	    		{
	    			TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("Finish"), FColor::Yellow,
						TEXT("bTurnOnlyWithAnims=true & no range -> finish (Succeeded)"));
	    			AIController->ClearFocus(EAIFocusPriority::Gameplay);
	    			return InstanceData.bInfinite
						? EStateTreeRunStatus::Running
						: EStateTreeRunStatus::Succeeded;
	    		}
	    	}
	    	else
	    	{
	    		TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("Blocked"), FColor::Orange,
					FString::Printf(TEXT("turn-by-anim skipped: IsPlayingRootMotion=%d (DeltaAngle=%.1f)"),
						AICharacter ? (int32)AICharacter->IsPlayingRootMotion() : -1, DeltaAngle));
	    	}
	    }
	}
	else
	{
		TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("NoFocal"), FColor::Red,
			TEXT("invalid FocalPoint -> clear focus & finish (no target/focus set)"));
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		// CleanUp(*AIController, NodeMemory);
		return InstanceData.bInfinite
					? EStateTreeRunStatus::Running
					: EStateTreeRunStatus::Failed;
	}

	return FStateTreeTaskCommonBase::Tick(Context, DeltaTime);
}

void FUHLSTTask_TurnTo::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	if (InstanceData.bDesiredRotationDisabled && InstanceData.AIController)
	{
		if (ACharacter* AICharacter = InstanceData.AIController->GetCharacter())
		{
			if (UCharacterMovementComponent* Move = AICharacter->GetCharacterMovement())
			{
				Move->bUseControllerDesiredRotation = InstanceData.bCachedUseControllerDesiredRotation;
			}
		}
		InstanceData.bDesiredRotationDisabled = false;
	}
}

#if WITH_EDITOR
FText FUHLSTTask_TurnTo::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText Format = (Formatting == EStateTreeNodeFormatting::RichText)
		? LOCTEXT("DebugTextRich", "<b>Turn To</> \"{Text}\"")
		: LOCTEXT("DebugText", "Turn To \"{Text}\"");

	return FText::FormatNamed(Format, TEXT("Text"),
		FText::FromString(InstanceData->TargetActor
			? InstanceData->TargetActor.GetFullName()
			: InstanceData->TargetLocation.ToString()));
}
#endif

FTurnSettings FUHLSTTask_TurnTo::GetTurnSettings(FStateTreeExecutionContext& Context, AActor* Actor) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	FTurnSettings Result;
	if (InstanceData.SettingsSource == EUHLSettingsSource::Actor)
	{
		Result = IUHLAIActorSettings::Execute_GetTurnSettings(Actor);
	}
	if (InstanceData.SettingsSource == EUHLSettingsSource::DataAsset)
	{
		if (InstanceData.RotateToAnimationsDataAsset)
		{
			Result = InstanceData.RotateToAnimationsDataAsset->TurnSettings;
		}
		else
		{
			TurnToStatics::Report(InstanceData.bDebug, Actor, TEXT("NoDataAsset"), FColor::Red,
				TEXT("SettingsSource=DataAsset but RotateToAnimationsDataAsset is NULL -> empty TurnSettings"));
		}
	}
	if (InstanceData.SettingsSource == EUHLSettingsSource::Node)
	{
		Result = InstanceData.TurnSettings;
	}
	return Result;
}

bool FUHLSTTask_TurnTo::TryPlayTurnAnimation(
	FStateTreeExecutionContext& Context, ACharacter* AICharacter, float DeltaAngle) const
{
	if (!AICharacter) return false;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.CurrentTurnSettings.bUseBestSingleMontage)
	{
		// commit to exactly one montage for the whole turn; motion warping inside it finishes the residual angle
		if (InstanceData.bBestSingleMontagePlayed)
		{
			// still playing -> keep waiting (in-progress, no chaining);
			// finished but goal not yet reached (warp undershoot) -> return false so bTurnOnlyWithAnims finishes
			return AICharacter->GetCurrentMontage() == InstanceData.CurrentTurnRange.AnimMontage;
		}

		bool bBestRangeSet = false;
		InstanceData.CurrentTurnRange = TurnToStatics::GetBestSingleTurnRange(DeltaAngle, bBestRangeSet, InstanceData.CurrentTurnSettings);
		if (bBestRangeSet && InstanceData.CurrentTurnRange.AnimMontage)
		{
			AICharacter->PlayAnimMontage(InstanceData.CurrentTurnRange.AnimMontage);
			InstanceData.bBestSingleMontagePlayed = true;
			InstanceData.LastTurnSign = (DeltaAngle > 0.f) ? 1 : ((DeltaAngle < 0.f) ? -1 : 0);
			TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("PlayBestSingle"), FColor::Green,
				FString::Printf(TEXT("BEST-SINGLE DeltaAngle=%.1f -> Range '%s' (TurnAngle=%.0f) PLAY '%s' (warp finishes residual)"),
					DeltaAngle, *InstanceData.CurrentTurnRange.Name, InstanceData.CurrentTurnRange.TurnAngle,
					*InstanceData.CurrentTurnRange.AnimMontage->GetName()));
			return true;
		}

		// |DeltaAngle| smaller than the smallest montage's nominal -> nothing to play (warp/precision handles it)
		TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("NoBestSingle"), FColor::Red,
			FString::Printf(TEXT("BEST-SINGLE: no montage with TurnAngle <= |%.1f| -> finish"), DeltaAngle));
		return false;
	}

	bool bCurrentTurnRangeSet = false;
	InstanceData.CurrentTurnRange = TurnToStatics::GetTurnRange(DeltaAngle, bCurrentTurnRangeSet, InstanceData.CurrentTurnSettings);

	if (bCurrentTurnRangeSet && InstanceData.CurrentTurnRange.AnimMontage)
	{
		// don't restart the same montage every tick
		const bool bAlreadyPlaying = AICharacter->GetCurrentMontage() == InstanceData.CurrentTurnRange.AnimMontage;
		if (!bAlreadyPlaying)
		{
			// --- anti-oscillation (hysteresis) ---
			// A brand-new turn that reverses direction while we're already close to the goal
			// means the previous montage overshot the target. Playing the opposite montage
			// would start a ping-pong, so finish instead (return false -> caller succeeds when
			// bTurnOnlyWithAnims). Large reversals (target genuinely moved) are still played.
			const int32 NewTurnSign = (DeltaAngle > 0.f) ? 1 : ((DeltaAngle < 0.f) ? -1 : 0);
			if (InstanceData.OvershootTolerance > 0.0f
				&& InstanceData.LastTurnSign != 0
				&& NewTurnSign != 0
				&& NewTurnSign != InstanceData.LastTurnSign
				&& FMath::Abs(DeltaAngle) < InstanceData.OvershootTolerance)
			{
				TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("AntiOsc"), FColor::Yellow,
					FString::Printf(TEXT("direction reversed (%d -> %d) while |DeltaAngle|=%.1f < %.1f -> skip opposite montage (finish)"),
						InstanceData.LastTurnSign, NewTurnSign, FMath::Abs(DeltaAngle), InstanceData.OvershootTolerance));
				return false;
			}

			AICharacter->PlayAnimMontage(InstanceData.CurrentTurnRange.AnimMontage);
			InstanceData.LastTurnSign = NewTurnSign;
			TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("Play"), FColor::Green,
				FString::Printf(TEXT("DeltaAngle=%.1f -> Range '%s' [%.0f..%.0f] PLAY '%s'"),
					DeltaAngle, *InstanceData.CurrentTurnRange.Name,
					InstanceData.CurrentTurnRange.Range.GetLowerBoundValue(),
					InstanceData.CurrentTurnRange.Range.GetUpperBoundValue(),
					*InstanceData.CurrentTurnRange.AnimMontage->GetName()));
		}
		return true;
	}

	FString Reason = !bCurrentTurnRangeSet
		? FString::Printf(TEXT("no TurnRange covers DeltaAngle=%.1f (ranges=%d)"),
			DeltaAngle, TurnToStatics::CountTurnRanges(InstanceData.CurrentTurnSettings))
		: FString::Printf(TEXT("Range '%s' matched but AnimMontage is NULL"),
			*InstanceData.CurrentTurnRange.Name);
	TurnToStatics::Report(InstanceData.bDebug, AICharacter, TEXT("NoPlay"), FColor::Red,
		FString::Printf(TEXT("NO MONTAGE: %s"), *Reason));
	return false;
}


#undef LOCTEXT_NAMESPACE
