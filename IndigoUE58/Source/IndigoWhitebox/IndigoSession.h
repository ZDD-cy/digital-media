#pragma once
#include "CoreMinimal.h"
#include "IndigoSession.generated.h"

UENUM(BlueprintType)
enum class EIndigoStep : uint8 { Receive, Pattern, Test, Heat, Retest, Fold, Clamp, Dye, Wash, Reveal, Letter, Pack, FilmOne, FilmTwo, Sky, End, Review };

USTRUCT(BlueprintType)
struct INDIGOWHITEBOX_API FIndigoSession {
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EIndigoStep Step = EIndigoStep::Receive;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 LeftPattern = -1;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 RightPattern = -1;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 DyeDepth = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Fuel = 5;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool Ready = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool SawOld = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool SawLetter = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 TestPhase = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 FoldCount = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ClampPhase = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool ClipLeft = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool ClipRight = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 WashPhase = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool OpenLeft = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool OpenRight = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Remaining = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Duration = 0;
    FString Pending;
    bool Busy() const { return Remaining > 0; }
    float Progress() const { return Duration > 0 ? FMath::Clamp(1.f-Remaining/Duration,0.f,1.f) : 0; }
    void Reset() { *this = FIndigoSession(); }
    void Enter(EIndigoStep Next) { Step=Next; Remaining=Duration=0; Pending.Empty(); }
    void Animate(const FString& Event, float Seconds) { Pending=Event; Remaining=Duration=Seconds; }
    bool Act(const FString& Action, int32 Value=-1);
    bool Advance(float Delta);
    FString Title() const;
    FString Hint() const;
};
