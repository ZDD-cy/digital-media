#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IndigoVisualInterface.h"
#include "IndigoCraftPreview.generated.h"

class UIndigoFlowComponent;
class UStaticMeshComponent;
class USceneComponent;
class UMaterialInstanceDynamic;
class AIndigoCraftHandle;
class UTexture2D;

// Replaceable blockout rig. Its own completed motion returns the original flow token.
UCLASS(Blueprintable)
class INDIGOINTERACTION_API AIndigoCraftPreview : public AActor, public IIndigoVisualInterface
{
    GENERATED_BODY()
public:
    AIndigoCraftPreview();
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TObjectPtr<UIndigoFlowComponent> Flow;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="0.1")) float MotionSeconds = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo", meta=(ClampMin="0.2")) float DyeSeconds = 4.f;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TArray<TObjectPtr<AActor>> SelectionSamples;
    // AA depths 0..4, AB 0..4 ... CC 0..4, using the existing artwork.
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TArray<TObjectPtr<UTexture2D>> ResultTextures;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Indigo") TObjectPtr<UTexture2D> UndyedTexture;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> Cloth;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> MovingCloth;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> FoldLine;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> LowerPlate;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> UpperPlate;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> LeftClamp;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> RightClamp;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> DyeBasin;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> DyeWater;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> DyeSwatch;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> WashBasin;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") TObjectPtr<UStaticMeshComponent> WashWater;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|P1") TObjectPtr<UStaticMeshComponent> TrialCloth;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo|P1") TArray<TObjectPtr<UStaticMeshComponent>> FuelMarkers;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bUnfolded = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") FIndigoResult DisplayedResult;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") int32 PreviewDepth = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") float DyeProgress = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") int32 CompletedFolds = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bPlaced = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bClosed = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bLeftClamped = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bRightClamped = false;
    virtual void Tick(float DeltaSeconds) override;
    virtual void ResetVisuals_Implementation() override;
    virtual void PlayIndigoAnimation_Implementation(FName Cue, int32 Token) override;
    virtual void SetDyeProgress_Implementation(int32 CompletedDepth, float Progress) override;
    virtual void ShowFinalQuilt_Implementation(const FIndigoResult& Result) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY() TObjectPtr<USceneComponent> FoldPivot;
    UPROPERTY() TObjectPtr<USceneComponent> PlatePivot;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ClothMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> SwatchMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> TrialMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<AIndigoCraftHandle>> Handles;
    TArray<FVector> DyeStartLocations;
    FVector ClothCenter;
    FVector ClothSize;
    FVector MoveStart;
    FVector UnfoldStartSize;
    FName ActiveCue;
    int32 MotionToken = 0;
    float Elapsed = 0.f;
    bool bFoldX = true;
    void PoseCloth();
    void ApplyMotion(float T);
    void FinishMotion();
    TArray<USceneComponent*> DyeParts() const;
    void UpdateHandles();
};
