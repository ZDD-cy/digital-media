#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IndigoTypes.h"
#include "IndigoPlayerController.generated.h"

class AIndigoDirector;
class AIndigoInteractable;
class ACameraActor;
class UIndigoFlowComponent;

UCLASS(Blueprintable)
class INDIGOINTERACTION_API AIndigoPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AIndigoPlayerController();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<AIndigoDirector> Director;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="0")) float CameraBlendSeconds = 0.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="1")) float DragThresholdPixels = 5.f;
    UFUNCTION(BlueprintCallable, Category="Indigo") void CancelInteraction();
    UFUNCTION(BlueprintCallable, Category="Indigo") void SwitchCamera(EIndigoCamera Camera);
    // Call from UMG OnMouseEnter/Leave for panels that overlap the 3D viewport.
    UFUNCTION(BlueprintCallable, Category="Indigo") void SetPointerBlocked(bool bBlocked);
protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY() TObjectPtr<AIndigoInteractable> Pressed;
    UPROPERTY() TObjectPtr<AIndigoInteractable> Hovered;
    UPROPERTY() TArray<TObjectPtr<AIndigoInteractable>> Highlighted;
    FTransform Origin;
    FVector2D PressPosition = FVector2D::ZeroVector;
    FVector DragPlaneOrigin = FVector::ZeroVector, DragPlaneNormal = FVector::UpVector, GrabOffset = FVector::ZeroVector;
    bool bDragging = false, bPointerBlocked = false;
    FTimerHandle CameraTimer;
    int32 CameraSerial = 0;
    TWeakObjectPtr<ACameraActor> CurrentCamera;
    UIndigoFlowComponent* GetFlow() const;
    AIndigoInteractable* TracePointer(bool bIgnorePressed = false) const;
    bool PointerOnDragPlane(FVector& OutPoint) const;
    void Press();
    void Release();
    void ClearHighlights();
    UFUNCTION() void HandleStep(EIndigoStep Step);
    UFUNCTION() void HandleInvalidation();
};
