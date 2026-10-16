#include "IndigoWorkbenchBridge.h"
#include "IndigoWorkbenchWidget.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "IndigoInteractable.h"
#include "IndigoPatternSource.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"

AIndigoWorkbenchBridge::AIndigoWorkbenchBridge()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    PrimaryActorTick.bCanEverTick = true;
}
void AIndigoWorkbenchBridge::BeginPlay()
{
    Super::BeginPlay();
    Player = UGameplayStatics::GetPlayerController(this, 0);
    if (!Player || !Player->IsLocalController()) { SetActorTickEnabled(false); return; }
    if (!Director) for (TActorIterator<AIndigoDirector> It(GetWorld()); It; ++It) { Director = *It; break; }
    Widget = CreateWidget<UIndigoWorkbenchWidget>(Player, UIndigoWorkbenchWidget::StaticClass());
    if (Widget) { Widget->SetBridge(this); Widget->AddToViewport(10); Widget->Refresh(); }
}
UIndigoFlowComponent* AIndigoWorkbenchBridge::GetFlow() const { return IsValid(Director) ? Director->Flow.Get() : nullptr; }
bool AIndigoWorkbenchBridge::IsPlayerNear() const
{
    const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
    return Pawn && FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionRadius);
}
bool AIndigoWorkbenchBridge::EnterWorkbench()
{
    if (bWorkbenchActive || !Player || !IsPlayerNear() || !GetFlow() || !IsValid(WorkbenchCamera)) return false;
    PreviousView = Player->GetViewTarget();
    bPreviousCursor = Player->bShowMouseCursor;
    Player->SetIgnoreMoveInput(true);
    Player->SetIgnoreLookInput(true);
    bOwnInputLock = true;
    bWorkbenchActive = true;
    Player->bShowMouseCursor = true;
    Player->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
    Player->SetViewTargetWithBlend(WorkbenchCamera, CameraBlendSeconds);
    int32 Width, Height; Player->GetViewportSize(Width, Height);
    Player->SetMouseLocation(Width / 2, Height / 2);
    BlendRemaining = FMath::Max(0.f, CameraBlendSeconds);
    GetFlow()->SetCameraBusy(BlendRemaining > 0.f);
    if (Widget) Widget->Refresh();
    return true;
}
void AIndigoWorkbenchBridge::LeaveWorkbench()
{
    if (!bWorkbenchActive) return;
    CancelDrag();
    bWorkbenchActive = false;
    BlendRemaining = 0.f;
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
    UIndigoFlowComponent* Flow = GetFlow();
    if (!bWorkbenchActive || !IsValid(Source) || !IsValid(Target) || !Target->Accepts(Source, Flow)) return false;
    return Flow->Step == EIndigoStep::Select ? Flow->SetPattern(Target->bLeftSlot, Source->Pattern) : Flow->Perform(Target->Action, Target->ActionIndex);
}
bool AIndigoWorkbenchBridge::PerformAction(EIndigoAction Action, int32 Index)
{
    CancelDrag();
    UIndigoFlowComponent* Flow = GetFlow();
    const bool Accepted = bWorkbenchActive && Flow && Flow->Perform(Action, Index);
    if (Widget) Widget->Refresh();
    return Accepted;
}
void AIndigoWorkbenchBridge::RestartWorkbench()
{
    if (!bWorkbenchActive || !GetFlow()) return;
    CancelDrag(); GetFlow()->Restart();
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
    FHitResult Hit;
    GetWorld()->LineTraceSingleByChannel(Hit,O,O+D*100000.f,ECC_Visibility,Params);
    return Cast<AIndigoInteractable>(Hit.GetActor());
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
        if (!TryDrop(Source,Target)) Source->InvalidDrop();
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
    if (BlendRemaining>0.f)
    {
        BlendRemaining=FMath::Max(0.f,BlendRemaining-Delta);
        if (BlendRemaining==0.f) if (UIndigoFlowComponent* Flow=GetFlow()) Flow->SetCameraBusy(false);
    }
    if (Player->WasInputKeyJustPressed(EKeys::E))
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
    if (Widget) Widget->RemoveFromParent();
    Super::EndPlay(Reason);
}
