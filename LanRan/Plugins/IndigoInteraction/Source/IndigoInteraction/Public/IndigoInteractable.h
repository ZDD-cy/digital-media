#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IndigoTypes.h"
#include "IndigoInteractable.generated.h"

class UBoxComponent;
class UIndigoFlowComponent;
UENUM(BlueprintType)
enum class EIndigoInteractionKind : uint8 { Click, DragSource, DropTarget };

UCLASS(Blueprintable)
class INDIGOINTERACTION_API AIndigoInteractable : public AActor
{
    GENERATED_BODY()
public:
    AIndigoInteractable();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UBoxComponent> HitBox;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") EIndigoInteractionKind Kind = EIndigoInteractionKind::Click;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") EIndigoStep ActiveStep = EIndigoStep::Select;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") EIndigoAction Action = EIndigoAction::Confirm;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") int32 ActionIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") FName DragGroup = TEXT("Pattern");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") EIndigoPattern Pattern = EIndigoPattern::A;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") bool bLeftSlot = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo") bool bReturnToSource = true;
    UFUNCTION(BlueprintPure, Category="Indigo") bool IsAvailable(UIndigoFlowComponent* Flow) const;
    UFUNCTION(BlueprintPure, Category="Indigo") bool Accepts(AIndigoInteractable* Source, UIndigoFlowComponent* Flow) const;
    UFUNCTION(BlueprintNativeEvent, Category="Indigo") void SetHover(bool bHovered);
    UFUNCTION(BlueprintNativeEvent, Category="Indigo") void SetDropHighlight(bool bHighlighted);
    UFUNCTION(BlueprintNativeEvent, Category="Indigo") void InvalidDrop();
    virtual void SetHover_Implementation(bool bHovered);
    virtual void SetDropHighlight_Implementation(bool bHighlighted);
    virtual void InvalidDrop_Implementation();
};
