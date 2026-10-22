#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IndigoTypes.h"
#include "IndigoInterfaceWidget.generated.h"

class AIndigoWorkbenchBridge;
class UBorder;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UVerticalBox;
class UWidget;

UCLASS()
class INDIGOINTERACTION_API UIndigoInterfaceWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetBridge(AIndigoWorkbenchBridge* InBridge);
    void Refresh();
    bool IsPointerOverPanel() const;
    bool IsStartScreenVisible() const { return bStartScreenVisible; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& InMouseEvent) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<AIndigoWorkbenchBridge> Bridge;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> Root;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> StartScreen;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> GameScreen;

    UPROPERTY(Transient)
    TObjectPtr<UWidget> GameChrome;

    UPROPERTY(Transient)
    TObjectPtr<UWidget> WorkbenchLayer;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StartLogo;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StartSubtitle;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> StartButtons;

    UPROPERTY(Transient)
    TObjectPtr<UButton> StartGameButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> QuitGameButton;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> TitleCard;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StepCounter;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> ProgressRow;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> ProgressLabels;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UBorder>> ProgressCircles;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> NavLabels;

    UPROPERTY(Transient)
    TObjectPtr<UWidget> PatternPanel;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> PatternRow;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PatternAButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PatternBButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PatternCButton;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> SelectionSummary;

    UPROPERTY(Transient)
    TObjectPtr<UImage> LeftPatternImage;

    UPROPERTY(Transient)
    TObjectPtr<UImage> RightPatternImage;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LeftPatternLabel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RightPatternLabel;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> TemperatureCard;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TemperatureValue;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TemperatureDescription;

    UPROPERTY(Transient)
    TObjectPtr<UButton> HeatButton;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ActionPanel;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ActionList;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HintText;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> SubtitlePanel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SubtitleText;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> NarrativeCard;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NarrativeText;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ResultCard;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResultEyebrow;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResultTitle;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ClothPreview;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> ClothPreviewContent;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResultQuote;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ClothButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> OrderButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> SwapButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ClearLeftButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ClearRightButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ConfirmButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> FoldButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PlaceButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ClosePlateButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ClampLeftButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ClampRightButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> DyeButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> FinishDyeButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ScoopButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> DipButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> LiftButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> OxidizeButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> WashButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> DrainButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> UnclampLeftButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> UnclampRightButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> UnfoldButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> DeliverButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ReviewButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BackButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ShipButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> RestartButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> QuitFromResultButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ExitWorkbenchButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> CameraTableButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> CameraCloseupButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> CameraQuiltButton;

    bool bStartScreenVisible = true;
    float StartAnimationTime = 0.f;
    float PageAnimationTime = 0.f;
    EIndigoStep LastStep = EIndigoStep::Receive;
    EIndigoPattern LastLeft = EIndigoPattern::None;
    EIndigoPattern LastRight = EIndigoPattern::None;
    int32 LastDepth = -1;

    UButton* AddActionButton(UVerticalBox* Parent, const FString& Index, const FString& Label);
    UButton* AddStartButton(UVerticalBox* Parent, const FString& Index, const FString& Label, const FString& Glyph);
    UImage* AddPatternCard(UHorizontalBox* Parent, const FString& Card, const FString& AssetName, TObjectPtr<UButton>& OutButton);
    void BuildStartScreen();
    void BuildGameScreen();
    void BuildSidebar(UCanvasPanel* Canvas);
    void BuildHeader(UCanvasPanel* Canvas);
    void BuildPatternPanel(UCanvasPanel* Canvas);
    void BuildTemperatureCard(UCanvasPanel* Canvas);
    void BuildActionPanel(UCanvasPanel* Canvas);
    void BuildResultCard(UCanvasPanel* Canvas);
    void UpdateActions();
    void UpdateClothPreview();
    void UpdateAnimatedElements(float DeltaTime);
    void SetActionVisible(UButton* Button, bool bVisible, bool bEnabled = true);
    void SetGameStarted(bool bStarted);

    UFUNCTION() void OnStartGame();
    UFUNCTION() void OnQuitGame();
    UFUNCTION() void OnCloth();
    UFUNCTION() void OnOrder();
    UFUNCTION() void OnPatternA();
    UFUNCTION() void OnPatternB();
    UFUNCTION() void OnPatternC();
    UFUNCTION() void OnSwap();
    UFUNCTION() void OnClearLeft();
    UFUNCTION() void OnClearRight();
    UFUNCTION() void OnConfirm();
    UFUNCTION() void OnFold();
    UFUNCTION() void OnPlace();
    UFUNCTION() void OnClosePlate();
    UFUNCTION() void OnClampLeft();
    UFUNCTION() void OnClampRight();
    UFUNCTION() void OnDye();
    UFUNCTION() void OnFinishDye();
    UFUNCTION() void OnAddFuel();
    UFUNCTION() void OnScoop();
    UFUNCTION() void OnDip();
    UFUNCTION() void OnLift();
    UFUNCTION() void OnOxidize();
    UFUNCTION() void OnWash();
    UFUNCTION() void OnDrain();
    UFUNCTION() void OnUnclampLeft();
    UFUNCTION() void OnUnclampRight();
    UFUNCTION() void OnUnfold();
    UFUNCTION() void OnDeliver();
    UFUNCTION() void OnReview();
    UFUNCTION() void OnBack();
    UFUNCTION() void OnShip();
    UFUNCTION() void OnRestart();
    UFUNCTION() void OnExitWorkbench();
    UFUNCTION() void OnCameraTable();
    UFUNCTION() void OnCameraCloseup();
    UFUNCTION() void OnCameraQuilt();
};
