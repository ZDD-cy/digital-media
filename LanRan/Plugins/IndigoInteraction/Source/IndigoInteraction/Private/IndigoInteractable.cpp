#include "IndigoInteractable.h"
#include "IndigoFlowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "IndigoPatternSource.h"
#include "IndigoDirector.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
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
    const FVector Scale=VisualTransform.GetScale3D().GetAbs().ComponentMax(FVector(.001f));
    HitBox->SetBoxExtent(Half+SlotPadding.ComponentMax(FVector::ZeroVector)/Scale);
}
void AIndigoInteractable::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); AlignSlotToVisual();
    if(Kind==EIndigoInteractionKind::DropTarget && ActiveStep==EIndigoStep::Select)
        for(TActorIterator<AIndigoDirector> It(GetWorld());It;++It) { ApplySelectedPattern(It->Flow);break; }
}

void AIndigoInteractable::ApplySelectedPattern(UIndigoFlowComponent* Flow)
{
    if(!Flow || Kind!=EIndigoInteractionKind::DropTarget || !IsValid(SlotVisual)) return;
    const EIndigoPattern Selected=bLeftSlot?Flow->Result.Left:Flow->Result.Right;
    if(!bPatternVisualInitialized)
    {
        EmptySlotMesh=SlotVisual->GetStaticMesh();
        for(UMaterialInterface* Material:SlotVisual->GetMaterials()) EmptySlotMaterials.Add(Material);
        bPatternVisualInitialized=true;
    }
    else if(Selected==VisiblePattern) return;
    VisiblePattern=Selected;
    if(Selected==EIndigoPattern::None)
    {
        SlotVisual->SetStaticMesh(EmptySlotMesh);
        for(int32 I=0;I<EmptySlotMaterials.Num();++I) SlotVisual->SetMaterial(I,EmptySlotMaterials[I]);
        return;
    }
    for(TActorIterator<AIndigoPatternSource> It(GetWorld());It;++It) if(It->Pattern==Selected && It->BoardMesh->GetStaticMesh())
    {
        // Preserve the authored transform, including the -90 degree yaw.
        SlotVisual->SetStaticMesh(It->BoardMesh->GetStaticMesh());
        for(int32 I=0;I<It->BoardMesh->GetNumMaterials();++I)
        {
            SlotVisual->SetMaterial(I,It->BoardMesh->GetMaterial(I));
            if(auto* Material=SlotVisual->CreateDynamicMaterialInstance(I))
                for(const FName Name:{FName(TEXT("SelectIndex")),FName(TEXT("SelectIndex1")),FName(TEXT("SelectIndex2")),FName(TEXT("SelectIndex3"))})
                    Material->SetScalarParameterValue(Name,static_cast<uint8>(Selected));
        }
        break;
    }
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
