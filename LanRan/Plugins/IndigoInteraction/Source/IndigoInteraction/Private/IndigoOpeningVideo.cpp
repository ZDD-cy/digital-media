#include "IndigoOpeningVideo.h"
#include "IndigoWorkbenchBridge.h"
#include "IndigoWorkbenchWidget.h"
#include "MediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "MediaSoundComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"

UIndigoOpeningVideoComponent::UIndigoOpeningVideoComponent() { PrimaryComponentTick.bCanEverTick=false; }
void UIndigoOpeningVideoComponent::BeginPlay()
{
    Super::BeginPlay();
    if(bPlayOnStart && VideoSource) PlayOpeningVideo();
}
bool UIndigoOpeningVideoComponent::PlayOpeningVideo()
{
    if(bPlaying || !VideoSource || !GetWorld()) return false;
    if(const auto* Bridge=Cast<AIndigoWorkbenchBridge>(GetOwner())) if(Bridge->bWorkbenchActive) return false;
    Player=UGameplayStatics::GetPlayerController(this,0);
    if(!Player || !Player->IsLocalController()) return false;
    bPlaying=true;bPreviousCursor=Player->bShowMouseCursor;
    Player->SetIgnoreMoveInput(true);Player->SetIgnoreLookInput(true);bOwnInputLock=true;
    Player->bShowMouseCursor=true;
    Player->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
    MediaPlayer=NewObject<UMediaPlayer>(this);MediaPlayer->PlayOnOpen=false;
    MediaPlayer->OnMediaOpened.AddDynamic(this,&UIndigoOpeningVideoComponent::MediaOpened);
    MediaPlayer->OnMediaOpenFailed.AddDynamic(this,&UIndigoOpeningVideoComponent::MediaFailed);
    MediaPlayer->OnEndReached.AddDynamic(this,&UIndigoOpeningVideoComponent::MediaEnded);
    VideoTexture=NewObject<UMediaTexture>(this);VideoTexture->AutoClear=true;
    VideoTexture->SetMediaPlayer(MediaPlayer);VideoTexture->UpdateResource();
    Sound=NewObject<UMediaSoundComponent>(GetOwner());Sound->bIsUISound=true;
    Sound->RegisterComponent();Sound->SetMediaPlayer(MediaPlayer);
    Screen=CreateWidget<UIndigoOpeningVideoWidget>(Player,UIndigoOpeningVideoWidget::StaticClass());
    if(!Screen) { Finish(EIndigoOpeningResult::Failed);return false; }
    Screen->SetVideo(this);Screen->AddToViewport(1000);
    GetWorld()->GetTimerManager().SetTimer(OpenTimer,this,&UIndigoOpeningVideoComponent::OpenTimedOut,FMath::Max(1.f,OpenTimeoutSeconds),false);
    if(!MediaPlayer->OpenSource(VideoSource)) { Finish(EIndigoOpeningResult::Failed);return false; }
    return bPlaying;
}
void UIndigoOpeningVideoComponent::MediaOpened(FString Url)
{
    if(!bPlaying) return;
    GetWorld()->GetTimerManager().ClearTimer(OpenTimer);
    MediaPlayer->SetLooping(false);
    if(!MediaPlayer->Play()) Finish(EIndigoOpeningResult::Failed);
}
void UIndigoOpeningVideoComponent::MediaFailed(FString Url) { Finish(EIndigoOpeningResult::Failed); }
void UIndigoOpeningVideoComponent::MediaEnded() { Finish(EIndigoOpeningResult::Completed); }
void UIndigoOpeningVideoComponent::OpenTimedOut() { Finish(EIndigoOpeningResult::Failed); }
void UIndigoOpeningVideoComponent::SkipOpeningVideo() { Finish(EIndigoOpeningResult::Skipped); }
void UIndigoOpeningVideoComponent::Finish(EIndigoOpeningResult Result,bool bNotify)
{
    if(!bPlaying) return;
    bPlaying=false;
    if(GetWorld()) GetWorld()->GetTimerManager().ClearTimer(OpenTimer);
    if(MediaPlayer)
    {
        MediaPlayer->OnMediaOpened.RemoveDynamic(this,&UIndigoOpeningVideoComponent::MediaOpened);
        MediaPlayer->OnMediaOpenFailed.RemoveDynamic(this,&UIndigoOpeningVideoComponent::MediaFailed);
        MediaPlayer->OnEndReached.RemoveDynamic(this,&UIndigoOpeningVideoComponent::MediaEnded);
        MediaPlayer->Close();
    }
    if(Sound) { Sound->SetMediaPlayer(nullptr);Sound->DestroyComponent();Sound=nullptr; }
    if(Screen) { Screen->RemoveFromParent();Screen=nullptr; }
    if(IsValid(Player))
    {
        if(bOwnInputLock) { Player->SetIgnoreMoveInput(false);Player->SetIgnoreLookInput(false); }
        Player->bShowMouseCursor=bPreviousCursor;
        if(bPreviousCursor) Player->SetInputMode(FInputModeGameAndUI());else Player->SetInputMode(FInputModeGameOnly());
    }
    bOwnInputLock=false;VideoTexture=nullptr;MediaPlayer=nullptr;
    if(bNotify) OnOpeningFinished.Broadcast(Result);
}
void UIndigoOpeningVideoComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Finish(EIndigoOpeningResult::Failed,false);Super::EndPlay(Reason);
}
void UIndigoOpeningVideoWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UCanvasPanel* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    auto Fill=[Root](UWidget* Child)
    {
        auto* CanvasSlot=Root->AddChildToCanvas(Child);CanvasSlot->SetAnchors(FAnchors(0,0,1,1));CanvasSlot->SetOffsets(FMargin(0));
    };
    UBorder* Black=WidgetTree->ConstructWidget<UBorder>();Black->SetBrushColor(FLinearColor::Black);Fill(Black);
    UScaleBox* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);Fill(Scale);
    Picture=WidgetTree->ConstructWidget<UImage>();Scale->SetContent(Picture);
    UButton* Button=WidgetTree->ConstructWidget<UIndigoWorkbenchButton>();
    UTextBlock* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(TEXT("跳过片头")));Button->SetContent(Text);
    auto* CanvasSlot=Root->AddChildToCanvas(Button);CanvasSlot->SetAnchors(FAnchors(1,1));CanvasSlot->SetPosition(FVector2D(-200,-70));CanvasSlot->SetSize(FVector2D(170,44));
    Button->OnClicked.AddDynamic(this,&UIndigoOpeningVideoWidget::Skip);
}
void UIndigoOpeningVideoWidget::SetVideo(UIndigoOpeningVideoComponent* Component)
{
    Video=Component;
    if(Picture && Video) { Picture->SetBrushResourceObject(Video->GetVideoTexture());Picture->SetBrushSize(FVector2D(1920,1080)); }
}
void UIndigoOpeningVideoWidget::NativeTick(const FGeometry& Geometry,float Delta)
{
    Super::NativeTick(Geometry,Delta);
    if(Video && Video->GetVideoTexture())
    {
        const auto* Texture=Video->GetVideoTexture();const FVector2D Size(Texture->GetSurfaceWidth(),Texture->GetSurfaceHeight());
        if(Size.X>0 && Size.Y>0) Picture->SetBrushSize(Size);
    }
}
void UIndigoOpeningVideoWidget::Skip() { if(Video) Video->SkipOpeningVideo(); }

