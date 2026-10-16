#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IndigoTypes.h"
#include "IndigoWorkbenchBridge.generated.h"

class AIndigoDirector;
class AIndigoInteractable;
class ACameraActor;
class APlayerController;
class UIndigoFlowComponent;
class UIndigoWorkbenchWidget;

// Works with the project's existing first-person PlayerController and pawn.
UCLASS(Blueprintable)
class INDIGOINTERACTION_API AIndigoWorkbenchBridge : public AActor
{
    GENERATED_BODY()
public:
    AIndigoWorkbenchBridge();
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TObjectPtr<AIndigoDirector> Director;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TObjectPtr<ACameraActor> WorkbenchCamera;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="50")) float InteractionRadius = 350.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="0")) float CameraBlendSeconds = 0.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Debug") bool bDebugPointer = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bWorkbenchActive = false;
    UFUNCTION(BlueprintPure, Category="Indigo") bool IsPlayerNear() const;
    UFUNCTION(BlueprintCallable, Category="Indigo") bool EnterWorkbench();
    UFUNCTION(BlueprintCallable, Category="Indigo") void LeaveWorkbench();
    UFUNCTION(BlueprintCallable, Category="Indigo") bool TryDrop(AIndigoInteractable* Source, AIndigoInteractable* Target);
    UFUNCTION(BlueprintCallable, Category="Indigo") void CancelDrag();
    UFUNCTION(BlueprintCallable, Category="Indigo") bool PerformAction(EIndigoAction Action, int32 Index = 0);
    UFUNCTION(BlueprintCallable, Category="Indigo") void RestartWorkbench();
    UFUNCTION(BlueprintPure, Category="Indigo") UIndigoFlowComponent* GetFlow() const;
    bool PointerDown(FVector2D Position);
    void PointerMove(FVector2D Position);
    void PointerUp(FVector2D Position);
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<APlayerController> Player;
    UPROPERTY(Transient) TObjectPtr<UIndigoWorkbenchWidget> Widget;
    UPROPERTY(Transient) TObjectPtr<AIndigoInteractable> Pressed;
    UPROPERTY(Transient) TObjectPtr<AIndigoInteractable> Hovered;
    UPROPERTY(Transient) TArray<TObjectPtr<AIndigoInteractable>> Highlighted;
    TWeakObjectPtr<AActor> PreviousView;
    FTransform DragOrigin;
    FVector2D PressPosition = FVector2D::ZeroVector;
    FVector PlaneOrigin = FVector::ZeroVector, PlaneNormal = FVector::UpVector, GrabOffset = FVector::ZeroVector;
    bool bDragging = false, bPreviousCursor = false, bOwnInputLock = false;
    bool bEventPointer = false;
    FVector2D EventPosition = FVector2D::ZeroVector;
    float BlendRemaining = 0.f;
    AIndigoInteractable* TracePointer(bool bIgnorePressed = false) const;
    bool OnPlane(FVector& Point) const;
    void Press();
    void Release();
    void ClearHighlights();
    void UpdateDrag(FVector2D Position);
};
