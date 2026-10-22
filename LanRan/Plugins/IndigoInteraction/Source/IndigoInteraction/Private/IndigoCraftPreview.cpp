#include "IndigoCraftPreview.h"
#include "IndigoFlowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "IndigoCraftHandle.h"
#include "Engine/World.h"

FVector AIndigoCraftPreview::PartCenter(USceneComponent* Part) const
{
    if(const FVector* Center=PartCenters.Find(Part)) return *Center;
    return Part->GetRelativeLocation();
}
void AIndigoCraftPreview::SetPartLocation(USceneComponent* Part,const FVector& Center)
{
    PartCenters.Add(Part,Center);
    FVector Offset=FVector::ZeroVector;
    if(auto* Mesh=Cast<UStaticMeshComponent>(Part)) if(Mesh->GetStaticMesh())
        Offset=Part->GetRelativeRotation().RotateVector(Mesh->GetStaticMesh()->GetBounds().Origin*Part->GetRelativeScale3D());
    Part->SetRelativeLocation(Center-Offset);
}
void AIndigoCraftPreview::SetPartScale(UStaticMeshComponent* Part,const FVector& CanonicalScale)
{
    const FVector Center=PartCenter(Part);
    const FVector Size=Part->GetStaticMesh()?Part->GetStaticMesh()->GetBounds().BoxExtent*2.f:FVector(100.f);
    // Convert desired table-space dimensions to the authored mesh's local axes.
    const FVector LocalSize=Part->GetRelativeRotation().UnrotateVector(CanonicalScale*100.f).GetAbs();
    Part->SetRelativeScale3D(LocalSize/Size.ComponentMax(FVector(.001f)));
    SetPartLocation(Part,Center);
}
float AIndigoCraftPreview::ColorDepthForStage(int32 Depth)
{
    const float Values[]={10.f,4.f,1.f,.1f,.03f};
    return Values[FMath::Clamp(Depth,0,4)];
}
FVector AIndigoCraftPreview::BasinMotion(float T,bool bWash) const
{
    const float Side=bWash?-BasinDistance:BasinDistance;
    const FVector Home=FVector::ZeroVector,Up(0,0,80),Across(Side,0,80),Inside(Side,0,28);
    const auto Ease=[](float V){V=FMath::Clamp(V,0.f,1.f);return V*V*(3.f-2.f*V);};
    if(T<.12f) return FMath::Lerp(Home,Up,Ease(T/.12f));
    if(T<.25f) return FMath::Lerp(Up,Across,Ease((T-.12f)/.13f));
    if(T<.4f) return FMath::Lerp(Across,Inside,Ease((T-.25f)/.15f));
    if(T<.55f) return Inside+FVector(0,0,bWash?2.f*FMath::Sin((T-.4f)/.15f*2.f*PI):0.f);
    if(T<.7f) return FMath::Lerp(Inside,Across,Ease((T-.55f)/.15f));
    if(T<.85f) return FMath::Lerp(Across,Up,Ease((T-.7f)/.15f));
    return FMath::Lerp(Up,Home,Ease((T-.85f)/.15f));
}
void AIndigoCraftPreview::UpdateClothParameters()
{
    const FIndigoResult& Result=Flow?Flow->Result:DisplayedResult;
    const float Left=FMath::Max(1,static_cast<int32>(Result.Left));
    const float Right=FMath::Max(1,static_cast<int32>(Result.Right));
    const float Depth=FMath::Lerp(ColorDepthForStage(PreviewDepth),ColorDepthForStage(PreviewDepth+1),DyeProgress);
    const FLinearColor Shades[]={FLinearColor(.92f,.93f,.90f),FLinearColor(.12f,.29f,.52f),FLinearColor(.035f,.12f,.32f),FLinearColor(.012f,.045f,.16f),FLinearColor(.004f,.012f,.065f)};
    const FLinearColor Color=FMath::Lerp(Shades[PreviewDepth],Shades[FMath::Min(4,PreviewDepth+1)],DyeProgress);
    for(UMaterialInstanceDynamic* Material:{ClothMaterial.Get(),SwatchMaterial.Get(),TrialMaterial.Get()}) if(Material)
    {
        Material->SetScalarParameterValue(TEXT("SelectIndex"),Left);
        Material->SetScalarParameterValue(TEXT("SelectIndex1"),Left);
        Material->SetScalarParameterValue(TEXT("SelectIndex2"),1.f);
        Material->SetScalarParameterValue(TEXT("SelectIndex3"),Right);
        Material->SetScalarParameterValue(TEXT("ColorDepth"),Depth);
        Material->SetScalarParameterValue(TEXT("RevealPattern"),Material==ClothMaterial.Get() && bUnfolded?1.f:0.f);
        // The authored graph multiplies ClothBaseColor by ColorDepth. Feed the
        // actual graph, not just the obsolete DyeColor parameter.
        Material->SetVectorParameterValue(TEXT("ClothBaseColor"),Color/FMath::Max(.001f,Depth));
        Material->SetVectorParameterValue(TEXT("ClothFigureColor"),FLinearColor(.92f,.93f,.90f,1.f));
    }
}

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
    TrialCloth=Part(TEXT("TrialCloth"),RootComponent);
    for(int32 I=0;I<5;++I) FuelMarkers.Add(Part(*FString::Printf(TEXT("Fuel%d"),I),RootComponent));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    DyeBasin->SetStaticMesh(Cylinder.Object); DyeWater->SetStaticMesh(Cylinder.Object);
    WashBasin->SetStaticMesh(Cylinder.Object); WashWater->SetStaticMesh(Cylinder.Object);
    ResetVisuals_Implementation();
}
void AIndigoCraftPreview::EndPlay(const EEndPlayReason::Type Reason)
{
    for(AIndigoCraftHandle* Handle:Handles) if(IsValid(Handle)) Handle->Destroy();
    Handles.Empty();Super::EndPlay(Reason);
}
void AIndigoCraftPreview::UpdateHandles()
{
    if(Handles.Num()!=4) return;
    EIndigoAction Actions[2]={EIndigoAction::Fold,EIndigoAction::Fold};
    FVector Sources[2]={FVector::ZeroVector,FVector::ZeroVector},Targets[2]={FVector::ZeroVector,FVector::ZeroVector};int32 Count=0;
    if(Flow->Step==EIndigoStep::Fold)
    {
        Count=1;const bool X=CompletedFolds%2==0;
        Sources[0]=ClothCenter+(X?FVector(ClothSize.X*.35,0,5):FVector(0,ClothSize.Y*.35,5));
        Targets[0]=ClothCenter+(X?FVector(-ClothSize.X*.35,0,5):FVector(0,-ClothSize.Y*.35,5));
    }
    else if(Flow->Step==EIndigoStep::Clamp)
    {
        Count=1;
        if(Flow->CanPerform(EIndigoAction::PlaceCloth))
        { Actions[0]=EIndigoAction::PlaceCloth;Sources[0]=ClothCenter+FVector(0,0,7);Targets[0]=FVector(0,52,10); }
        else if(Flow->CanPerform(EIndigoAction::ClosePlate))
        { Actions[0]=EIndigoAction::ClosePlate;Sources[0]=UpperPlate->GetComponentLocation()-GetActorLocation()+FVector(0,0,7);Targets[0]=FVector(0,52,13); }
        else
        { Count=2;Actions[0]=EIndigoAction::ClampLeft;Actions[1]=EIndigoAction::ClampRight;
          Sources[0]=FVector(45,52,35);Sources[1]=FVector(-45,52,35);Targets[0]=FVector(21,52,20);Targets[1]=FVector(-21,52,20); }
    }
    for(int32 I=0;I<2;++I) for(int32 J=0;J<2;++J)
    {
        const bool Enabled=I<Count && Flow->CanPerform(Actions[I],Actions[I]==EIndigoAction::Fold?Flow->FoldIndex:0);
        Handles[I*2+J]->Configure(Flow->Step,Actions[I],Actions[I]==EIndigoAction::Fold?Flow->FoldIndex:0,J==1,
            GetActorTransform().TransformPosition(J==1?Targets[I]:Sources[I]),Enabled);
    }
}
void AIndigoCraftPreview::BeginPlay()
{
    Super::BeginPlay();
    if(RuntimeClothMaterial)
        for(auto* Part:{Cloth.Get(),MovingCloth.Get(),DyeSwatch.Get(),TrialCloth.Get()}) Part->SetMaterial(0,RuntimeClothMaterial);
    ClothMaterial=Cloth->CreateDynamicMaterialInstance(0);
    if(ClothMaterial) MovingCloth->SetMaterial(0,ClothMaterial);
    SwatchMaterial=DyeSwatch->CreateDynamicMaterialInstance(0);
    TrialMaterial=TrialCloth->CreateDynamicMaterialInstance(0);
    ResetVisuals_Implementation();
    for(int32 I=0;I<4;++I)
    {
        FActorSpawnParameters Params;Params.Owner=this;
        Handles.Add(GetWorld()->SpawnActor<AIndigoCraftHandle>(GetActorLocation(),FRotator::ZeroRotator,Params));
    }
}
void AIndigoCraftPreview::PoseCloth()
{
    SetPartLocation(Cloth,ClothCenter); Cloth->SetRelativeRotation(ClothRotation);
    SetPartScale(Cloth,ClothSize/100.f);
}
void AIndigoCraftPreview::ResetVisuals_Implementation()
{
    PartCenters.Empty();
    MotionToken=0; ActiveCue=NAME_None; Elapsed=0.f;
    CompletedFolds=0; bPlaced=bClosed=bLeftClamped=bRightClamped=false;
    bUnfolded=false; DisplayedResult=FIndigoResult();
    ClothCenter=FVector(0,-20,3); ClothSize=FVector(108,76,.35f); PoseCloth();
    MovingCloth->SetVisibility(false);
    FoldLine->SetVisibility(false);
    FoldPivot->SetRelativeTransform(FTransform::Identity);
    SetPartLocation(LowerPlate,FVector(0,52,2)); SetPartScale(LowerPlate,FVector(.46,.44,.04));
    PlatePivot->SetRelativeLocation(FVector(-23,52,9)); PlatePivot->SetRelativeRotation(FRotator(110,0,0));
    SetPartLocation(UpperPlate,FVector(23,0,0)); SetPartScale(UpperPlate,FVector(.46,.44,.035));
    SetPartLocation(LeftClamp,FVector(21,52,24)); SetPartLocation(RightClamp,FVector(-21,52,24));
    SetPartScale(LeftClamp,FVector(.06,.54,.05)); SetPartScale(RightClamp,FVector(.06,.54,.05));
    SetPartLocation(DyeBasin,FVector(BasinDistance,52,28)); SetPartScale(DyeBasin,FVector(BasinDiameter/100.f,BasinDiameter/100.f,.56));
    SetPartLocation(DyeWater,FVector(BasinDistance,52,53)); SetPartScale(DyeWater,FVector((BasinDiameter-10.f)/100.f,(BasinDiameter-10.f)/100.f,.01));
    SetPartLocation(DyeSwatch,FVector(0,-28,16)); SetPartScale(DyeSwatch,FVector(.48,.32,.005)); DyeSwatch->SetVisibility(false);
    DyeBasin->SetVisibility(false); DyeWater->SetVisibility(false); DyeStartLocations.Empty();
    SetPartLocation(WashBasin,FVector(-BasinDistance,52,28)); SetPartScale(WashBasin,FVector(BasinDiameter/100.f,BasinDiameter/100.f,.56));
    SetPartLocation(WashWater,FVector(-BasinDistance,52,53)); SetPartScale(WashWater,FVector((BasinDiameter-10.f)/100.f,(BasinDiameter-10.f)/100.f,.01));
    WashBasin->SetVisibility(false); WashWater->SetVisibility(false);
    SetPartLocation(TrialCloth,FVector(BasinDistance,-6,18));SetPartScale(TrialCloth,FVector(.22,.10,.005));TrialCloth->SetVisibility(false);
    if(TrialMaterial) TrialMaterial->SetVectorParameterValue(TEXT("DyeColor"),FLinearColor(.8,.85,.76));
    for(int32 I=0;I<FuelMarkers.Num();++I)
    {
        SetPartLocation(FuelMarkers[I],FVector(62+I*14,104,5));SetPartScale(FuelMarkers[I],FVector(.10,.12,.10));FuelMarkers[I]->SetVisibility(false);
    }
    if(ClothMaterial)
    {
        ClothMaterial->SetScalarParameterValue(TEXT("ShowPattern"),0.f);
        if(UndyedTexture) ClothMaterial->SetTextureParameterValue(TEXT("ResultTexture"),UndyedTexture);
    }
    SetDyeProgress_Implementation(0,0.f);
}
void AIndigoCraftPreview::ShowFinalQuilt_Implementation(const FIndigoResult& Result)
{
    DisplayedResult=Result; bUnfolded=true;
    ClothCenter=FVector(0,-20,3); ClothSize=FVector(108,76,.35f); PoseCloth();
    SetDyeProgress_Implementation(Result.Depth,0.f);
    if(ClothMaterial)
    {
        ClothMaterial->SetScalarParameterValue(TEXT("PatternLeft"),static_cast<uint8>(Result.Left));
        ClothMaterial->SetScalarParameterValue(TEXT("PatternRight"),static_cast<uint8>(Result.Right));
        ClothMaterial->SetScalarParameterValue(TEXT("ShowPattern"),1.f);
        const int32 TextureIndex=((static_cast<int32>(Result.Left)-1)*3+static_cast<int32>(Result.Right)-1)*5+FMath::Clamp(Result.Depth,0,4);
        if(ResultTextures.IsValidIndex(TextureIndex) && ResultTextures[TextureIndex])
            ClothMaterial->SetTextureParameterValue(TEXT("ResultTexture"),ResultTextures[TextureIndex]);
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
    UpdateClothParameters();
}
void AIndigoCraftPreview::PlayIndigoAnimation_Implementation(FName Cue,int32 Token)
{
    if (!IsValid(Flow) || Token!=Flow->GetAnimationToken() || !Flow->bAnimationBusy) return;
    if (Cue!=TEXT("Fold") && Cue!=TEXT("PlaceCloth") && Cue!=TEXT("ClosePlate") && Cue!=TEXT("ClampLeft") && Cue!=TEXT("ClampRight") && Cue!=TEXT("Dye") && Cue!=TEXT("Wash") && Cue!=TEXT("Drain") && Cue!=TEXT("UnclampLeft") && Cue!=TEXT("UnclampRight") && Cue!=TEXT("Unfold") && Cue!=TEXT("Deliver") && Cue!=TEXT("Ship") && Cue!=TEXT("ScoopTest") && Cue!=TEXT("DipTest") && Cue!=TEXT("LiftTest") && Cue!=TEXT("OxidizeTest") && Cue!=TEXT("AddFuel")) return;
    MotionToken=Token; ActiveCue=Cue; Elapsed=0.f;
    if(Cue==TEXT("ScoopTest") && TrialMaterial) TrialMaterial->SetVectorParameterValue(TEXT("DyeColor"),FLinearColor(.8,.85,.76));
    if (Cue==TEXT("Fold"))
    {
        bFoldX=CompletedFolds%2==0;
        const FVector HalfSize=ClothSize* (bFoldX?FVector(.5,1,1):FVector(1,.5,1));
        const FVector Offset=bFoldX?FVector(ClothSize.X*.25,0,0):FVector(0,ClothSize.Y*.25,0);
        FoldPivot->SetRelativeLocation(ClothCenter+FVector(0,0,ClothSize.Z*.5f)); FoldPivot->SetRelativeRotation(FRotator::ZeroRotator);
        SetPartLocation(Cloth,ClothCenter-Offset); SetPartScale(Cloth,HalfSize/100.f);
        SetPartLocation(MovingCloth,Offset-FVector(0,0,ClothSize.Z*.5f)); MovingCloth->SetRelativeRotation(ClothRotation);
        SetPartScale(MovingCloth,HalfSize/100.f); MovingCloth->SetVisibility(true);
    }
    if (Cue==TEXT("PlaceCloth")) MoveStart=ClothCenter;
    if (Cue==TEXT("Dye") || Cue==TEXT("Wash") || Cue==TEXT("Drain"))
    {
        DyeStartLocations.Empty();
        for(USceneComponent* Part:DyeParts()) DyeStartLocations.Add(PartCenter(Part));
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
        SetPartLocation(MovingCloth,(bFoldX?FVector(ClothSize.X*.25,0,0):FVector(0,ClothSize.Y*.25,0))-FVector(0,0,ClothSize.Z*.5f));
    }
    else if (ActiveCue==TEXT("PlaceCloth"))
        SetPartLocation(Cloth,FMath::Lerp(MoveStart,FVector(0,52,5.5),Smooth)+FVector(0,0,12.f*FMath::Sin(PI*T)));
    else if (ActiveCue==TEXT("ClosePlate")) PlatePivot->SetRelativeRotation(FRotator(FMath::Lerp(110.f,0.f,Smooth),0,0));
    else if (ActiveCue==TEXT("ClampLeft")) SetPartLocation(LeftClamp,FVector(21,52,FMath::Lerp(24.f,11.f,Smooth)));
    else if (ActiveCue==TEXT("ClampRight")) SetPartLocation(RightClamp,FVector(-21,52,FMath::Lerp(24.f,11.f,Smooth)));
    else if (ActiveCue==TEXT("UnclampLeft")) SetPartLocation(LeftClamp,FMath::Lerp(FVector(21,52,11),FVector(45,52,24),Smooth));
    else if (ActiveCue==TEXT("UnclampRight")) SetPartLocation(RightClamp,FMath::Lerp(FVector(-21,52,11),FVector(-45,52,24),Smooth));
    else if (ActiveCue==TEXT("Wash") || ActiveCue==TEXT("Drain"))
    {
        FVector Offset;
        if(ActiveCue==TEXT("Wash"))
            Offset=BasinMotion(T,true);
        else Offset=FVector(0,0,22*FMath::Sin(PI*T));
        const auto Parts=DyeParts();
        if(DyeStartLocations.Num()==Parts.Num()) for(int32 I=0;I<Parts.Num();++I) SetPartLocation(Parts[I],DyeStartLocations[I]+Offset);
    }
    else if (ActiveCue==TEXT("Unfold"))
    {
        PlatePivot->SetRelativeRotation(FRotator(110*FMath::Clamp(T*4.f,0.f,1.f),0,0));
        const float U=FMath::Clamp((T-.25f)/.75f,0.f,1.f);
        ClothCenter=FMath::Lerp(MoveStart,FVector(0,-20,3),U);
        // Reverse the three blockout folds one at a time, then restore a flat sheet.
        const float X=FMath::Clamp(U*3.f,0.f,1.f)+FMath::Clamp(U*3.f-2.f,0.f,1.f);
        const float Y=FMath::Clamp(U*3.f-1.f,0.f,1.f);
        ClothSize=UnfoldStartSize*FVector(FMath::Pow(2.f,X),FMath::Pow(2.f,Y),FMath::Pow(.5f,X+Y)); PoseCloth();
    }
    else if (ActiveCue==TEXT("Deliver")) SetPartLocation(Cloth,ClothCenter+FVector(0,0,6*FMath::Sin(PI*T)));
    else if (ActiveCue==TEXT("Ship"))
    {
        SetPartLocation(Cloth,ClothCenter+FVector(0,70*Smooth,4*FMath::Sin(PI*T)));
        SetPartScale(Cloth,FMath::Lerp(FVector(1.08,.76,.0035),FVector(.54,.38,.014),Smooth));
    }
    else if (ActiveCue==TEXT("ScoopTest")) SetPartLocation(TrialCloth,FVector(BasinDistance,-6+30*FMath::Sin(PI*T),18));
    else if (ActiveCue==TEXT("DipTest")) SetPartLocation(TrialCloth,FMath::Lerp(FVector(BasinDistance,-6,18),FVector(BasinDistance,52,46),Smooth));
    else if (ActiveCue==TEXT("LiftTest")) SetPartLocation(TrialCloth,FMath::Lerp(FVector(BasinDistance,52,46),FVector(BasinDistance,-6,18),Smooth));
    else if (ActiveCue==TEXT("OxidizeTest") && TrialMaterial)
        TrialMaterial->SetVectorParameterValue(TEXT("DyeColor"),FMath::Lerp(FLinearColor(.8,.85,.76),Flow->Temperature==EIndigoTemperature::Ready?FLinearColor(.065,.2,.44):FLinearColor(.48,.57,.53),Smooth));
    else if (ActiveCue==TEXT("AddFuel"))
    {
        const int32 Index=FMath::Clamp(Flow->Fuel,0,FuelMarkers.Num()-1);
        SetPartLocation(FuelMarkers[Index],FVector(62+Index*14,104,5)+FVector(0,-52*Smooth,-12*Smooth));
        FuelMarkers[Index]->SetVisibility(T<.9f);
    }
    else if (ActiveCue==TEXT("Dye"))
    {
        // Clear the rim before translating horizontally in either direction.
        const FVector Offset=BasinMotion(T,false);
        const auto Parts=DyeParts();
        if(DyeStartLocations.Num()==Parts.Num()) for(int32 I=0;I<Parts.Num();++I) SetPartLocation(Parts[I],DyeStartLocations[I]+Offset);
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
    UpdateHandles();
    UpdateClothParameters();
    const bool Trial=Flow->Step==EIndigoStep::Trial;
    const bool DyeStage=Flow->Step==EIndigoStep::Dye || Flow->Step==EIndigoStep::Wash || Trial;
    const bool Final=Flow->Step==EIndigoStep::Reveal || Flow->Step==EIndigoStep::End || Flow->Step==EIndigoStep::Delivery;
    const bool Craft=Flow->Step==EIndigoStep::Fold || Flow->Step==EIndigoStep::Clamp || DyeStage || Final;
    SetActorHiddenInGame(!Craft);
    for (AActor* Sample:SelectionSamples) if (IsValid(Sample)) Sample->SetActorHiddenInGame(Craft);
    DyeBasin->SetVisibility(Craft); DyeWater->SetVisibility(Craft);
    DyeSwatch->SetVisibility(DyeStage && !Trial);TrialCloth->SetVisibility(Trial);
    Cloth->SetVisibility(!Trial);
    for(int32 I=0;I<FuelMarkers.Num();++I) FuelMarkers[I]->SetVisibility(Flow->bEnableTemperatureRules && (Trial || Flow->Step==EIndigoStep::Dye) && I<Flow->Fuel);
    WashBasin->SetVisibility(Craft); WashWater->SetVisibility(Craft);
    const bool Assembly=Flow->Step==EIndigoStep::Clamp || Flow->Step==EIndigoStep::Dye || Flow->Step==EIndigoStep::Wash;
    LowerPlate->SetVisibility(Assembly); UpperPlate->SetVisibility(Assembly); LeftClamp->SetVisibility(Assembly); RightClamp->SetVisibility(Assembly);
    FoldLine->SetVisibility(Flow->Step==EIndigoStep::Fold && !Flow->bAnimationBusy);
    SetPartLocation(FoldLine,ClothCenter+FVector(0,0,ClothSize.Z*.5f+.2f));
    SetPartScale(FoldLine,CompletedFolds%2==0?FVector(.008f,ClothSize.Y/100.f,.003f):FVector(ClothSize.X/100.f,.008f,.003f));
    if (!MotionToken) return;
    // An external reset, completion, or step change must never let an old motion finish a newer one.
    if (!Flow->bAnimationBusy || Flow->GetAnimationToken()!=MotionToken) { MotionToken=0; ActiveCue=NAME_None; MovingCloth->SetVisibility(false); return; }
    Elapsed+=FMath::Max(0.f,DeltaSeconds);
    const float T=FMath::Clamp(Elapsed/FMath::Max(.1f,ActiveCue==TEXT("Dye")?DyeSeconds:MotionSeconds),0.f,1.f);
    ApplyMotion(T);
    if (T>=1.f) FinishMotion();
}
