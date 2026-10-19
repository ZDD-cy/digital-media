#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "IndigoOpeningVideo.generated.h"

class UMediaSource;
class UMediaPlayer;
class UMediaTexture;
class UMediaSoundComponent;
class UImage;
class APlayerController;
class UIndigoOpeningVideoWidget;

UENUM(BlueprintType)
enum class EIndigoOpeningResult : uint8 { Completed, Skipped, Failed };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIndigoOpeningFinished,EIndigoOpeningResult,Result);

// Optional movie before first-person play. An empty source leaves normal startup unchanged.
UCLASS(ClassGroup=(Indigo),BlueprintType,meta=(BlueprintSpawnableComponent))
class INDIGOINTERACTION_API UIndigoOpeningVideoComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UIndigoOpeningVideoComponent();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Indigo|Opening Video") TObjectPtr<UMediaSource> VideoSource;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Indigo|Opening Video") bool bPlayOnStart=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Indigo|Opening Video",meta=(ClampMin="1")) float OpenTimeoutSeconds=15.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Indigo|Opening Video") bool bPlaying=false;
    UPROPERTY(BlueprintAssignable,Category="Indigo|Opening Video") FIndigoOpeningFinished OnOpeningFinished;
    UFUNCTION(BlueprintCallable,Category="Indigo|Opening Video") bool PlayOpeningVideo();
    UFUNCTION(BlueprintCallable,Category="Indigo|Opening Video") void SkipOpeningVideo();
    UFUNCTION(BlueprintPure,Category="Indigo|Opening Video") UMediaTexture* GetVideoTexture() const { return VideoTexture; }
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<UMediaPlayer> MediaPlayer;
    UPROPERTY(Transient) TObjectPtr<UMediaTexture> VideoTexture;
    UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> Sound;
    UPROPERTY(Transient) TObjectPtr<UIndigoOpeningVideoWidget> Screen;
    UPROPERTY(Transient) TObjectPtr<APlayerController> Player;
    bool bOwnInputLock=false,bPreviousCursor=false;
    FTimerHandle OpenTimer;
    UFUNCTION() void MediaOpened(FString Url);
    UFUNCTION() void MediaFailed(FString Url);
    UFUNCTION() void MediaEnded();
    void OpenTimedOut();
    void Finish(EIndigoOpeningResult Result,bool bNotify=true);
};

UCLASS()
class INDIGOINTERACTION_API UIndigoOpeningVideoWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetVideo(UIndigoOpeningVideoComponent* Component);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
private:
    UPROPERTY(Transient) TObjectPtr<UIndigoOpeningVideoComponent> Video;
    UPROPERTY(Transient) TObjectPtr<UImage> Picture;
    UFUNCTION() void Skip();
};
