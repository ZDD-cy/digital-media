#include "IndigoCraftHandle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
AIndigoCraftHandle::AIndigoCraftHandle()
{
    PrimaryActorTick.bCanEverTick=true;
    Marker=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));Marker->SetupAttachment(HitBox);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    Marker->SetStaticMesh(Cube.Object);Marker->SetMaterial(0,Mat.Object);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);Marker->SetCastShadow(false);
    HitBox->SetBoxExtent(FVector(9,9,3));DragGroup=TEXT("Craft");
}
void AIndigoCraftHandle::Configure(EIndigoStep NewStep,EIndigoAction NewAction,int32 Index,bool bTarget,const FVector& Position,bool bEnabled)
{
    const bool Changed=ActiveStep!=NewStep || Action!=NewAction || ActionIndex!=Index;
    if(Changed || !bEnabledBefore || bTarget) SetActorLocation(Position);
    ActiveStep=NewStep;Action=NewAction;ActionIndex=Index;
    Kind=bTarget?EIndigoInteractionKind::DropTarget:EIndigoInteractionKind::DragSource;
    Marker->SetRelativeScale3D(bTarget?FVector(.22,.22,.012):FVector(.13,.13,.025));
    SetActorHiddenInGame(!bEnabled);SetActorEnableCollision(bEnabled);bEnabledBefore=bEnabled;
    if(!Material) Material=Marker->CreateDynamicMaterialInstance(0);
    UpdateColor();
}
void AIndigoCraftHandle::UpdateColor()
{
    if(Material) Material->SetVectorParameterValue(TEXT("Color"),bHover || bHighlight || FlashRemaining>0.f?FLinearColor(1.f,.8f,.08f):
        Kind==EIndigoInteractionKind::DropTarget?FLinearColor(.1f,1.f,.3f):FLinearColor(.05f,.45f,1.f));
}
void AIndigoCraftHandle::SetHover_Implementation(bool Value){bHover=Value;UpdateColor();}
void AIndigoCraftHandle::SetDropHighlight_Implementation(bool Value){bHighlight=Value;UpdateColor();}
void AIndigoCraftHandle::FlashTarget(){FlashRemaining=1.f;UpdateColor();}
void AIndigoCraftHandle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(FlashRemaining>0.f){FlashRemaining=FMath::Max(0.f,FlashRemaining-DeltaSeconds);UpdateColor();}
}
