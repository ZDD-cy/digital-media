#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IndigoTypes.h"
#include "IndigoVisualInterface.generated.h"

UINTERFACE(BlueprintType)
class INDIGOINTERACTION_API UIndigoVisualInterface : public UInterface { GENERATED_BODY() };

class INDIGOINTERACTION_API IIndigoVisualInterface
{
    GENERATED_BODY()
public:
    // Reset must stop old sequences/timelines and restore all scene objects.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Indigo|Visual") void ResetVisuals();
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Indigo|Visual") void SetPattern(EIndigoPattern Left, EIndigoPattern Right);
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Indigo|Visual") void SetDyeProgress(int32 CompletedDepth, float Progress);
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Indigo|Visual") void ShowFinalQuilt(const FIndigoResult& Result);
    // Complete via Flow.CompleteAnimation(Token). Never cache only the latest token.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Indigo|Visual") void PlayIndigoAnimation(FName Cue, int32 Token);
};
