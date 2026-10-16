#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IndigoTypes.h"
#include "IndigoFlowComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIndigoStepEvent, EIndigoStep, Step);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIndigoSubtitleEvent, FText, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIndigoResultEvent, FIndigoResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIndigoAnimationEvent, FName, Cue, int32, Token);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIndigoInvalidationEvent);

UCLASS(ClassGroup=(Indigo), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class INDIGOINTERACTION_API UIndigoFlowComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UIndigoFlowComponent();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") EIndigoStep Step = EIndigoStep::Receive;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") FIndigoResult Result;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") int32 FoldIndex = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bAnimationBusy = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bCameraBusy = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Indigo", meta=(ClampMin="1")) int32 FoldCount = 3;
    // Placeholder only. Disable when real animation callbacks are connected.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Animation") bool bUsePlaceholderTimer = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Animation", meta=(ClampMin="0.05")) float PlaceholderSeconds = 0.6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Visual") TArray<TObjectPtr<AActor>> VisualTargets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Config") TMap<EIndigoStep, FIndigoStepConfig> StepConfig;
    UPROPERTY(BlueprintAssignable, Category="Indigo") FIndigoStepEvent OnStepChanged;
    UPROPERTY(BlueprintAssignable, Category="Indigo") FIndigoSubtitleEvent OnSubtitle;
    UPROPERTY(BlueprintAssignable, Category="Indigo") FIndigoResultEvent OnResultChanged;
    UPROPERTY(BlueprintAssignable, Category="Indigo") FIndigoAnimationEvent OnAnimationRequested;
    UPROPERTY(BlueprintAssignable, Category="Indigo") FIndigoInvalidationEvent OnInteractionInvalidated;

    UFUNCTION(BlueprintPure, Category="Indigo") bool IsInputLocked() const { return bAnimationBusy || bCameraBusy; }
    UFUNCTION(BlueprintPure, Category="Indigo") bool CanPerform(EIndigoAction Action, int32 Index = 0) const;
    UFUNCTION(BlueprintCallable, Category="Indigo") bool Perform(EIndigoAction Action, int32 Index = 0);
    UFUNCTION(BlueprintCallable, Category="Indigo") bool SetPattern(bool bLeft, EIndigoPattern Pattern);
    UFUNCTION(BlueprintCallable, Category="Indigo") void Restart();
    UFUNCTION(BlueprintCallable, Category="Indigo|Animation") bool CompleteAnimation(int32 Token);
    UFUNCTION(BlueprintCallable, Category="Indigo|Animation") bool ReportDyeProgress(int32 Token, float Progress);
    UFUNCTION(BlueprintCallable, Category="Indigo|Camera") void SetCameraBusy(bool bBusy);
    UFUNCTION(BlueprintCallable, Category="Indigo|Visual") void RefreshVisuals();
    UFUNCTION(BlueprintPure, Category="Indigo") int32 GetAnimationToken() const { return ActiveToken; }
    UFUNCTION(BlueprintPure, Category="Indigo") bool IsReview() const { return bReview; }
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    bool bClothSeen = false, bOrderSeen = false, bPlaced = false, bPlateClosed = false;
    bool bLeftClamp = false, bRightClamp = false, bReview = false;
    int32 WashPhase = 0;
    int32 Serial = 0, ActiveToken = 0;
    EIndigoAction Pending = EIndigoAction::Confirm;
    FTimerHandle AnimationTimer;
    void Enter(EIndigoStep Next);
    void StartAnimation(EIndigoAction Action, FName Cue);
    void PublishResult();
};
