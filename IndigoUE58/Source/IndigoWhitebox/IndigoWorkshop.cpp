#include "IndigoWorkshop.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

static FLinearColor Blue(int32 Depth) {
    static FColor Colors[]={FColor(195,207,202),FColor(126,171,193),FColor(64,121,159),FColor(35,83,116),FColor(23,57,80)};
    return FLinearColor(Colors[FMath::Clamp(Depth,0,4)]);
}
static FString PatternName(int32 V) { return V<0?TEXT("—"):FString::Chr(TCHAR('A'+V)); }

AIndigoWorkbench::AIndigoWorkbench() {
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Piece("Floor",FVector(0,0,-12),FVector(100,100,.2f),FLinearColor(.65f,.7f,.68f));
    Piece("Table",FVector(0,0,105),FVector(5.4f,3,.18f),FLinearColor(.66f,.69f,.62f));
    for(int32 X:{-235,235}) for(int32 Y:{-115,115}) Piece(FName(*FString::Printf(TEXT("Leg%d_%d"),X,Y)),FVector(X,Y,48),FVector(.16f,.16f,1),FLinearColor(.5f,.56f,.53f));
    Piece("Vat",FVector(365,0,58),FVector(2.05f,2.05f,1.2f),FLinearColor(.43f,.5f,.5f),true);
    Piece("DyeLiquid",FVector(365,0,119),FVector(1.83f,1.83f,.025f),FLinearColor(.13f,.25f,.28f),true);
    Piece("WashTub",FVector(-360,40,26),FVector(1.4f,1.4f,.52f),FLinearColor(.55f,.66f,.68f),true);
    Piece("Water",FVector(-360,40,53),FVector(1.25f,1.25f,.025f),FLinearColor(.32f,.55f,.63f),true);
    Piece("Hearth",FVector(365,145,18),FVector(.7f,.65f,.3f),FLinearColor(.5f,.52f,.44f));
    Piece("FuelBox",FVector(355,-160,22),FVector(1.4f,.6f,.4f),FLinearColor(.65f,.62f,.49f));
    for(int32 I=0;I<5;++I) Piece(FName(*FString::Printf(TEXT("Fuel%d"),I)),FVector(305+I*24,-160,48),FVector(.18f,.4f,.15f),FLinearColor(.78f,.7f,.5f));
    for(int32 I=0;I<3;++I) Piece(FName(*FString::Printf(TEXT("PatternBoard%d"),I)),FVector(-130+I*130,-180,158),FVector(.95f,.15f,.85f),FLinearColor(.7f,.74f,.65f));
    Cloth=Piece("Quilt",FVector(0,0,116),FVector(4.4f,2.55f,.015f),FLinearColor::White);
    Cloth->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));
    Bundle=Piece("FoldedBundle",FVector(0,0,126),FVector(1.5f,.95f,.15f),Blue(0));
    Lid=Piece("UpperBoard",FVector(0,0,140),FVector(1.8f,1.2f,.1f),FLinearColor(.57f,.65f,.55f));
    LeftClip=Piece("LeftClip",FVector(-72,0,137),FVector(.12f,1.35f,.35f),FLinearColor(.3f,.4f,.43f));
    RightClip=Piece("RightClip",FVector(72,0,137),FVector(.12f,1.35f,.35f),FLinearColor(.3f,.4f,.43f));
    Strip=Piece("TestStrip",FVector(360,25,145),FVector(.15f,.6f,.025f),Blue(0));
}
UStaticMeshComponent* AIndigoWorkbench::Piece(FName Name,FVector Pos,FVector Size,FLinearColor Color,bool Cylinder) {
    auto* M=CreateDefaultSubobject<UStaticMeshComponent>(Name); M->SetupAttachment(RootComponent);
    M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Cylinder?TEXT("/Engine/BasicShapes/Cylinder.Cylinder"):TEXT("/Engine/BasicShapes/Cube.Cube")));
    M->SetRelativeLocation(Pos); M->SetRelativeScale3D(Size); M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    M->ComponentTags.Add(FName(*FString::Printf(TEXT("%f,%f,%f"),Color.R,Color.G,Color.B)));
    Parts.Add(M); return M;
}
void AIndigoWorkbench::SetColor(UStaticMeshComponent* M,FLinearColor Color) {
    UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Indigo/Materials/M_Surface.M_Surface"));
    if(!Base) return;
    auto* Dynamic=UMaterialInstanceDynamic::Create(Base,this); Dynamic->SetVectorParameterValue(TEXT("Color"),Color); M->SetMaterial(0,Dynamic);
}
void AIndigoWorkbench::OnConstruction(const FTransform& Transform) {
    Super::OnConstruction(Transform);
    for(auto* P:Parts) if(P && P->ComponentTags.Num()) {
        TArray<FString> Values; P->ComponentTags[0].ToString().ParseIntoArray(Values,TEXT(","));
        if(Values.Num()==3) SetColor(P,FLinearColor(FCString::Atof(*Values[0]),FCString::Atof(*Values[1]),FCString::Atof(*Values[2])));
    }
    auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Indigo/Materials/M_Cloth.M_Cloth"));
    if(Mat) { ClothMaterial=UMaterialInstanceDynamic::Create(Mat,this); Cloth->SetMaterial(0,ClothMaterial); }
    BundleMaterial=Cast<UMaterialInstanceDynamic>(Bundle->GetMaterial(0));
    StripMaterial=Cast<UMaterialInstanceDynamic>(Strip->GetMaterial(0));
    RefreshArt();
}
void AIndigoWorkbench::BeginPlay() {
    Super::BeginPlay(); Session.Reset(); RefreshArt();
    Camera=GetWorld()->SpawnActor<ACameraActor>();
    Camera->SetActorLocation(FVector(640,900,880)); Camera->SetActorRotation((FVector(30,0,80)-Camera->GetActorLocation()).Rotation());
    Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
    Camera->GetCameraComponent()->SetOrthoWidth(1350);
    Camera->GetCameraComponent()->bConstrainAspectRatio=false;
    Camera->GetCameraComponent()->bAutoCalculateOrthoPlanes=true;
    if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(Camera);
}
UTexture2D* AIndigoWorkbench::GetQuiltTexture(bool Preview) const {
    const TCHAR L=Session.LeftPattern<0?TCHAR('X'):TCHAR('A'+Session.LeftPattern),R=Session.RightPattern<0?TCHAR('X'):TCHAR('A'+Session.RightPattern);
    const FString Name=FString::Printf(TEXT("T_%c%c_%d"),L,R,Preview?0:Session.DyeDepth);
    return LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/Indigo/Textures/")+Name+TEXT(".")+Name));
}
void AIndigoWorkbench::RefreshArt() {
    using E=EIndigoStep; auto Step=Session.Step;
    const bool Final=Step>=E::Reveal;
    if(ClothMaterial) ClothMaterial->SetTextureParameterValue(TEXT("Pattern"),GetQuiltTexture(!Final));
    Cloth->SetVisibility(Step<=E::Fold || Final);
    Cloth->SetRelativeLocation(FVector(0,0,116));
    Cloth->SetRelativeScale3D(Step==E::Fold?FVector(4.4f-Session.FoldCount,2.55f-Session.FoldCount*.35f,1):FVector(4.4f,2.55f,1));
    Bundle->SetVisibility(Step>=E::Clamp && Step<=E::Wash);
    Lid->SetVisibility(Step>=E::Clamp && Step<=E::Wash);
    Lid->SetRelativeLocation(FVector(Step==E::Clamp&&Session.ClampPhase<2?-175:0,0,140));
    Bundle->SetRelativeLocation(FVector(Step==E::Clamp&&Session.ClampPhase==0?-175:0,0,126));
    LeftClip->SetVisibility(Session.ClipLeft&&!Session.OpenLeft&&Step<=E::Wash);
    RightClip->SetVisibility(Session.ClipRight&&!Session.OpenRight&&Step<=E::Wash);
    Strip->SetVisibility(Step==E::Test||Step==E::Retest);
    if(StripMaterial) StripMaterial->SetVectorParameterValue(TEXT("Color"),Session.TestPhase==4?Blue(Step==E::Test?0:2):FLinearColor(.32f,.48f,.36f));
    if(BundleMaterial) BundleMaterial->SetVectorParameterValue(TEXT("Color"),Blue(Session.DyeDepth));
    for(auto* P:Parts) if(P->GetName().StartsWith(TEXT("Fuel")) && P->GetName()!=TEXT("FuelBox")) { int32 I=FCString::Atoi(*P->GetName().Right(1)); P->SetVisibility(I<Session.Fuel); }
}
bool AIndigoWorkbench::Command(const FString& Action,int32 Value) {
    bool Changed=Session.Act(Action,Value); if(Changed) RefreshArt(); return Changed;
}
void AIndigoWorkbench::Tick(float Delta) {
    Super::Tick(Delta); if(Session.Advance(Delta)) RefreshArt();
    if(Camera) {
        const bool Final=Session.Step>=EIndigoStep::Reveal;
        const bool Tools=Session.Step==EIndigoStep::Test||Session.Step==EIndigoStep::Heat||Session.Step==EIndigoStep::Retest||Session.Step==EIndigoStep::Dye;
        const float Width=Final?1110:Tools?1220:1350;
        const FVector Focus=Tools?FVector(245,-100,100):FVector(150,-100,100);
        const FVector Eye=Focus+FVector(640,900,800);
        Camera->SetActorLocation(FMath::VInterpTo(Camera->GetActorLocation(),Eye,Delta,3));
        Camera->SetActorRotation((Focus-Eye).Rotation());
        auto* C=Camera->GetCameraComponent(); C->SetOrthoWidth(FMath::FInterpTo(C->OrthoWidth,Width,Delta,3));
    }
    if(Session.Pending=="dye") {
        const float P=Session.Progress(); FVector Pos(365,0,P<.44f?105-50*FMath::Sin(P/.44f*PI):155);
        Bundle->SetRelativeLocation(Pos); Lid->SetRelativeLocation(Pos+FVector(0,0,14));
        LeftClip->SetRelativeLocation(Pos+FVector(-72,0,10)); RightClip->SetRelativeLocation(Pos+FVector(72,0,10));
        if(BundleMaterial) BundleMaterial->SetVectorParameterValue(TEXT("Color"),FMath::Lerp(FLinearColor(.16f,.35f,.23f),Blue(Session.DyeDepth+1),FMath::Clamp((P-.62f)/.38f,0.f,1.f)));
    } else if(Session.Step==EIndigoStep::Dye || Session.Step==EIndigoStep::Wash) {
        FVector Pos=Session.Pending=="wash"&&Session.WashPhase==0?FVector(-360,40,65):FVector(0,0,126);
        Bundle->SetRelativeLocation(Pos); Lid->SetRelativeLocation(Pos+FVector(0,0,14));
        LeftClip->SetRelativeLocation(Pos+FVector(-72,0,10)); RightClip->SetRelativeLocation(Pos+FVector(72,0,10));
    }
}
AIndigoGameMode::AIndigoGameMode() { PlayerControllerClass=AIndigoController::StaticClass(); HUDClass=AIndigoHUD::StaticClass(); DefaultPawnClass=nullptr; }
void AIndigoController::BeginPlay() {
    Super::BeginPlay(); bShowMouseCursor=true; bEnableClickEvents=true;
    FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(Mode);
    for(TActorIterator<AIndigoWorkbench> It(GetWorld());It;++It) { Bench=*It; break; }
    if(!Bench) Bench=GetWorld()->SpawnActor<AIndigoWorkbench>();
}
void AIndigoController::Tell(const FString& Value) { Message=Value; MessageUntil=GetWorld()->GetTimeSeconds()+3; }
void AIndigoController::Activate(const FString& A) {
    if(A=="help") { Help=!Help; Held.Empty(); return; }
    if(!Bench || Bench->Session.Busy() || Help) return;
    if(A.StartsWith("pick")) { Held=A; Tell(TEXT("已拿起。拖到目标区域，或再点一次目标放下。")); return; }
    if(A=="left" || A=="right" || A=="foldTarget" || A=="boardTarget") { DropOn(A); return; }
    if(Bench->Command(A)) Held.Empty();
}
void AIndigoController::DropOn(const FString& Target) {
    if(!Bench || Bench->Session.Busy()) { Held.Empty(); return; }
    FString Action; int32 Value=-1;
    if(Target=="left" || Target=="right") {
        if(Held.StartsWith("pick") && Held.Len()==5) { Value=int32(Held[4]-'A'); if(Value<0||Value>2) return; }
        else if(!Held.IsEmpty()) return;
        Action=Target;
    } else if(Target=="foldTarget" && Held=="pickFold") Action="fold";
    else if(Target=="boardTarget" && Held=="pickCloth") Action="cloth";
    else if(Target=="boardTarget" && Held=="pickLid") Action="lid";
    if(!Action.IsEmpty()&&Bench->Command(Action,Value)) Held.Empty();
    else { Held.Empty(); Tell(TEXT("未对准，物件已复位。请放到标记区域。")); }
}
void AIndigoController::PlayerTick(float Delta) {
    Super::PlayerTick(Delta); auto* H=Cast<AIndigoHUD>(GetHUD()); if(!H||!Bench) return;
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("IndigoRenderCheck"))) {
        SmokeTime+=Delta;
        if(SmokeTime>8 && SmokePhase==0) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Indigo_Start.png"),true,false); ++SmokePhase; }
        else if(SmokeTime>10 && SmokePhase==1) { Bench->Session.LeftPattern=0;Bench->Session.RightPattern=1;Bench->Session.DyeDepth=4;Bench->Session.Fuel=1;Bench->Session.Enter(EIndigoStep::Reveal);Bench->RefreshArt();++SmokePhase; }
        else if(SmokeTime>13 && SmokePhase==2) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Indigo_Reveal.png"),true,false);++SmokePhase; }
        else if(SmokeTime>16 && SmokePhase==3) { ConsoleCommand(TEXT("quit"));++SmokePhase; }
    }
#endif
    float X=0,Y=0; const bool HasMouse=GetMousePosition(X,Y); FVector2D P(X,Y);
    if(!HasMouse || WasInputKeyJustPressed(EKeys::Escape)) { Held.Empty(); MouseHeld=false; return; }
    if(WasInputKeyJustPressed(EKeys::LeftMouseButton)) {
        const FString A=H->HitAt(P); PressAt=P; Moved=false; MouseHeld=true;
        if(A.StartsWith("pick")) Activate(A);
    }
    if(MouseHeld && FVector2D::Distance(P,PressAt)>7) Moved=true;
    if(WasInputKeyJustReleased(EKeys::LeftMouseButton) && MouseHeld) {
        MouseHeld=false; const FString A=H->HitAt(P);
        if(Moved && !Held.IsEmpty()) { if(A=="left"||A=="right"||A=="foldTarget"||A=="boardTarget") DropOn(A); else { Held.Empty(); Tell(TEXT("未对准，已回原位。可先点选，再点目标。")); } }
        else if(!A.StartsWith("pick")) Activate(A);
    }
    if(Bench->Session.Busy()) Held.Empty();
}
void AIndigoHUD::Rect(float X,float Y,float W,float H,FLinearColor Color) { DrawRect(Color,X*Scale,Y*Scale,W*Scale,H*Scale); }
void AIndigoHUD::Text(const FString& V,float X,float Y,float Size,FLinearColor Color) { DrawText(V,Color,X*Scale,Y*Scale,GEngine->GetMediumFont(),Size*Scale*1.65f); }
void AIndigoHUD::Button(const FString& Label,const FString& Action,float X,float Y,float W,bool Enabled) {
    const FLinearColor Fill=Enabled?FLinearColor(.87f,.92f,.92f):FLinearColor(.79f,.81f,.81f);
    Rect(X,Y,W,40,Fill); Text(Label,X+12,Y+9,.85f,Enabled?Ink:FLinearColor(.42f,.46f,.46f));
    Hits.Add({Action,FBox2D(FVector2D(X,Y)*Scale,FVector2D(X+W,Y+40)*Scale),Enabled});
}
void AIndigoHUD::Target(const FString& Label,const FString& Action,FVector World,bool Enabled) {
    FVector2D P; if(PlayerOwner->ProjectWorldLocationToScreen(World,P)) Button(Label,Action,P.X/Scale-65,P.Y/Scale-20,130,Enabled);
}
FString AIndigoHUD::HitAt(FVector2D P) const { for(int32 I=Hits.Num()-1;I>=0;--I) if(Hits[I].Enabled && Hits[I].Bounds.IsInside(P)) return Hits[I].Action; return FString(); }
void AIndigoHUD::DrawHUD() {
    Super::DrawHUD(); if(!Canvas) return; Hits.Reset();
    auto* PC=Cast<AIndigoController>(PlayerOwner); if(!PC||!PC->Bench) return;
    auto* B=PC->Bench; auto& S=B->Session; using E=EIndigoStep;
    Scale=FMath::Max(.5f,FMath::Min(Canvas->SizeX/1280.f,Canvas->SizeY/800.f));
    float W=Canvas->SizeX/Scale,H=Canvas->SizeY/Scale,R=W-310,Y=168;
    Rect(0,0,W,82,FLinearColor(.93f,.95f,.92f)); Text(TEXT("留一处旧样"),24,16,1.6f);
    Text(TEXT("INDIGO / UE 5.8 NATIVE WHITEBOX"),25,54,.65f); Button(TEXT("操作说明"),"help",W-146,20,122);
    Rect(R,82,310,H-82,FLinearColor(.93f,.95f,.92f));
    Text(TEXT("温州染坊 · 可玩白盒"),R+20,99,.7f); Text(S.Title(),R+20,127,1.15f);
    auto Add=[&](const FString& Label,const FString& Action,bool Enabled=true){Button(Label,Action,R+20,Y,270,Enabled&&!S.Busy());Y+=49;};
    if(S.Step==E::Receive) {
        Add(S.SawOld?TEXT("已查看旧布角"):TEXT("查看旧布角"),"old"); Add(S.SawLetter?TEXT("已读委托单正面"):TEXT("读委托单正面"),"letter"); Add(TEXT("接下委托  >"),"next",S.SawOld&&S.SawLetter);
        Target(TEXT("旧布角"),"old",FVector(-120,0,125),!S.Busy()); Target(TEXT("委托单"),"letter",FVector(125,0,125),!S.Busy());
    } else if(S.Step==E::Pattern) {
        Add(TEXT("A · 菱纹      拖到布面"),"pickA"); Add(TEXT("B · 花纹      拖到布面"),"pickB"); Add(TEXT("C · 水纹      拖到布面"),"pickC"); Add(TEXT("交换左右"),"swap"); Add(TEXT("确认排版  >"),"next",S.LeftPattern>=0&&S.RightPattern>=0);
        Target(TEXT("左侧 / ")+PatternName(S.LeftPattern),"left",FVector(-135,0,125)); Target(TEXT("右侧 / ")+PatternName(S.RightPattern),"right",FVector(135,0,125));
    } else if(S.Step==E::Test || S.Step==E::Retest) {
        if(S.TestPhase<3) Add(S.TestPhase==0?TEXT("舀起一勺染液"):S.TestPhase==1?TEXT("浸入试布"):TEXT("提出，等待显色"),"test");
        else if(S.TestPhase==4) Add(S.Step==E::Test?TEXT("去补温  >"):TEXT("开始叠布  >"),"next"); else Add(TEXT("观察显色中…"),"",false);
    } else if(S.Step==E::Heat) Add(TEXT("添一份谷壳"),"heat",!S.Ready&&S.Fuel>0);
    else if(S.Step==E::Fold) {
        Add(FString::Printf(TEXT("拿起第 %d / 3 折布边"),S.FoldCount+1),"pickFold");
        Target(TEXT("布边 →"),"pickFold",FVector(-130,30,125),!S.Busy()); Target(TEXT("折线 / 放到这里"),"foldTarget",FVector(100,0,125),!S.Busy());
    } else if(S.Step==E::Clamp) {
        if(S.ClampPhase<2) { Add(S.ClampPhase==0?TEXT("拿起折好的布"):TEXT("拿起上花版"),S.ClampPhase==0?"pickCloth":"pickLid"); Target(TEXT("下版定位区"),"boardTarget",FVector(0,0,150),!S.Busy()); }
        else { Add(S.ClipLeft?TEXT("左侧已扣紧"):TEXT("扣紧左夹具"),"clipL",!S.ClipLeft); Add(S.ClipRight?TEXT("右侧已扣紧"):TEXT("扣紧右夹具"),"clipR",!S.ClipRight); }
    } else if(S.Step==E::Dye) {
        if(S.DyeDepth<4) { Add(FString::Printf(TEXT("入染 · 第 %d 档"),S.DyeDepth+1),"dye",S.Ready); Add(TEXT("添一份谷壳"),"heat",!S.Ready&&S.Fuel>0); }
        Add(S.DyeDepth>0?TEXT("收工 · 留下现在的蓝"):TEXT("完成第一档后可收工"),"finish",S.DyeDepth>0);
    } else if(S.Step==E::Wash) {
        if(S.WashPhase<2) Add(S.WashPhase==0?TEXT("放入清水清洗"):TEXT("提起沥水"),"wash");
        else { Add(S.OpenLeft?TEXT("左侧已松开"):TEXT("松开左夹具"),"openL",!S.OpenLeft); Add(S.OpenRight?TEXT("右侧已松开"):TEXT("松开右夹具"),"openR",!S.OpenRight); Add(TEXT("打开花版，展开被面"),"next",S.OpenLeft&&S.OpenRight); }
    } else if(S.Step==E::Reveal) Add(TEXT("交付 · 打开委托后页"),"next");
    else if(S.Step==E::Letter) {
        const TArray<FString> Lines={TEXT("我成婚时，母亲送过我一床"),TEXT("蓝夹缬被面。如今女儿也要"),TEXT("成婚了，想给她做一床新的。"),TEXT("旧样留在我这里，"),TEXT("请把新的寄给她。")};
        for(const FString& L:Lines) { Text(L,R+20,Y,.88f);Y+=30; } Y+=10; Add(TEXT("读完了，去叠布寄件"),"next");
    } else if(S.Step==E::Pack) Add(TEXT("叠好被面，装入包裹"),"next");
    else if(S.Step==E::End) { Add(TEXT("回看作品"),"review"); Add(TEXT("重新开始"),"restart"); }
    else if(S.Step==E::Review) Add(TEXT("返回结束页"),"next");
    else Add(TEXT("正在播放结尾…"),"",false);
    Y=FMath::Max(Y+15,470.f);
    Text(TEXT("左侧 / 右侧    ")+PatternName(S.LeftPattern)+TEXT(" / ")+PatternName(S.RightPattern),R+20,Y,.85f); Y+=33;
    Text(S.Ready?TEXT("缸温   合适 / 可入染"):TEXT("缸温   偏冷 / 待补温"),R+20,Y,.85f);Y+=33;
    Text(FString::Printf(TEXT("谷壳   %d / 5"),S.Fuel),R+20,Y,.85f);Y+=33;
    Text(FString::Printf(TEXT("染色   %d / 4 档"),S.DyeDepth),R+20,Y,.85f);Y+=35;
    for(int32 I=1;I<=4;++I) Rect(R+20+(I-1)*65,Y,57,6,I<=S.DyeDepth?Blue(I):FLinearColor(.7f,.76f,.76f));
    Text(TEXT("占位纹样 · 简化工艺规则"),R+20,H-48,.67f);
    Rect(22,H-116,R-45,94,FLinearColor(.93f,.96f,.94f,.96f)); Text(TEXT("匠人的心声"),40,H-104,.7f);
    FString Hint=S.Hint(); if(Hint.Len()>38) Hint.InsertAt(38,TEXT("\n")); Text(Hint,40,H-82,.87f);
    if(S.Busy()) { Rect(40,H-38,R-82,3,FLinearColor(.7f,.8f,.8f)); Rect(40,H-38,(R-82)*S.Progress(),3,FLinearColor(.15f,.35f,.45f)); }
    if(S.Step==E::FilmOne||S.Step==E::FilmTwo||S.Step==E::Sky) {
        Rect(0,82,R,H-215,S.Step==E::Sky?FLinearColor(.3f,.56f,.72f):FLinearColor(.075f,.16f,.25f));
        Text(S.Title(),R*.18f,H*.35f,2,FLinearColor::White);
        Text(S.Step==E::Sky?TEXT("这件做到这里，就差不多了。"):TEXT("15 秒分镜占位 / 地区、工艺、正式影像待接入"),R*.12f,H*.46f,.9f,FLinearColor(.8f,.9f,1));
    }
    if(GetWorld()->GetTimeSeconds()<PC->MessageUntil) { Rect(25,96,R-50,38,FLinearColor(.18f,.3f,.36f)); Text(PC->Message,36,105,.8f,FLinearColor::White); }
    if(!PC->Held.IsEmpty()) { float X,YMouse; if(PC->GetMousePosition(X,YMouse)) { Rect(X/Scale+15,YMouse/Scale+12,132,34,FLinearColor(.14f,.3f,.4f)); Text(TEXT("已拿起 → 放到标记"),X/Scale+22,YMouse/Scale+20,.65f,FLinearColor::White); } }
    if(PC->Help) {
        Hits.Reset(); Rect(W/2-340,H/2-210,680,420,FLinearColor(.94f,.96f,.93f)); Text(TEXT("制作一床属于你的蓝"),W/2-305,H/2-177,1.5f);
        const TArray<FString> HelpLines={TEXT("花版、布边和花版上盖：拖放，或先点选再点目标。"),TEXT("试布 → 补温复试 → 叠布 → 夹版 → 入染。"),TEXT("每段染色结束后缸温回落，再染前添一份谷壳。"),TEXT("至少一档可收工，最多四档；动画期间请等待。"),TEXT("展开后可停留，点击交付才会打开委托后页。"),TEXT("Esc 取消拿取；Shift+F1 释放编辑器中的鼠标。"),TEXT("几何、纹样与地域短片为白盒占位，无正式音频。")};
        float HY=H/2-125;for(const FString& L:HelpLines){Text(L,W/2-305,HY,.85f);HY+=32;}Button(TEXT("知道了"),"help",W/2+155,H/2+140,150);
    }
}
