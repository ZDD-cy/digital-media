#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IndigoTypes.h"
#include "IndigoDirector.generated.h"

class ACameraActor;
class UIndigoFlowComponent;
UCLASS(Blueprintable)
class INDIGOINTERACTION_API AIndigoDirector : public AActor
{
    GENERATED_BODY()
public:
    AIndigoDirector();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UIndigoFlowComponent> Flow;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TMap<EIndigoCamera, TObjectPtr<ACameraActor>> Cameras;
};
