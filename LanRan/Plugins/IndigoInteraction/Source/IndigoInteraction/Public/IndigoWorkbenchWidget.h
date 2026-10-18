#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "IndigoWorkbenchWidget.generated.h"

class AIndigoWorkbenchBridge;
class UBorder;
class UButton;
class UTextBlock;
class UHorizontalBox;

UCLASS()
class UIndigoWorkbenchButton : public UButton
{
    GENERATED_BODY()
public:
    UIndigoWorkbenchButton(const FObjectInitializer& Init) : Super(Init) { InitIsFocusable(false); }
};

UCLASS()
class INDIGOINTERACTION_API UIndigoWorkbenchWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetBridge(AIndigoWorkbenchBridge* InBridge) { Bridge = InBridge; }
    void Refresh();
    bool IsPointerOverPanel() const;
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<AIndigoWorkbenchBridge> Bridge;
    UPROPERTY(Transient) TObjectPtr<UBorder> Panel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Hint;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Help;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Narrative;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> TrialRow;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> FuelRow;
    UPROPERTY(Transient) TObjectPtr<UButton> ScoopTest;
    UPROPERTY(Transient) TObjectPtr<UButton> DipTest;
    UPROPERTY(Transient) TObjectPtr<UButton> LiftTest;
    UPROPERTY(Transient) TObjectPtr<UButton> OxidizeTest;
    UPROPERTY(Transient) TObjectPtr<UButton> AddFuel;
    UPROPERTY(Transient) TObjectPtr<UButton> Ship;
    UPROPERTY(Transient) TObjectPtr<UBorder> SubtitlePanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Subtitle;
    UPROPERTY(Transient) TObjectPtr<UButton> TableCamera;
    UPROPERTY(Transient) TObjectPtr<UButton> CloseupCamera;
    UPROPERTY(Transient) TObjectPtr<UButton> QuiltCamera;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> SelectionRow1;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> SelectionRow2;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> CraftRow1;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> CraftRow2;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> DyeRow;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> WashRow1;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> WashRow2;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> ResultRow;
    UPROPERTY(Transient) TObjectPtr<UButton> Wash;
    UPROPERTY(Transient) TObjectPtr<UButton> Drain;
    UPROPERTY(Transient) TObjectPtr<UButton> UnclampLeft;
    UPROPERTY(Transient) TObjectPtr<UButton> UnclampRight;
    UPROPERTY(Transient) TObjectPtr<UButton> Unfold;
    UPROPERTY(Transient) TObjectPtr<UButton> Deliver;
    UPROPERTY(Transient) TObjectPtr<UButton> Review;
    UPROPERTY(Transient) TObjectPtr<UButton> Back;
    UPROPERTY(Transient) TObjectPtr<UButton> Cloth;
    UPROPERTY(Transient) TObjectPtr<UButton> Order;
    UPROPERTY(Transient) TObjectPtr<UButton> Swap;
    UPROPERTY(Transient) TObjectPtr<UButton> ClearLeft;
    UPROPERTY(Transient) TObjectPtr<UButton> ClearRight;
    UPROPERTY(Transient) TObjectPtr<UButton> Confirm;
    UPROPERTY(Transient) TObjectPtr<UButton> Fold;
    UPROPERTY(Transient) TObjectPtr<UButton> Place;
    UPROPERTY(Transient) TObjectPtr<UButton> Close;
    UPROPERTY(Transient) TObjectPtr<UButton> ClampLeft;
    UPROPERTY(Transient) TObjectPtr<UButton> ClampRight;
    UPROPERTY(Transient) TObjectPtr<UButton> Dye;
    UPROPERTY(Transient) TObjectPtr<UButton> FinishDye;
    UFUNCTION() void OnCloth();
    UFUNCTION() void OnOrder();
    UFUNCTION() void OnSwap();
    UFUNCTION() void OnClearLeft();
    UFUNCTION() void OnClearRight();
    UFUNCTION() void OnConfirm();
    UFUNCTION() void OnExit();
    UFUNCTION() void OnFold();
    UFUNCTION() void OnPlace();
    UFUNCTION() void OnClose();
    UFUNCTION() void OnClampLeft();
    UFUNCTION() void OnClampRight();
    UFUNCTION() void OnRestart();
    UFUNCTION() void OnDye();
    UFUNCTION() void OnFinishDye();
    UFUNCTION() void OnWash();
    UFUNCTION() void OnDrain();
    UFUNCTION() void OnUnclampLeft();
    UFUNCTION() void OnUnclampRight();
    UFUNCTION() void OnUnfold();
    UFUNCTION() void OnDeliver();
    UFUNCTION() void OnReview();
    UFUNCTION() void OnBack();
    UFUNCTION() void OnTableCamera();
    UFUNCTION() void OnCloseupCamera();
    UFUNCTION() void OnQuiltCamera();
    UFUNCTION() void OnScoopTest();
    UFUNCTION() void OnDipTest();
    UFUNCTION() void OnLiftTest();
    UFUNCTION() void OnOxidizeTest();
    UFUNCTION() void OnAddFuel();
    UFUNCTION() void OnShip();
};
