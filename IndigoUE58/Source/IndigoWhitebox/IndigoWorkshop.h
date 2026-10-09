#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "IndigoSession.h"
#include "IndigoWorkshop.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;
class ACameraActor;

UCLASS(Blueprintable)
class INDIGOWHITEBOX_API AIndigoWorkbench : public AActor {
    GENERATED_BODY()
public:
    AIndigoWorkbench();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") FIndigoSession Session;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Indigo") UStaticMeshComponent* Cloth;
    UFUNCTION(BlueprintCallable, Category="Indigo") bool Command(const FString& Action, int32 Value=-1);
    UFUNCTION(BlueprintCallable, Category="Indigo") UTexture2D* GetQuiltTexture(bool Preview=false) const;
    UFUNCTION(BlueprintCallable, Category="Indigo") void RefreshArt();
private:
    UPROPERTY() TArray<UStaticMeshComponent*> Parts;
    UPROPERTY() UStaticMeshComponent* Bundle;
    UPROPERTY() UStaticMeshComponent* Lid;
    UPROPERTY() UStaticMeshComponent* LeftClip;
    UPROPERTY() UStaticMeshComponent* RightClip;
    UPROPERTY() UStaticMeshComponent* Strip;
    UPROPERTY() UMaterialInstanceDynamic* ClothMaterial;
    UPROPERTY() UMaterialInstanceDynamic* BundleMaterial;
    UPROPERTY() UMaterialInstanceDynamic* StripMaterial;
    UPROPERTY() ACameraActor* Camera;
    UStaticMeshComponent* Piece(FName Name, FVector Pos, FVector Scale, FLinearColor Color, bool Cylinder=false);
    void SetColor(UStaticMeshComponent* Mesh, FLinearColor Color);
};

UCLASS()
class INDIGOWHITEBOX_API AIndigoController : public APlayerController {
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    UPROPERTY() AIndigoWorkbench* Bench;
    FString Held;
    FVector2D PressAt=FVector2D::ZeroVector;
    bool MouseHeld=false;
    bool Moved=false;
    bool Help=false;
    FString Message;
    float MessageUntil=0;
    float SmokeTime=0;
    int32 SmokePhase=0;
    void Activate(const FString& Action);
    void DropOn(const FString& Target);
    void Tell(const FString& Text);
};

struct FIndigoHit { FString Action; FBox2D Bounds; bool Enabled; };

UCLASS()
class INDIGOWHITEBOX_API AIndigoHUD : public AHUD {
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    FString HitAt(FVector2D Point) const;
private:
    TArray<FIndigoHit> Hits;
    float Scale=1;
    FLinearColor Ink=FLinearColor(.11f,.18f,.23f);
    void Text(const FString& Value, float X,float Y,float Size=1, FLinearColor Color=FLinearColor(.11f,.18f,.23f));
    void Rect(float X,float Y,float W,float H,FLinearColor Color);
    void Button(const FString& Label,const FString& Action,float X,float Y,float W=270,bool Enabled=true);
    void Target(const FString& Label,const FString& Action,FVector World,bool Enabled=true);
};

UCLASS()
class INDIGOWHITEBOX_API AIndigoGameMode : public AGameModeBase {
    GENERATED_BODY()
public:
    AIndigoGameMode();
};
