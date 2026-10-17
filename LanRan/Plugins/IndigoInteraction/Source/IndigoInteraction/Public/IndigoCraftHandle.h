#pragma once
#include "CoreMinimal.h"
#include "IndigoInteractable.h"
#include "IndigoCraftHandle.generated.h"
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
// Reusable visual handle. All hit testing and drop acceptance remain in the bridge.
UCLASS()
class INDIGOINTERACTION_API AIndigoCraftHandle : public AIndigoInteractable
{
    GENERATED_BODY()
public:
    AIndigoCraftHandle();
    void Configure(EIndigoStep Step,EIndigoAction NewAction,int32 Index,bool bTarget,const FVector& Position,bool bEnabled);
    void FlashTarget();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetHover_Implementation(bool bHovered) override;
    virtual void SetDropHighlight_Implementation(bool bHighlighted) override;
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Marker;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
    bool bEnabledBefore=false;
    bool bHover=false;
    bool bHighlight=false;
    float FlashRemaining=0.f;
    void UpdateColor();
};
