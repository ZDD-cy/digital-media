#include "IndigoInteractable.h"
#include "IndigoFlowComponent.h"
#include "Components/BoxComponent.h"
void AIndigoInteractable::SetHover_Implementation(bool Value) { HitBox->SetHiddenInGame(!Value); }
void AIndigoInteractable::SetDropHighlight_Implementation(bool Value) { HitBox->SetHiddenInGame(!Value); }
void AIndigoInteractable::InvalidDrop_Implementation() {}

AIndigoInteractable::AIndigoInteractable()
{
    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
    SetRootComponent(HitBox); HitBox->SetBoxExtent(FVector(35.f));
    HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    HitBox->SetGenerateOverlapEvents(false);
}

bool AIndigoInteractable::IsAvailable(UIndigoFlowComponent* Flow) const
{
    if (!IsValid(Flow) || Flow->IsInputLocked() || Flow->Step != ActiveStep) return false;
    if (ActiveStep == EIndigoStep::Select && Kind != EIndigoInteractionKind::Click) return true;
    return Flow->CanPerform(Action, ActionIndex);
}

bool AIndigoInteractable::Accepts(AIndigoInteractable* Source, UIndigoFlowComponent* Flow) const
{
    return IsValid(Source) && Kind == EIndigoInteractionKind::DropTarget &&
        Source->Kind == EIndigoInteractionKind::DragSource && Source->DragGroup == DragGroup &&
        Source->IsAvailable(Flow) && IsAvailable(Flow) &&
        (ActiveStep == EIndigoStep::Select || (Action == Source->Action && ActionIndex == Source->ActionIndex));
}
