#pragma once
#include "CoreMinimal.h"
#include "IndigoTypes.generated.h"

UENUM(BlueprintType)
enum class EIndigoPattern : uint8 { None, A, B, C };
UENUM(BlueprintType)
enum class EIndigoStep : uint8 { Receive, Select, Fold, Clamp, Dye, Wash, Reveal, End };
UENUM(BlueprintType)
enum class EIndigoAction : uint8
{
    InspectCloth, InspectOrder, Swap, ClearLeft, ClearRight, Confirm,
    Fold, PlaceCloth, ClosePlate, ClampLeft, ClampRight,
    Dye, FinishDye, Wash, Drain, UnclampLeft, UnclampRight, Unfold,
    Deliver, Review, Back
};
UENUM(BlueprintType)
enum class EIndigoCamera : uint8 { Table, Closeup, Quilt };

USTRUCT(BlueprintType)
struct INDIGOINTERACTION_API FIndigoResult
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") EIndigoPattern Left = EIndigoPattern::None;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") EIndigoPattern Right = EIndigoPattern::None;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") int32 Depth = 0;
};

USTRUCT(BlueprintType)
struct INDIGOINTERACTION_API FIndigoStepConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") FText Subtitle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") EIndigoCamera Camera = EIndigoCamera::Table;
};
