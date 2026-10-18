#pragma once
#include "CoreMinimal.h"
#include "IndigoInteractable.h"
#include "IndigoPatternSource.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class INDIGOINTERACTION_API AIndigoPatternSource : public AIndigoInteractable
{
    GENERATED_BODY()
public:
    AIndigoPatternSource();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> BoardMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UTextRenderComponent> PatternLabel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="0.05")) float VisualScale = 0.45f;
    virtual void OnConstruction(const FTransform& Transform) override;
};
