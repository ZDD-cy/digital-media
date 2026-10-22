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
class UIndigoInterfaceWidget;
class UIndigoOpeningVideoComponent;
class UAudioComponent;
class UMediaPlayer;
class UMediaSoundComponent;
class USoundBase;

// Works with the project's existing first-person PlayerController and pawn.
UCLASS(Blueprintable)
class INDIGOINTERACTION_API AIndigoWorkbenchBridge : public AActor
{
    GENERATED_BODY()
public:
    AIndigoWorkbenchBridge();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Indigo|Opening Video") TObjectPtr<UIndigoOpeningVideoComponent> OpeningVideo;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Indigo|Audio") FString OpeningVideoFile = TEXT("Movies/Opening.mp4");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Indigo|Audio") FString BackgroundMusicFile = TEXT("Movies/BGM.mp3");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Indigo|Audio") TSoftObjectPtr<USoundBase> BackgroundMusicAsset;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Indigo|Audio",meta=(ClampMin="0",ClampMax="1")) float BackgroundMusicVolume = 0.55f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Indigo") bool bGameStarted = false;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TObjectPtr<AIndigoDirector> Director;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TObjectPtr<ACameraActor> WorkbenchCamera;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="50")) float InteractionRadius = 350.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="0")) float CameraBlendSeconds = 0.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Debug") bool bDebugPointer = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bWorkbenchActive = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|Old Sample") bool bOldSamplePreviewVisible = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|Camera") EIndigoCamera ActiveCamera = EIndigoCamera::Table;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|Subtitle") FText CurrentSubtitle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Subtitle", meta=(ClampMin="0")) float SubtitleMinimumSeconds = 2.5f;
    UFUNCTION(BlueprintPure, Category="Indigo") bool IsPlayerNear() const;
    UFUNCTION(BlueprintCallable, Category="Indigo") void StartGame();
    UFUNCTION(BlueprintCallable, Category="Indigo") void QuitGame();
    UFUNCTION(BlueprintCallable, Category="Indigo") bool SelectNextPattern(EIndigoPattern Pattern);
    UFUNCTION(BlueprintCallable, Category="Indigo") bool EnterWorkbench();
    UFUNCTION(BlueprintCallable, Category="Indigo") void LeaveWorkbench();
    UFUNCTION(BlueprintCallable, Category="Indigo") bool TryDrop(AIndigoInteractable* Source, AIndigoInteractable* Target);
    UFUNCTION(BlueprintCallable, Category="Indigo") void CancelDrag();
    UFUNCTION(BlueprintCallable, Category="Indigo") bool PerformAction(EIndigoAction Action, int32 Index = 0);
    UFUNCTION(BlueprintCallable, Category="Indigo") void RestartWorkbench();
    UFUNCTION(BlueprintCallable, Category="Indigo|Old Sample") void DismissOldSamplePreview();
    UFUNCTION(BlueprintPure, Category="Indigo") UIndigoFlowComponent* GetFlow() const;
    UFUNCTION(BlueprintPure, Category="Indigo|Camera") bool CanSwitchWorkbenchCamera(EIndigoCamera Camera) const;
    UFUNCTION(BlueprintCallable, Category="Indigo|Camera") bool SwitchWorkbenchCamera(EIndigoCamera Camera);
    UFUNCTION(BlueprintCallable, Category="Indigo|Pointer") bool PointerDown(FVector2D Position);
    UFUNCTION(BlueprintCallable, Category="Indigo|Pointer") void PointerMove(FVector2D Position);
    UFUNCTION(BlueprintCallable, Category="Indigo|Pointer") void PointerUp(FVector2D Position);
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<APlayerController> Player;
    UPROPERTY(Transient) TObjectPtr<UIndigoInterfaceWidget> Widget;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> BackgroundMusicAudio;
    UPROPERTY(Transient) TObjectPtr<UMediaPlayer> BackgroundMusicPlayer;
    UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> BackgroundMusicSound;
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
    float SubtitleRemaining = 0.f;
    float MusicFadeElapsed = 0.f;
    FText PendingSubtitle;
    TWeakObjectPtr<ACameraActor> CurrentCamera;
    ACameraActor* ResolveCamera(EIndigoCamera Camera) const;
    void ApplyCamera(EIndigoCamera Camera, bool bForce = false);
    UFUNCTION() void HandleStep(EIndigoStep Step);
    UFUNCTION() void HandleSubtitle(FText Text);
    UFUNCTION() void HandleMusicOpened(FString Url);
    UFUNCTION() void HandleMusicFailed(FString Url);
    void PlayBackgroundMusic();
    void StopBackgroundMusic();
    AIndigoInteractable* TracePointer(bool bIgnorePressed = false) const;
    bool OnPlane(FVector& Point) const;
    void Press();
    void Release();
    void ClearHighlights();
    void UpdateDrag(FVector2D Position);
};
