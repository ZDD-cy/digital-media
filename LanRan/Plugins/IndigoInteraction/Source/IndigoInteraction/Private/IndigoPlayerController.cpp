#include "IndigoPlayerController.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "IndigoInteractable.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

AIndigoPlayerController::AIndigoPlayerController()
{
    bShowMouseCursor = true;
    bAutoManageActiveCameraTarget = false;
}

void AIndigoPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
    for (TActorIterator<AIndigoDirector> It(GetWorld()); It; ++It) { Director = *It; break; }
    SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
    if (UIndigoFlowComponent* F = GetFlow())
    {
        F->OnStepChanged.AddDynamic(this, &AIndigoPlayerController::HandleStep);
        F->OnInteractionInvalidated.AddDynamic(this, &AIndigoPlayerController::HandleInvalidation);
        HandleStep(F->Step); F->RefreshVisuals();
    }
    else UE_LOG(LogTemp, Error, TEXT("Indigo: place one IndigoDirector in the level."));
}

void AIndigoPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AIndigoPlayerController::Press);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AIndigoPlayerController::Release);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AIndigoPlayerController::CancelInteraction);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AIndigoPlayerController::CancelInteraction);
}

UIndigoFlowComponent* AIndigoPlayerController::GetFlow() const { return IsValid(Director) ? Director->Flow.Get() : nullptr; }

AIndigoInteractable* AIndigoPlayerController::TracePointer(bool bIgnorePressed) const
{
    FVector O, D;
    if (bPointerBlocked || !DeprojectMousePositionToWorld(O, D)) return nullptr;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(IndigoPointer), false);
    if (bIgnorePressed && IsValid(Pressed)) Params.AddIgnoredActor(Pressed);
    FHitResult Hit;
    GetWorld()->LineTraceSingleByChannel(Hit, O, O + D * 100000.f, ECC_Visibility, Params);
    return Cast<AIndigoInteractable>(Hit.GetActor());
}

bool AIndigoPlayerController::PointerOnDragPlane(FVector& P) const
{
    FVector O, D;
    if (!DeprojectMousePositionToWorld(O, D)) return false;
    const float Denom = FVector::DotProduct(D, DragPlaneNormal);
    if (FMath::Abs(Denom) < KINDA_SMALL_NUMBER) return false;
    const float T = FVector::DotProduct(DragPlaneOrigin - O, DragPlaneNormal) / Denom;
    if (T < 0.f) return false;
    P = O + D * T; return true;
}

void AIndigoPlayerController::Press()
{
    CancelInteraction();
    UIndigoFlowComponent* F = GetFlow();
    AIndigoInteractable* Hit = TracePointer();
    if (!IsValid(Hit) || !Hit->IsAvailable(F) || Hit->Kind == EIndigoInteractionKind::DropTarget) return;
    if (!GetMousePosition(PressPosition.X, PressPosition.Y)) return;
    Pressed = Hit; Origin = Hit->GetActorTransform();
    DragPlaneOrigin = Origin.GetLocation();
    DragPlaneNormal = PlayerCameraManager->GetCameraRotation().Vector();
    FVector Point;
    GrabOffset = PointerOnDragPlane(Point) ? Origin.GetLocation() - Point : FVector::ZeroVector;
}

void AIndigoPlayerController::PlayerTick(float Delta)
{
    Super::PlayerTick(Delta);
    UIndigoFlowComponent* F = GetFlow();
    if (!F || F->IsInputLocked() || bPointerBlocked) { CancelInteraction(); return; }
    float X, Y;
    if (!GetMousePosition(X, Y)) { CancelInteraction(); return; }
    // Lost focus or UI consumed release: cancel; do not synthesize a successful drop.
    if (IsValid(Pressed) && !IsInputKeyDown(EKeys::LeftMouseButton)) { CancelInteraction(); return; }
    if (IsValid(Pressed) && Pressed->Kind == EIndigoInteractionKind::DragSource)
    {
        if (!Pressed->IsAvailable(F)) { CancelInteraction(); return; }
        if (!bDragging && FVector2D::Distance(PressPosition, FVector2D(X, Y)) >= DragThresholdPixels)
        {
            bDragging = true;
            for (TActorIterator<AIndigoInteractable> It(GetWorld()); It; ++It)
                if (It->Accepts(Pressed, F)) { Highlighted.Add(*It); It->SetDropHighlight(true); }
        }
        FVector P;
        if (bDragging && PointerOnDragPlane(P)) Pressed->SetActorLocation(P + GrabOffset);
    }
    AIndigoInteractable* Hit = TracePointer(bDragging);
    if (IsValid(Hit) && !Hit->IsAvailable(F)) Hit = nullptr;
    if (Hit != Hovered)
    {
        if (IsValid(Hovered)) Hovered->SetHover(false);
        Hovered = Hit;
        if (IsValid(Hovered)) Hovered->SetHover(true);
    }
}

void AIndigoPlayerController::Release()
{
    UIndigoFlowComponent* F = GetFlow();
    if (!IsValid(Pressed) || !F || F->IsInputLocked() || bPointerBlocked) { CancelInteraction(); return; }
    AIndigoInteractable* Source = Pressed;
    AIndigoInteractable* Target = TracePointer(bDragging);
    const bool bWasDragging = bDragging;
    const FTransform Start = Origin;
    const bool bValidDrop = bWasDragging && IsValid(Target) && Target->Accepts(Source, F);
    // Clear before dispatch: the state or animation may invalidate interactions synchronously.
    Pressed = nullptr; bDragging = false; ClearHighlights();
    Source->SetActorTransform(Start);
    if (bValidDrop)
    {
        // Move before the callback so visual animation can own the transform afterward.
        if (!Source->bReturnToSource) Source->SetActorTransform(Target->GetActorTransform());
        const bool bAccepted = F->Step == EIndigoStep::Select ? F->SetPattern(Target->bLeftSlot, Source->Pattern) : F->Perform(Target->Action, Target->ActionIndex);
        if (!bAccepted) { Source->SetActorTransform(Start); Source->InvalidDrop(); }
    }
    else if (!bWasDragging && Source == Target && Source->Kind == EIndigoInteractionKind::Click)
        F->Perform(Source->Action, Source->ActionIndex);
    else if (bWasDragging) Source->InvalidDrop();
}

void AIndigoPlayerController::ClearHighlights()
{
    for (AIndigoInteractable* Target : Highlighted) if (IsValid(Target)) Target->SetDropHighlight(false);
    Highlighted.Empty();
}

void AIndigoPlayerController::CancelInteraction()
{
    if (IsValid(Pressed) && bDragging) Pressed->SetActorTransform(Origin);
    Pressed = nullptr; bDragging = false; ClearHighlights();
    if (IsValid(Hovered)) Hovered->SetHover(false);
    Hovered = nullptr;
}

void AIndigoPlayerController::SetPointerBlocked(bool bBlocked)
{
    bPointerBlocked = bBlocked;
    if (bBlocked) CancelInteraction();
}

void AIndigoPlayerController::HandleInvalidation() { CancelInteraction(); }
void AIndigoPlayerController::HandleStep(EIndigoStep Step)
{
    if (UIndigoFlowComponent* F = GetFlow())
        if (const FIndigoStepConfig* C = F->StepConfig.Find(Step)) SwitchCamera(C->Camera);
}

void AIndigoPlayerController::SwitchCamera(EIndigoCamera Camera)
{
    UIndigoFlowComponent* F = GetFlow();
    if (!F) return;
    const auto* Entry = Director->Cameras.Find(Camera);
    if (!Entry || !IsValid(Entry->Get()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Indigo: camera slot %d is not assigned."), static_cast<int32>(Camera)); return;
    }
    CancelInteraction();
    GetWorld()->GetTimerManager().ClearTimer(CameraTimer);
    const int32 Token = ++CameraSerial;
    const float Duration = CurrentCamera.Get() == Entry->Get() ? 0.f : FMath::Max(0.f, CameraBlendSeconds);
    CurrentCamera = Entry->Get();
    F->SetCameraBusy(Duration > 0.f);
    SetViewTargetWithBlend(Entry->Get(), Duration);
    if (Duration > 0.f)
        GetWorld()->GetTimerManager().SetTimer(CameraTimer, FTimerDelegate::CreateWeakLambda(this, [this, Token]()
        {
            if (CameraSerial == Token) if (UIndigoFlowComponent* Flow = GetFlow()) Flow->SetCameraBusy(false);
        }), Duration, false);
}

void AIndigoPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    CancelInteraction(); ++CameraSerial;
    GetWorld()->GetTimerManager().ClearTimer(CameraTimer);
    if (UIndigoFlowComponent* F = GetFlow())
    {
        F->OnStepChanged.RemoveDynamic(this, &AIndigoPlayerController::HandleStep);
        F->OnInteractionInvalidated.RemoveDynamic(this, &AIndigoPlayerController::HandleInvalidation);
    }
    Super::EndPlay(Reason);
}
