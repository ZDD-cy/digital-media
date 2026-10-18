#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "Components/SceneComponent.h"
AIndigoDirector::AIndigoDirector()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Flow = CreateDefaultSubobject<UIndigoFlowComponent>(TEXT("Flow"));
}
