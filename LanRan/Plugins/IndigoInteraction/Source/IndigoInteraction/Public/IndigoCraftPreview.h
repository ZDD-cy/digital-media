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
class UMaterialInterface;
class UStaticMesh;

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Materials") FRotator ClothRotation = FRotator(0.f,-90.f,0.f);
    // Applied only after the cloth is unfolded for the final presentation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Presentation") FRotator FinalClothRotationOffset = FRotator(0.f,90.f,0.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Materials") TObjectPtr<UMaterialInterface> RuntimeClothMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Geometry", meta=(ClampMin="90")) float BasinDiameter = 112.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Geometry", meta=(ClampMin="100")) float BasinDistance = 116.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Indigo|Geometry", meta=(ClampMin="0")) float PlateCloseClearance = .1f;
    UFUNCTION(BlueprintPure, Category="Indigo|Materials") static float ColorDepthForStage(int32 Depth);
    UFUNCTION(BlueprintPure, Category="Indigo|Materials") static FLinearColor ClothColorForStage(int32 Depth);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") float DyeProgress = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") int32 CompletedFolds = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bPlaced = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bClosed = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bLeftClamped = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") bool bRightClamped = false;
    virtual void Tick(float DeltaSeconds) override;
    virtual void ResetVisuals_Implementation() override;
    virtual void PlayIndigoAnimation_Implementation(FName Cue, int32 Token) override;
    virtual void SetPattern_Implementation(EIndigoPattern Left, EIndigoPattern Right) override;
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
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> AuthoredLowerPlateMesh;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> AuthoredUpperPlateMesh;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> AuthoredLowerPlateMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> AuthoredUpperPlateMaterials;
    TArray<FVector> DyeStartLocations;
    FVector ClothCenter;
    FVector ClothSize;
    FVector MoveStart;
    FVector UnfoldStartSize;
    FVector UpperPlateCloseStart;
    float UpperPlateCloseDrop = 0.f;
    FName ActiveCue;
    int32 MotionToken = 0;
    float Elapsed = 0.f;
    bool bFoldX = true;
    bool bPlateDefaultsCaptured = false;
    void CapturePlateDefaults();
    void RestorePlateVisual(UStaticMeshComponent* Plate,UStaticMesh* DefaultMesh,const TArray<TObjectPtr<UMaterialInterface>>& DefaultMaterials,const FVector& CanonicalSize);
    void ApplyPlatePattern(UStaticMeshComponent* Plate,EIndigoPattern Pattern,const FVector& CanonicalSize);
    void PreparePlateClose();
    void PoseCloth();
    void ApplyMotion(float T);
    void FinishMotion();
    TArray<USceneComponent*> DyeParts() const;
    void UpdateHandles();
    TMap<USceneComponent*,FVector> PartCenters;
    FVector PartCenter(USceneComponent* Part) const;
    void SetPartLocation(USceneComponent* Part,const FVector& Center);
    void SetPartScale(UStaticMeshComponent* Part,const FVector& CanonicalScale);
    void UpdateClothParameters();
    FVector BasinMotion(float T,bool bWash) const;
};
