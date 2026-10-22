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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") bool bSkipReceipt = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|P1") bool bEnableTemperatureRules = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|P1") bool bEnableDeliveryStory = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Story", meta=(ClampMin="0")) float DeliveryReadingSeconds = 8.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|P1") EIndigoTemperature Temperature = EIndigoTemperature::Cold;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|P1") int32 Fuel = 5;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|P1") int32 TrialIndex = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|P1") bool bColdTrialSeen = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|P1") bool bWarmTrialSeen = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Story") FText OrderFront = FText::FromString(TEXT("旧样留一处，其余花纹请你挑。做好后寄回家里。"));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Story") FText OrderBack = FText::FromString(TEXT("我成婚时，母亲送过我一床蓝夹缬被面。如今女儿也要成婚了，想给她做一床新的。旧样留在我这里，请把新的寄给她。"));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Story") FText ShippingText = FText::FromString(TEXT("希望这床被面，能陪她过好往后的日子。"));
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
    UFUNCTION(BlueprintPure, Category="Indigo|Story") bool HasSeenOrder() const { return bOrderSeen; }
    UFUNCTION(BlueprintPure, Category="Indigo|Story") float GetDeliveryReadingRemaining() const;
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    bool bClothSeen = false, bOrderSeen = false, bPlaced = false, bPlateClosed = false;
    bool bLeftClamp = false, bRightClamp = false, bReview = false;
    int32 WashPhase = 0;
    int32 Serial = 0, ActiveToken = 0;
    EIndigoAction Pending = EIndigoAction::Confirm;
    FTimerHandle AnimationTimer;
    float DeliveryOpenedAt = 0.f;
    void Enter(EIndigoStep Next);
    void StartAnimation(EIndigoAction Action, FName Cue);
    void PublishResult();
};
