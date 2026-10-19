#include "IndigoInteractable.h"
#include "IndigoFlowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
void AIndigoInteractable::SetHover_Implementation(bool Value) { HitBox->SetHiddenInGame(!Value); }
void AIndigoInteractable::SetDropHighlight_Implementation(bool Value) { HitBox->SetHiddenInGame(!Value); }
void AIndigoInteractable::InvalidDrop_Implementation() {}

AIndigoInteractable::AIndigoInteractable()
{
    PrimaryActorTick.bCanEverTick = true;
    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
    SetRootComponent(HitBox); HitBox->SetBoxExtent(FVector(35.f));
    HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    HitBox->SetGenerateOverlapEvents(false);
}

void AIndigoInteractable::AlignSlotToVisual()
{
    if (Kind != EIndigoInteractionKind::DropTarget || !IsValid(SlotVisual) || !SlotVisual->GetStaticMesh()) return;
    FVector Low, High; SlotVisual->GetLocalBounds(Low, High);
    const FTransform VisualTransform = SlotVisual->GetComponentTransform();
    SetActorTransform(FTransform(VisualTransform.GetRotation(), VisualTransform.TransformPosition((Low+High)*.5f), VisualTransform.GetScale3D()));
    const FVector Half = (High-Low)*.5f;
    HitBox->SetBoxExtent(FVector(Half.X+4.f, Half.Y+4.f, FMath::Max(8.f,Half.Z+4.f)));
}
void AIndigoInteractable::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); AlignSlotToVisual();
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
