#include "IndigoCraftPreview.h"
#include "IndigoFlowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

AIndigoCraftPreview::AIndigoCraftPreview()
{
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    FoldPivot=CreateDefaultSubobject<USceneComponent>(TEXT("FoldPivot")); FoldPivot->SetupAttachment(RootComponent);
    PlatePivot=CreateDefaultSubobject<USceneComponent>(TEXT("PlatePivot")); PlatePivot->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto Part=[this](const TCHAR* Name,USceneComponent* Parent)
    {
        auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(Name); Mesh->SetupAttachment(Parent);
        Mesh->SetStaticMesh(Cube.Object); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false); return Mesh;
    };
    Cloth=Part(TEXT("Cloth"),RootComponent); MovingCloth=Part(TEXT("MovingCloth"),FoldPivot);
    FoldLine=Part(TEXT("FoldLine"),RootComponent); FoldLine->SetCastShadow(false);
    LowerPlate=Part(TEXT("LowerPlate"),RootComponent); UpperPlate=Part(TEXT("UpperPlate"),PlatePivot);
    LeftClamp=Part(TEXT("LeftClamp"),RootComponent); RightClamp=Part(TEXT("RightClamp"),RootComponent);
    DyeBasin=Part(TEXT("DyeBasin"),RootComponent); DyeWater=Part(TEXT("DyeWater"),RootComponent);
    DyeSwatch=Part(TEXT("DyeSwatch"),RootComponent);
    WashBasin=Part(TEXT("WashBasin"),RootComponent); WashWater=Part(TEXT("WashWater"),RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    DyeBasin->SetStaticMesh(Cylinder.Object); DyeWater->SetStaticMesh(Cylinder.Object);
    WashBasin->SetStaticMesh(Cylinder.Object); WashWater->SetStaticMesh(Cylinder.Object);
    ResetVisuals_Implementation();
}
void AIndigoCraftPreview::BeginPlay()
{
    Super::BeginPlay();
    ClothMaterial=Cloth->CreateDynamicMaterialInstance(0);
    if(ClothMaterial) MovingCloth->SetMaterial(0,ClothMaterial);
    SwatchMaterial=DyeSwatch->CreateDynamicMaterialInstance(0);
    ResetVisuals_Implementation();
}
void AIndigoCraftPreview::PoseCloth()
{
    Cloth->SetRelativeLocation(ClothCenter); Cloth->SetRelativeRotation(FRotator::ZeroRotator);
    Cloth->SetRelativeScale3D(ClothSize/100.f);
}
void AIndigoCraftPreview::ResetVisuals_Implementation()
{
    MotionToken=0; ActiveCue=NAME_None; Elapsed=0.f;
    CompletedFolds=0; bPlaced=bClosed=bLeftClamped=bRightClamped=false;
    bUnfolded=false; DisplayedResult=FIndigoResult();
    ClothCenter=FVector(0,-20,32); ClothSize=FVector(108,76,.35f); PoseCloth();
    MovingCloth->SetVisibility(false);
    FoldLine->SetVisibility(false);
    FoldPivot->SetRelativeTransform(FTransform::Identity);
    LowerPlate->SetRelativeLocation(FVector(0,52,2)); LowerPlate->SetRelativeScale3D(FVector(.46,.44,.04));
    PlatePivot->SetRelativeLocation(FVector(-23,52,9)); PlatePivot->SetRelativeRotation(FRotator(110,0,0));
    UpperPlate->SetRelativeLocation(FVector(23,0,0)); UpperPlate->SetRelativeScale3D(FVector(.46,.44,.035));
    LeftClamp->SetRelativeLocation(FVector(-21,52,24)); RightClamp->SetRelativeLocation(FVector(21,52,24));
    LeftClamp->SetRelativeScale3D(FVector(.06,.54,.05)); RightClamp->SetRelativeScale3D(FVector(.06,.54,.05));
    DyeBasin->SetRelativeLocation(FVector(96,52,-25)); DyeBasin->SetRelativeScale3D(FVector(.66,.66,.4));
    DyeWater->SetRelativeLocation(FVector(96,52,-4)); DyeWater->SetRelativeScale3D(FVector(.64,.64,.01));
    DyeSwatch->SetRelativeLocation(FVector(0,-28,16)); DyeSwatch->SetRelativeScale3D(FVector(.48,.32,.005)); DyeSwatch->SetVisibility(false);
    DyeBasin->SetVisibility(false); DyeWater->SetVisibility(false); DyeStartLocations.Empty();
    WashBasin->SetRelativeLocation(FVector(-96,52,-25)); WashBasin->SetRelativeScale3D(FVector(.66,.66,.4));
    WashWater->SetRelativeLocation(FVector(-96,52,-4)); WashWater->SetRelativeScale3D(FVector(.64,.64,.01));
    WashBasin->SetVisibility(false); WashWater->SetVisibility(false);
    if(ClothMaterial) ClothMaterial->SetScalarParameterValue(TEXT("ShowPattern"),0.f);
    SetDyeProgress_Implementation(0,0.f);
}
void AIndigoCraftPreview::ShowFinalQuilt_Implementation(const FIndigoResult& Result)
{
    DisplayedResult=Result; bUnfolded=true;
    ClothCenter=FVector(0,-20,32); ClothSize=FVector(108,76,.35f); PoseCloth();
    SetDyeProgress_Implementation(Result.Depth,0.f);
    if(ClothMaterial)
    {
        ClothMaterial->SetScalarParameterValue(TEXT("PatternLeft"),static_cast<uint8>(Result.Left));
        ClothMaterial->SetScalarParameterValue(TEXT("PatternRight"),static_cast<uint8>(Result.Right));
        ClothMaterial->SetScalarParameterValue(TEXT("ShowPattern"),1.f);
    }
}
TArray<USceneComponent*> AIndigoCraftPreview::DyeParts() const { return {Cloth,LowerPlate,PlatePivot,LeftClamp,RightClamp}; }
void AIndigoCraftPreview::SetDyeProgress_Implementation(int32 CompletedDepth,float Progress)
{
    PreviewDepth=FMath::Clamp(CompletedDepth,0,4);
    DyeProgress=FMath::Clamp(Progress,0.f,1.f);
    const FLinearColor Shades[]={FLinearColor(.8f,.85f,.76f),FLinearColor(.2f,.42f,.68f),FLinearColor(.065f,.2f,.44f),FLinearColor(.02f,.075f,.24f),FLinearColor(.006f,.018f,.09f)};
    const FLinearColor Color=FMath::Lerp(Shades[PreviewDepth],Shades[FMath::Min(4,PreviewDepth+1)],DyeProgress);
    if(ClothMaterial) ClothMaterial->SetVectorParameterValue(TEXT("DyeColor"),Color);
    if(SwatchMaterial) SwatchMaterial->SetVectorParameterValue(TEXT("DyeColor"),Color);
}
void AIndigoCraftPreview::PlayIndigoAnimation_Implementation(FName Cue,int32 Token)
{
    if (!IsValid(Flow) || Token!=Flow->GetAnimationToken() || !Flow->bAnimationBusy) return;
    if (Cue!=TEXT("Fold") && Cue!=TEXT("PlaceCloth") && Cue!=TEXT("ClosePlate") && Cue!=TEXT("ClampLeft") && Cue!=TEXT("ClampRight") && Cue!=TEXT("Dye") && Cue!=TEXT("Wash") && Cue!=TEXT("Drain") && Cue!=TEXT("UnclampLeft") && Cue!=TEXT("UnclampRight") && Cue!=TEXT("Unfold") && Cue!=TEXT("Deliver")) return;
    MotionToken=Token; ActiveCue=Cue; Elapsed=0.f;
    if (Cue==TEXT("Fold"))
    {
        bFoldX=CompletedFolds%2==0;
        const FVector HalfSize=ClothSize* (bFoldX?FVector(.5,1,1):FVector(1,.5,1));
        const FVector Offset=bFoldX?FVector(ClothSize.X*.25,0,0):FVector(0,ClothSize.Y*.25,0);
        FoldPivot->SetRelativeLocation(ClothCenter); FoldPivot->SetRelativeRotation(FRotator::ZeroRotator);
        Cloth->SetRelativeLocation(ClothCenter-Offset); Cloth->SetRelativeScale3D(HalfSize/100.f);
        MovingCloth->SetRelativeLocation(Offset); MovingCloth->SetRelativeRotation(FRotator::ZeroRotator);
        MovingCloth->SetRelativeScale3D(HalfSize/100.f); MovingCloth->SetVisibility(true);
    }
    if (Cue==TEXT("PlaceCloth")) MoveStart=ClothCenter;
    if (Cue==TEXT("Dye") || Cue==TEXT("Wash") || Cue==TEXT("Drain"))
    {
        DyeStartLocations.Empty();
        for(USceneComponent* Part:DyeParts()) DyeStartLocations.Add(Part->GetRelativeLocation());
        SetDyeProgress_Implementation(Flow->Result.Depth,0.f);
    }
    if(Cue==TEXT("Unfold")) { MoveStart=ClothCenter; UnfoldStartSize=ClothSize; }
    ApplyMotion(0.f);
}
void AIndigoCraftPreview::ApplyMotion(float T)
{
    const float Smooth=T*T*(3.f-2.f*T);
    if (ActiveCue==TEXT("Fold"))
    {
        FoldPivot->SetRelativeRotation(FQuat(bFoldX?FVector::YAxisVector:FVector::XAxisVector,(bFoldX?-1.f:1.f)*PI*Smooth));
        MovingCloth->SetRelativeLocation((bFoldX?FVector(ClothSize.X*.25,0,0):FVector(0,ClothSize.Y*.25,0))+FVector(0,0,ClothSize.Z*Smooth));
    }
    else if (ActiveCue==TEXT("PlaceCloth"))
        Cloth->SetRelativeLocation(FMath::Lerp(MoveStart,FVector(0,52,5.5),Smooth)+FVector(0,0,12.f*FMath::Sin(PI*T)));
    else if (ActiveCue==TEXT("ClosePlate")) PlatePivot->SetRelativeRotation(FRotator(FMath::Lerp(110.f,0.f,Smooth),0,0));
    else if (ActiveCue==TEXT("ClampLeft")) LeftClamp->SetRelativeLocation(FVector(-21,52,FMath::Lerp(24.f,11.f,Smooth)));
    else if (ActiveCue==TEXT("ClampRight")) RightClamp->SetRelativeLocation(FVector(21,52,FMath::Lerp(24.f,11.f,Smooth)));
    else if (ActiveCue==TEXT("UnclampLeft")) LeftClamp->SetRelativeLocation(FMath::Lerp(FVector(-21,52,11),FVector(-45,52,24),Smooth));
    else if (ActiveCue==TEXT("UnclampRight")) RightClamp->SetRelativeLocation(FMath::Lerp(FVector(21,52,11),FVector(45,52,24),Smooth));
    else if (ActiveCue==TEXT("Wash") || ActiveCue==TEXT("Drain"))
    {
        FVector Offset;
        if(ActiveCue==TEXT("Wash"))
            Offset=FVector(-96*FMath::Sin(PI*T),0,-28*FMath::Sin(PI*T)+3*FMath::Sin(6*PI*T));
        else Offset=FVector(0,0,22*FMath::Sin(PI*T));
        const auto Parts=DyeParts();
        if(DyeStartLocations.Num()==Parts.Num()) for(int32 I=0;I<Parts.Num();++I) Parts[I]->SetRelativeLocation(DyeStartLocations[I]+Offset);
    }
    else if (ActiveCue==TEXT("Unfold"))
    {
        PlatePivot->SetRelativeRotation(FRotator(110*FMath::Clamp(T*4.f,0.f,1.f),0,0));
        const float U=FMath::Clamp((T-.25f)/.75f,0.f,1.f);
        ClothCenter=FMath::Lerp(MoveStart,FVector(0,-20,32),U);
        // Reverse the three blockout folds one at a time, then restore a flat sheet.
        const float X=FMath::Clamp(U*3.f,0.f,1.f)+FMath::Clamp(U*3.f-2.f,0.f,1.f);
        const float Y=FMath::Clamp(U*3.f-1.f,0.f,1.f);
        ClothSize=UnfoldStartSize*FVector(FMath::Pow(2.f,X),FMath::Pow(2.f,Y),FMath::Pow(.5f,X+Y)); PoseCloth();
    }
    else if (ActiveCue==TEXT("Deliver")) Cloth->SetRelativeLocation(ClothCenter+FVector(0,0,6*FMath::Sin(PI*T)));
    else if (ActiveCue==TEXT("Dye"))
    {
        // Lift, immerse, hold, lift to oxidize, then return. One complete cycle is one depth.
        const auto Ease=[](float V){V=FMath::Clamp(V,0.f,1.f);return V*V*(3.f-2.f*V);};
        FVector Offset;
        if(T<.2f) Offset=FMath::Lerp(FVector::ZeroVector,FVector(96,0,30),Ease(T/.2f));
        else if(T<.45f) Offset=FMath::Lerp(FVector(96,0,30),FVector(96,0,-28),Ease((T-.2f)/.25f));
        else if(T<.6f) Offset=FVector(96,0,-28);
        else if(T<.8f) Offset=FMath::Lerp(FVector(96,0,-28),FVector(96,0,30),Ease((T-.6f)/.2f));
        else Offset=FMath::Lerp(FVector(96,0,30),FVector::ZeroVector,Ease((T-.8f)/.2f));
        const auto Parts=DyeParts();
        if(DyeStartLocations.Num()==Parts.Num()) for(int32 I=0;I<Parts.Num();++I) Parts[I]->SetRelativeLocation(DyeStartLocations[I]+Offset);
        Flow->ReportDyeProgress(MotionToken,FMath::Clamp((T-.45f)/.55f,0.f,1.f));
    }
}
void AIndigoCraftPreview::FinishMotion()
{
    if (ActiveCue==TEXT("Fold"))
    {
        ClothCenter-=bFoldX?FVector(ClothSize.X*.25,0,0):FVector(0,ClothSize.Y*.25,0);
        ClothCenter.Z+=ClothSize.Z*.5;
        ClothSize*=bFoldX?FVector(.5,1,2):FVector(1,.5,2);
        ++CompletedFolds; PoseCloth(); MovingCloth->SetVisibility(false);
    }
    else if (ActiveCue==TEXT("PlaceCloth")) { bPlaced=true; ClothCenter=FVector(0,52,5.5); PoseCloth(); }
    else if (ActiveCue==TEXT("ClosePlate")) bClosed=true;
    else if (ActiveCue==TEXT("ClampLeft")) bLeftClamped=true;
    else if (ActiveCue==TEXT("ClampRight")) bRightClamped=true;
    else if (ActiveCue==TEXT("UnclampLeft")) bLeftClamped=false;
    else if (ActiveCue==TEXT("UnclampRight")) bRightClamped=false;
    else if (ActiveCue==TEXT("Unfold")) { bClosed=false; bUnfolded=true; }
    const int32 CompletedToken=MotionToken;
    MotionToken=0; ActiveCue=NAME_None;
    Flow->CompleteAnimation(CompletedToken);
}
void AIndigoCraftPreview::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsValid(Flow)) return;
    const bool DyeStage=Flow->Step==EIndigoStep::Dye || Flow->Step==EIndigoStep::Wash;
    const bool Final=Flow->Step==EIndigoStep::Reveal || Flow->Step==EIndigoStep::End;
    const bool Craft=Flow->Step==EIndigoStep::Fold || Flow->Step==EIndigoStep::Clamp || DyeStage || Final;
    SetActorHiddenInGame(!Craft);
    for (AActor* Sample:SelectionSamples) if (IsValid(Sample)) Sample->SetActorHiddenInGame(Craft);
    DyeBasin->SetVisibility(DyeStage); DyeWater->SetVisibility(DyeStage);
    DyeSwatch->SetVisibility(DyeStage);
    WashBasin->SetVisibility(Flow->Step==EIndigoStep::Wash); WashWater->SetVisibility(Flow->Step==EIndigoStep::Wash);
    LowerPlate->SetVisibility(!Final); UpperPlate->SetVisibility(!Final); LeftClamp->SetVisibility(!Final); RightClamp->SetVisibility(!Final);
    FoldLine->SetVisibility(Flow->Step==EIndigoStep::Fold && !Flow->bAnimationBusy);
    FoldLine->SetRelativeLocation(ClothCenter+FVector(0,0,ClothSize.Z*.5f+.2f));
    FoldLine->SetRelativeScale3D(CompletedFolds%2==0?FVector(.008f,ClothSize.Y/100.f,.003f):FVector(ClothSize.X/100.f,.008f,.003f));
    if (!MotionToken) return;
    // An external reset, completion, or step change must never let an old motion finish a newer one.
    if (!Flow->bAnimationBusy || Flow->GetAnimationToken()!=MotionToken) { MotionToken=0; ActiveCue=NAME_None; MovingCloth->SetVisibility(false); return; }
    Elapsed+=FMath::Max(0.f,DeltaSeconds);
    const float T=FMath::Clamp(Elapsed/FMath::Max(.1f,ActiveCue==TEXT("Dye")?DyeSeconds:MotionSeconds),0.f,1.f);
    ApplyMotion(T);
    if (T>=1.f) FinishMotion();
}
