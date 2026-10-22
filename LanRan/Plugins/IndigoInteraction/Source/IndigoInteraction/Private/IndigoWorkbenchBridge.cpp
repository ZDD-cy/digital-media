#include "IndigoWorkbenchBridge.h"
#include "IndigoInterfaceWidget.h"
#include "IndigoOpeningVideo.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "IndigoInteractable.h"
#include "IndigoPatternSource.h"
#include "IndigoCraftHandle.h"
#include "TimerManager.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "FileMediaSource.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AIndigoWorkbenchBridge::AIndigoWorkbenchBridge()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    PrimaryActorTick.bCanEverTick = true;
    OpeningVideo=CreateDefaultSubobject<UIndigoOpeningVideoComponent>(TEXT("OpeningVideo"));
    OpeningVideo->bPlayOnStart = false;
    BackgroundMusicAsset = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/SW_BackgroundMusic.SW_BackgroundMusic")));
}
void AIndigoWorkbenchBridge::BeginPlay()
{
    Super::BeginPlay();
    Player = UGameplayStatics::GetPlayerController(this, 0);
    if (!Player || !Player->IsLocalController()) { SetActorTickEnabled(false); return; }
    if (!Director) for (TActorIterator<AIndigoDirector> It(GetWorld()); It; ++It) { Director = *It; break; }
    if (UIndigoFlowComponent* Flow=GetFlow())
    {
        Flow->OnStepChanged.AddDynamic(this,&AIndigoWorkbenchBridge::HandleStep);
        Flow->OnSubtitle.AddDynamic(this,&AIndigoWorkbenchBridge::HandleSubtitle);
        Flow->OnInteractionInvalidated.AddDynamic(this,&AIndigoWorkbenchBridge::CancelDrag);
        if (Flow->bSkipReceipt && Flow->Step==EIndigoStep::Receive) Flow->Restart();
    }
    Player->SetIgnoreMoveInput(true);
    Player->SetIgnoreLookInput(true);
    Player->bShowMouseCursor = true;
    Player->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
    Widget = CreateWidget<UIndigoInterfaceWidget>(Player, UIndigoInterfaceWidget::StaticClass());
    if (Widget) { Widget->SetBridge(this); Widget->AddToViewport(10); Widget->Refresh(); }
}
UIndigoFlowComponent* AIndigoWorkbenchBridge::GetFlow() const { return IsValid(Director) ? Director->Flow.Get() : nullptr; }
void AIndigoWorkbenchBridge::PlayBackgroundMusic()
{
    if (BackgroundMusicAudio || (BackgroundMusicSound && BackgroundMusicPlayer)) return;
    USoundBase* MusicAsset = BackgroundMusicAsset.LoadSynchronous();
    if (!MusicAsset) MusicAsset = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_BackgroundMusic.SW_BackgroundMusic"));
    if (MusicAsset)
    {
        BackgroundMusicAudio = UGameplayStatics::CreateSound2D(this, MusicAsset, 0.f, 1.f, 0.f, nullptr, false, false);
        if (BackgroundMusicAudio)
        {
            BackgroundMusicAudio->bIsUISound = true;
            BackgroundMusicAudio->FadeIn(1.75f, BackgroundMusicVolume);
            UE_LOG(LogTemp, Display, TEXT("Indigo: imported background music started."));
            return;
        }
    }
    const FString MusicPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), BackgroundMusicFile));
    if (!FPaths::FileExists(MusicPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Indigo: background music not found at %s"), *MusicPath);
        return;
    }
    UFileMediaSource* Source = NewObject<UFileMediaSource>(this, TEXT("IndigoBackgroundMusicSource"));
    if (!Source) return;
    Source->SetFilePath(MusicPath);
    BackgroundMusicPlayer = NewObject<UMediaPlayer>(this, TEXT("IndigoBackgroundMusicPlayer"));
    BackgroundMusicPlayer->PlayOnOpen = false;
    BackgroundMusicPlayer->OnMediaOpened.AddDynamic(this, &AIndigoWorkbenchBridge::HandleMusicOpened);
    BackgroundMusicPlayer->OnMediaOpenFailed.AddDynamic(this, &AIndigoWorkbenchBridge::HandleMusicFailed);
    BackgroundMusicSound = NewObject<UMediaSoundComponent>(this, TEXT("IndigoBackgroundMusicSound"));
    BackgroundMusicSound->bIsUISound = true;
    BackgroundMusicSound->SetVolumeMultiplier(0.f);
    BackgroundMusicSound->SetMediaPlayer(BackgroundMusicPlayer);
    BackgroundMusicSound->RegisterComponent();
    MusicFadeElapsed = 0.f;
    if (!BackgroundMusicPlayer->OpenSource(Source))
    {
        UE_LOG(LogTemp, Warning, TEXT("Indigo: failed to open background music at %s"), *MusicPath);
        StopBackgroundMusic();
    }
}
void AIndigoWorkbenchBridge::HandleMusicOpened(FString Url)
{
    if (!BackgroundMusicPlayer) return;
    BackgroundMusicPlayer->SetLooping(true);
    MusicFadeElapsed = 0.f;
    if (!BackgroundMusicPlayer->Play()) StopBackgroundMusic();
}
void AIndigoWorkbenchBridge::HandleMusicFailed(FString Url)
{
    UE_LOG(LogTemp, Warning, TEXT("Indigo: background music failed to decode: %s"), *Url);
    StopBackgroundMusic();
}
void AIndigoWorkbenchBridge::StopBackgroundMusic()
{
    if (BackgroundMusicAudio)
    {
        BackgroundMusicAudio->Stop();
        BackgroundMusicAudio->DestroyComponent();
        BackgroundMusicAudio = nullptr;
    }
    if (BackgroundMusicSound)
    {
        BackgroundMusicSound->SetMediaPlayer(nullptr);
        BackgroundMusicSound->DestroyComponent();
        BackgroundMusicSound = nullptr;
    }
    if (BackgroundMusicPlayer)
    {
        BackgroundMusicPlayer->OnMediaOpened.RemoveDynamic(this, &AIndigoWorkbenchBridge::HandleMusicOpened);
        BackgroundMusicPlayer->OnMediaOpenFailed.RemoveDynamic(this, &AIndigoWorkbenchBridge::HandleMusicFailed);
        BackgroundMusicPlayer->Close();
        BackgroundMusicPlayer = nullptr;
    }
    MusicFadeElapsed = 0.f;
}
ACameraActor* AIndigoWorkbenchBridge::ResolveCamera(EIndigoCamera Camera) const
{
    if(IsValid(Director)) if(const auto* Entry=Director->Cameras.Find(Camera)) if(IsValid(Entry->Get())) return Entry->Get();
    return IsValid(WorkbenchCamera)?WorkbenchCamera.Get():nullptr;
}
bool AIndigoWorkbenchBridge::CanSwitchWorkbenchCamera(EIndigoCamera Camera) const
{
    const UIndigoFlowComponent* Flow=GetFlow();
    return bWorkbenchActive && IsValid(Player) && Flow && !Flow->IsInputLocked() && static_cast<uint8>(Camera)<=static_cast<uint8>(EIndigoCamera::Quilt) && ResolveCamera(Camera) &&
        (Camera!=EIndigoCamera::Quilt || Flow->Step==EIndigoStep::Reveal || Flow->Step==EIndigoStep::End || Flow->Step==EIndigoStep::Delivery);
}
bool AIndigoWorkbenchBridge::SwitchWorkbenchCamera(EIndigoCamera Camera)
{
    DismissOldSamplePreview();
    if(!CanSwitchWorkbenchCamera(Camera)) return false;
    ApplyCamera(Camera); return true;
}
void AIndigoWorkbenchBridge::ApplyCamera(EIndigoCamera Camera,bool bForce)
{
    ACameraActor* Target=ResolveCamera(Camera);
    if(!bWorkbenchActive || !Player || !Target || !GetFlow()) return;
    CancelDrag();
    const bool Changed=bForce || CurrentCamera.Get()!=Target;
    ActiveCamera=Camera;
    if(!Changed) return;
    CurrentCamera=Target;
    BlendRemaining=FMath::Max(0.f,CameraBlendSeconds);
    GetFlow()->SetCameraBusy(BlendRemaining>0.f);
    Player->SetViewTargetWithBlend(Target,BlendRemaining);
}
void AIndigoWorkbenchBridge::HandleStep(EIndigoStep Step)
{
    if(const UIndigoFlowComponent* Flow=GetFlow()) if(const FIndigoStepConfig* Config=Flow->StepConfig.Find(Step)) ApplyCamera(Config->Camera);
}
void AIndigoWorkbenchBridge::HandleSubtitle(FText Text)
{
    if(Text.IsEmpty() || CurrentSubtitle.EqualTo(Text)) return;
    if(SubtitleRemaining>0.f && !CurrentSubtitle.IsEmpty()) PendingSubtitle=Text;
    else { CurrentSubtitle=Text;PendingSubtitle=FText::GetEmpty();SubtitleRemaining=FMath::Max(0.f,SubtitleMinimumSeconds); }
}
bool AIndigoWorkbenchBridge::IsPlayerNear() const
{
    if(OpeningVideo && OpeningVideo->bPlaying) return false;
    const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
    return Pawn && FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionRadius);
}
void AIndigoWorkbenchBridge::StartGame()
{
    if (bGameStarted) return;
    bGameStarted = true;
    if (Player)
    {
        Player->SetIgnoreMoveInput(false);
        Player->SetIgnoreLookInput(false);
        Player->bShowMouseCursor = false;
        Player->SetInputMode(FInputModeGameOnly());
    }
    PlayBackgroundMusic();
    if (OpeningVideo && !OpeningVideo->PlayOpeningVideo())
    {
        UE_LOG(LogTemp, Display, TEXT("Indigo: opening video was unavailable; entering the game directly."));
    }
    if (Widget) Widget->Refresh();
}
void AIndigoWorkbenchBridge::QuitGame()
{
    if (Player) UKismetSystemLibrary::QuitGame(this, Player, EQuitPreference::Quit, false);
}
bool AIndigoWorkbenchBridge::SelectNextPattern(EIndigoPattern Pattern)
{
    DismissOldSamplePreview();
    UIndigoFlowComponent* Flow = GetFlow();
    if (!bGameStarted || !bWorkbenchActive || !Flow || Flow->Step != EIndigoStep::Select || Pattern == EIndigoPattern::None) return false;
    bool bLeft = false;
    if (Flow->Result.Left == EIndigoPattern::None) bLeft = true;
    else if (Flow->Result.Right == EIndigoPattern::None) bLeft = false;
    else return false;
    const bool bSelected = Flow->SetPattern(bLeft, Pattern);
    if (Widget) Widget->Refresh();
    return bSelected;
}
bool AIndigoWorkbenchBridge::EnterWorkbench()
{
    if(!bGameStarted || (OpeningVideo && OpeningVideo->bPlaying)) return false;
    if (bWorkbenchActive || !Player || !IsPlayerNear() || !GetFlow() || !IsValid(WorkbenchCamera)) return false;
    PreviousView = Player->GetViewTarget();
    bPreviousCursor = Player->bShowMouseCursor;
    Player->SetIgnoreMoveInput(true);
    Player->SetIgnoreLookInput(true);
    bOwnInputLock = true;
    bWorkbenchActive = true;
    Player->bShowMouseCursor = true;
    Player->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
    if(const FIndigoStepConfig* Config=GetFlow()->StepConfig.Find(GetFlow()->Step))
    {
        CurrentSubtitle=Config->Subtitle; PendingSubtitle=FText::GetEmpty(); SubtitleRemaining=SubtitleMinimumSeconds;
        ApplyCamera(Config->Camera,true);
    }
    else ApplyCamera(EIndigoCamera::Table,true);
    int32 Width, Height; Player->GetViewportSize(Width, Height);
    Player->SetMouseLocation(Width / 2, Height / 2);
    if (Widget) Widget->Refresh();
    return true;
}
void AIndigoWorkbenchBridge::LeaveWorkbench()
{
    DismissOldSamplePreview();
    if (!bWorkbenchActive) return;
    CancelDrag();
    bWorkbenchActive = false;
    BlendRemaining = 0.f;
    CurrentCamera.Reset();
    if (UIndigoFlowComponent* Flow = GetFlow()) Flow->SetCameraBusy(false);
    if (Player)
    {
        if (AActor* View = PreviousView.Get()) Player->SetViewTargetWithBlend(View, CameraBlendSeconds);
        else if (Player->GetPawn()) Player->SetViewTargetWithBlend(Player->GetPawn(), CameraBlendSeconds);
        if (bOwnInputLock) { Player->SetIgnoreMoveInput(false); Player->SetIgnoreLookInput(false); }
        Player->bShowMouseCursor = bPreviousCursor;
        if (bPreviousCursor) Player->SetInputMode(FInputModeGameAndUI());
        else Player->SetInputMode(FInputModeGameOnly());
    }
    bOwnInputLock = false;
    if (Widget) Widget->Refresh();
}
bool AIndigoWorkbenchBridge::TryDrop(AIndigoInteractable* Source, AIndigoInteractable* Target)
{
    DismissOldSamplePreview();
    UIndigoFlowComponent* Flow = GetFlow();
    if (!bWorkbenchActive || !IsValid(Source) || !IsValid(Target) || !Target->Accepts(Source, Flow)) return false;
    return Flow->Step == EIndigoStep::Select ? Flow->SetPattern(Target->bLeftSlot, Source->Pattern) : Flow->Perform(Target->Action, Target->ActionIndex);
}
bool AIndigoWorkbenchBridge::PerformAction(EIndigoAction Action, int32 Index)
{
    CancelDrag();
    if (Action != EIndigoAction::InspectCloth) DismissOldSamplePreview();
    UIndigoFlowComponent* Flow = GetFlow();
    const bool Accepted = bWorkbenchActive && Flow && Flow->Perform(Action, Index);
    if (Accepted && Action == EIndigoAction::InspectCloth) bOldSamplePreviewVisible = true;
    if(Accepted && Action==EIndigoAction::InspectCloth) HandleSubtitle(FText::FromString(TEXT("这块旧布角上的纹样，要为她留在中央。")));
    if(Accepted && Action==EIndigoAction::InspectOrder) HandleSubtitle(Flow->OrderFront);
    if (Widget) Widget->Refresh();
    return Accepted;
}
void AIndigoWorkbenchBridge::DismissOldSamplePreview()
{
    if (!bOldSamplePreviewVisible) return;
    bOldSamplePreviewVisible = false;
    if (Widget) Widget->Refresh();
}
void AIndigoWorkbenchBridge::RestartWorkbench()
{
    if (!bWorkbenchActive || !GetFlow()) return;
    DismissOldSamplePreview();
    CancelDrag(); CurrentSubtitle=FText::GetEmpty(); PendingSubtitle=FText::GetEmpty(); SubtitleRemaining=0.f; GetFlow()->Restart();
    // Retain the camera blend lock if restart is pressed while entering.
    GetFlow()->SetCameraBusy(BlendRemaining > 0.f);
    if (Widget) Widget->Refresh();
}
AIndigoInteractable* AIndigoWorkbenchBridge::TracePointer(bool bIgnorePressed) const
{
    FVector O,D;
    if (!Player || (Widget && Widget->IsPointerOverPanel())) return nullptr;
    const bool Projected=bEventPointer ? Player->DeprojectScreenPositionToWorld(EventPosition.X,EventPosition.Y,O,D) : Player->DeprojectMousePositionToWorld(O,D);
    if (!Projected) return nullptr;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(IndigoWorkbenchPointer), false);
    if (Player->GetPawn()) Params.AddIgnoredActor(Player->GetPawn());
    if (bIgnorePressed && IsValid(Pressed)) Params.AddIgnoredActor(Pressed);
    // Only the registered, available workbench handles participate in pointer picking.
    // Decorative boards and child meshes must not occlude their own drop targets.
    AIndigoInteractable* Best=nullptr;
    float BestDistance=TNumericLimits<float>::Max();
    for(TActorIterator<AIndigoInteractable> It(GetWorld());It;++It)
    {
        AIndigoInteractable* Candidate=*It;
        if((bIgnorePressed && Candidate==Pressed) || !Candidate->IsAvailable(GetFlow())) continue;
        if(bIgnorePressed && IsValid(Pressed) && !Candidate->Accepts(Pressed,GetFlow())) continue;
        Candidate->AlignSlotToVisual();
        FHitResult Hit;
        if(Candidate->HitBox->LineTraceComponent(Hit,O,O+D*100000.f,Params))
        {
            // Enlarged neighboring slots can overlap. Choose the nearest board center
            // in screen space instead of whichever front face the ray meets first.
            float Distance=Hit.Distance;
            if(bIgnorePressed && Candidate->Kind==EIndigoInteractionKind::DropTarget)
            {
                FVector2D Center,Pointer=EventPosition;
                if(!bEventPointer) Player->GetMousePosition(Pointer.X,Pointer.Y);
                if(Player->ProjectWorldLocationToScreen(Candidate->GetActorLocation(),Center))
                    Distance=FVector2D::DistSquared(Center,Pointer);
            }
            if(Distance<BestDistance) { Best=Candidate;BestDistance=Distance; }
        }
    }
    return Best;
}
bool AIndigoWorkbenchBridge::OnPlane(FVector& Point) const
{
    FVector O,D;
    if (!Player) return false;
    const bool Projected=bEventPointer ? Player->DeprojectScreenPositionToWorld(EventPosition.X,EventPosition.Y,O,D) : Player->DeprojectMousePositionToWorld(O,D);
    if (!Projected) return false;
    const float Dot = FVector::DotProduct(D,PlaneNormal);
    if (FMath::Abs(Dot)<KINDA_SMALL_NUMBER) return false;
    const float T = FVector::DotProduct(PlaneOrigin-O,PlaneNormal)/Dot;
    if (T<0.f) return false;
    Point=O+D*T; return true;
}
void AIndigoWorkbenchBridge::Press()
{
    const bool FromEvent=bEventPointer;
    CancelDrag();
    bEventPointer=FromEvent;
    UIndigoFlowComponent* Flow=GetFlow();
    AIndigoInteractable* Hit=TracePointer();
    if (Hit && bOldSamplePreviewVisible && Hit->Action != EIndigoAction::InspectCloth) DismissOldSamplePreview();
    if (!Hit || !Hit->IsAvailable(Flow) || Hit->Kind==EIndigoInteractionKind::DropTarget) return;
    if (bEventPointer) PressPosition=EventPosition;
    else if (!Player->GetMousePosition(PressPosition.X,PressPosition.Y)) return;
    Pressed=Hit; DragOrigin=Hit->GetActorTransform(); PlaneOrigin=DragOrigin.GetLocation();
    PlaneNormal=Player->PlayerCameraManager->GetCameraRotation().Vector();
    FVector Point; GrabOffset=OnPlane(Point)?PlaneOrigin-Point:FVector::ZeroVector;
}
void AIndigoWorkbenchBridge::Release()
{
    UIndigoFlowComponent* Flow=GetFlow();
    if (!Pressed || !Flow || Flow->IsInputLocked()) { CancelDrag(); return; }
    AIndigoInteractable* Source=Pressed;
    AIndigoInteractable* Target=TracePointer(bDragging);
    const bool WasDragging=bDragging;
    const FTransform Start=DragOrigin;
    Pressed=nullptr; bDragging=false; ClearHighlights();
    Source->SetActorTransform(Start);
    if (WasDragging)
    {
        if (!TryDrop(Source,Target))
        {
            Source->InvalidDrop();
            HandleSubtitle(FText::FromString(TEXT("还没放到合适的位置，试试亮起的区域。")));
            for(TActorIterator<AIndigoInteractable> It(GetWorld());It;++It) if(It->Accepts(Source,Flow))
            {
                if(auto* Handle=Cast<AIndigoCraftHandle>(*It)) Handle->FlashTarget();
                else
                {
                    It->SetDropHighlight(true);TWeakObjectPtr<AIndigoInteractable> Weak=*It;
                    FTimerHandle Timer;GetWorld()->GetTimerManager().SetTimer(Timer,FTimerDelegate::CreateWeakLambda(this,[Weak](){if(Weak.IsValid()) Weak->SetDropHighlight(false);}),1.f,false);
                }
            }
        }
    }
    else if (Source==Target && Source->Kind==EIndigoInteractionKind::Click) PerformAction(Source->Action, Source->ActionIndex);
    if (Widget) Widget->Refresh();
}
void AIndigoWorkbenchBridge::ClearHighlights()
{
    for (AIndigoInteractable* Target:Highlighted) if (IsValid(Target)) Target->SetDropHighlight(false);
    Highlighted.Empty();
}
void AIndigoWorkbenchBridge::CancelDrag()
{
    if (IsValid(Pressed) && bDragging) Pressed->SetActorTransform(DragOrigin);
    Pressed=nullptr; bDragging=false; ClearHighlights();
    if (IsValid(Hovered)) Hovered->SetHover(false);
    Hovered=nullptr; bEventPointer=false;
}
bool AIndigoWorkbenchBridge::PointerDown(FVector2D Position)
{
    if (!bWorkbenchActive || !GetFlow() || GetFlow()->IsInputLocked()) return false;
    EventPosition=Position; bEventPointer=true; Press();
    if (!Pressed) bEventPointer=false;
    return IsValid(Pressed);
}
void AIndigoWorkbenchBridge::PointerMove(FVector2D Position)
{
    if (!bEventPointer || !IsValid(Pressed)) return;
    EventPosition=Position; UpdateDrag(Position);
}
void AIndigoWorkbenchBridge::PointerUp(FVector2D Position)
{
    if (!bEventPointer) return;
    EventPosition=Position; UpdateDrag(Position); Release(); bEventPointer=false;
}
void AIndigoWorkbenchBridge::UpdateDrag(FVector2D Position)
{
    UIndigoFlowComponent* Flow=GetFlow();
    if (!IsValid(Pressed) || !Pressed->IsAvailable(Flow)) { CancelDrag(); return; }
    if (Pressed->Kind!=EIndigoInteractionKind::DragSource) return;
    if (!bDragging && FVector2D::Distance(PressPosition,Position)>=5.f)
    {
        bDragging=true;
        for (TActorIterator<AIndigoInteractable> It(GetWorld());It;++It)
            if (It->Accepts(Pressed,Flow)) { Highlighted.Add(*It); It->SetDropHighlight(true); }
    }
    FVector P; if (bDragging && OnPlane(P)) Pressed->SetActorLocation(P+GrabOffset);
}
void AIndigoWorkbenchBridge::Tick(float Delta)
{
    Super::Tick(Delta);
    if (!Player) return;
    if (BackgroundMusicSound && BackgroundMusicPlayer && BackgroundMusicPlayer->IsPlaying())
    {
        MusicFadeElapsed += Delta;
        const float Fade = FMath::Clamp(MusicFadeElapsed / 2.f, 0.f, 1.f);
        BackgroundMusicSound->SetVolumeMultiplier(BackgroundMusicVolume * Fade * Fade * (3.f - 2.f * Fade));
    }
    if(bWorkbenchActive)
    {
        SubtitleRemaining=FMath::Max(0.f,SubtitleRemaining-Delta);
        if(SubtitleRemaining==0.f && !PendingSubtitle.IsEmpty())
        {
            CurrentSubtitle=PendingSubtitle;PendingSubtitle=FText::GetEmpty();SubtitleRemaining=FMath::Max(0.f,SubtitleMinimumSeconds);
        }
    }
    if (BlendRemaining>0.f)
    {
        BlendRemaining=FMath::Max(0.f,BlendRemaining-Delta);
        if (BlendRemaining==0.f) if (UIndigoFlowComponent* Flow=GetFlow()) Flow->SetCameraBusy(false);
    }
    if (bGameStarted && !(OpeningVideo && OpeningVideo->bPlaying) && Player->WasInputKeyJustPressed(EKeys::E))
    {
        if (bWorkbenchActive) LeaveWorkbench(); else EnterWorkbench();
    }
    if (bWorkbenchActive)
    {
        UIndigoFlowComponent* Flow=GetFlow();
        // GameAndUI can focus a widget while the game viewport remains interactive.
        const bool HasFocus=GetWorld()->GetGameViewport() && GetWorld()->GetGameViewport()->Viewport && GetWorld()->GetGameViewport()->Viewport->IsForegroundWindow();
        if (bDebugPointer && (Player->WasInputKeyJustPressed(EKeys::LeftMouseButton) || Player->WasInputKeyJustReleased(EKeys::LeftMouseButton)))
            UE_LOG(LogTemp,Display,TEXT("Indigo pointer: foreground=%d hit=%s pressed=%s"),HasFocus,*GetNameSafe(TracePointer(bDragging)),*GetNameSafe(Pressed));
        if (!Flow || Flow->IsInputLocked() || !HasFocus) CancelDrag();
        else if (Player->WasInputKeyJustPressed(EKeys::RightMouseButton)) CancelDrag();
        else
        {
            if (!bEventPointer && Player->WasInputKeyJustPressed(EKeys::LeftMouseButton)) Press();
            if (!bEventPointer && Player->WasInputKeyJustReleased(EKeys::LeftMouseButton)) Release();
            if (IsValid(Pressed) && !bEventPointer)
            {
                float X,Y;
                if (!Player->IsInputKeyDown(EKeys::LeftMouseButton) || !Pressed->IsAvailable(Flow) || !Player->GetMousePosition(X,Y)) CancelDrag();
                else if (Pressed->Kind==EIndigoInteractionKind::DragSource)
                {
                    UpdateDrag(FVector2D(X,Y));
                }
            }
            AIndigoInteractable* Hit=TracePointer(bDragging);
            if (Hit && !Hit->IsAvailable(Flow)) Hit=nullptr;
            if (Hit!=Hovered)
            {
                if (IsValid(Hovered)) Hovered->SetHover(false);
                Hovered=Hit; if (Hovered) Hovered->SetHover(true);
            }
        }
        for (TActorIterator<AIndigoPatternSource> It(GetWorld());It;++It)
            It->PatternLabel->SetWorldRotation((Player->PlayerCameraManager->GetCameraLocation()-It->PatternLabel->GetComponentLocation()).Rotation());
    }
    if (Widget) Widget->Refresh();
}
void AIndigoWorkbenchBridge::EndPlay(const EEndPlayReason::Type Reason)
{
    LeaveWorkbench();
    StopBackgroundMusic();
    if(UIndigoFlowComponent* Flow=GetFlow())
    {
        Flow->OnStepChanged.RemoveDynamic(this,&AIndigoWorkbenchBridge::HandleStep);
        Flow->OnSubtitle.RemoveDynamic(this,&AIndigoWorkbenchBridge::HandleSubtitle);
        Flow->OnInteractionInvalidated.RemoveDynamic(this,&AIndigoWorkbenchBridge::CancelDrag);
    }
    if (Widget) Widget->RemoveFromParent();
    Super::EndPlay(Reason);
}
