#include "IndigoCraftPreview.h"
#include "IndigoWorkbenchBridge.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "IndigoPatternSource.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "UObject/Script.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoCraftTest,"Indigo.Workbench.CraftCallbacks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoCraftTest::RunTest(const FString& Parameters)
{
    // This isolated test world has no BeginPlay; allow Actor::ProcessEvent for interface dispatch.
    FEditorScriptExecutionGuard ScriptGuard;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Director=World->SpawnActor<AIndigoDirector>();
    auto* Bridge=World->SpawnActor<AIndigoWorkbenchBridge>();
    auto* Preview=World->SpawnActor<AIndigoCraftPreview>();
    auto* F=Director->Flow.Get();
    Bridge->Director=Director; Bridge->bWorkbenchActive=true;
    F->bUsePlaceholderTimer=false; F->VisualTargets.Add(Preview); Preview->Flow=F; Preview->MotionSeconds=1.f;
    TestTrue(TEXT("Rig implements visual interface"),Preview->Implements<UIndigoVisualInterface>());
    auto Select=[&]()
    {
        Bridge->PerformAction(EIndigoAction::InspectCloth); Bridge->PerformAction(EIndigoAction::InspectOrder);
        F->SetPattern(true,EIndigoPattern::A); F->SetPattern(false,EIndigoPattern::B); Bridge->PerformAction(EIndigoAction::Confirm);
    };
    auto Animate=[&](EIndigoAction Action,int32 Index=0)
    {
        TestTrue(TEXT("Bridge accepts indexed action"),Bridge->PerformAction(Action,Index));
        const int32 Token=F->GetAnimationToken();
        Preview->Tick(.45f);
        TestTrue(TEXT("Half-finished visual keeps flow locked"),F->bAnimationBusy);
        TestFalse(TEXT("Rapid repeated input rejected"),Bridge->PerformAction(Action,Index));
        TestFalse(TEXT("Old callback rejected"),F->CompleteAnimation(Token-1));
        Preview->Tick(.6f);
        TestFalse(TEXT("Visual completion unlocks flow without fallback timer"),F->bAnimationBusy);
        TestFalse(TEXT("Duplicate callback rejected"),F->CompleteAnimation(Token));
    };
    for(int32 Run=0;Run<2;++Run)
    {
        Bridge->RestartWorkbench(); Select();
        TestFalse(TEXT("Cannot skip first fold"),Bridge->PerformAction(EIndigoAction::Fold,1));
        TestFalse(TEXT("Cannot place before folding"),Bridge->PerformAction(EIndigoAction::PlaceCloth));
        for(int32 I=0;I<3;++I)
        {
            Animate(EIndigoAction::Fold,I);
            TestEqual(TEXT("Exactly one completed fold"),F->FoldIndex,I+1);
            TestEqual(TEXT("Visual fold matches state"),Preview->CompletedFolds,I+1);
        }
        TestTrue(TEXT("Three folds enter clamp stage"),F->Step==EIndigoStep::Clamp);
        TestFalse(TEXT("Cannot close before placing"),Bridge->PerformAction(EIndigoAction::ClosePlate));
        TestFalse(TEXT("Cannot clamp open board"),Bridge->PerformAction(EIndigoAction::ClampLeft));
        Animate(EIndigoAction::PlaceCloth); TestTrue(TEXT("Cloth placed visually"),Preview->bPlaced);
        TestFalse(TEXT("Cannot place twice"),Bridge->PerformAction(EIndigoAction::PlaceCloth));
        Animate(EIndigoAction::ClosePlate); TestTrue(TEXT("Upper board closed visually"),Preview->bClosed);
        Animate(Run==0?EIndigoAction::ClampRight:EIndigoAction::ClampLeft);
        TestFalse(TEXT("Single clamp cannot start dye"),Bridge->PerformAction(EIndigoAction::Dye));
        TestFalse(TEXT("Clamped side cannot repeat"),Bridge->PerformAction(Run==0?EIndigoAction::ClampRight:EIndigoAction::ClampLeft));
        Animate(Run==0?EIndigoAction::ClampLeft:EIndigoAction::ClampRight);
        TestTrue(TEXT("Both clamps enter dye"),F->Step==EIndigoStep::Dye && Preview->bLeftClamped && Preview->bRightClamped);
        TestTrue(TEXT("Selection preserved"),F->Result.Left==EIndigoPattern::A && F->Result.Right==EIndigoPattern::B);
    }
    Bridge->RestartWorkbench(); Select();
    Bridge->PerformAction(EIndigoAction::Fold,0); const int32 CancelledToken=F->GetAnimationToken(); Preview->Tick(.4f);
    Bridge->RestartWorkbench(); Preview->Tick(2.f);
    TestTrue(TEXT("Restart hides rig and resets pose"),Preview->IsHidden() && Preview->CompletedFolds==0 && !Preview->MovingCloth->IsVisible());
    TestTrue(TEXT("Restart restores cloth dimensions with authored yaw"),Preview->Cloth->GetStaticMesh()->GetBounds().TransformBy(Preview->Cloth->GetComponentTransform()).BoxExtent.Equals(FVector(54,38,.175),.001));
    Select(); Bridge->PerformAction(EIndigoAction::Fold,0);
    TestFalse(TEXT("Cancelled run cannot complete new motion"),F->CompleteAnimation(CancelledToken));
    Preview->Tick(1.1f);
    TestEqual(TEXT("New run folds once"),F->FoldIndex,1);
    Bridge->bWorkbenchActive=false; World->DestroyWorld(false);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoFoldClearanceTest,"Indigo.Workbench.FoldSurfaceClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoFoldClearanceTest::RunTest(const FString& Parameters)
{
    FEditorScriptExecutionGuard Guard;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Director=World->SpawnActor<AIndigoDirector>();auto* Rig=World->SpawnActor<AIndigoCraftPreview>();
    auto* Flow=Director->Flow.Get();Rig->Flow=Flow;Flow->VisualTargets.Add(Rig);Flow->bUsePlaceholderTimer=false;Flow->bSkipReceipt=true;
    auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    Rig->Cloth->SetStaticMesh(Plane);Rig->MovingCloth->SetStaticMesh(Plane);Rig->MotionSeconds=1.f;
    Flow->Restart();Flow->SetPattern(true,EIndigoPattern::A);Flow->SetPattern(false,EIndigoPattern::B);Flow->Perform(EIndigoAction::Confirm);
    for(int32 Fold=0;Fold<3;++Fold)
    {
        const float RestZ=Plane->GetBoundingBox().TransformBy(Rig->Cloth->GetComponentTransform()).Min.Z;
        TestTrue(TEXT("Start fold"),Flow->Perform(EIndigoAction::Fold,Fold));
        for(int32 Sample=0;Sample<19;++Sample)
        {
            Rig->Tick(.05f);
            const FBox Moving=Plane->GetBoundingBox().TransformBy(Rig->MovingCloth->GetComponentTransform());
            TestTrue(TEXT("Folded sheet never goes below the resting layer"),Moving.Min.Z>=RestZ-.001f);
        }
        Rig->Tick(.1f);
    }
    TestTrue(TEXT("Three safe folds complete"),Flow->Step==EIndigoStep::Clamp);
    TestTrue(TEXT("Screen-left clamp is positive X for the workbench cameras"),Rig->LeftClamp->GetRelativeLocation().X>Rig->RightClamp->GetRelativeLocation().X);
    World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoPlatePatternTest,"Indigo.Workbench.SelectedPatternPlates",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoPlatePatternTest::RunTest(const FString& Parameters)
{
    FEditorScriptExecutionGuard Guard;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Director=World->SpawnActor<AIndigoDirector>();
    auto* Rig=World->SpawnActor<AIndigoCraftPreview>();
    auto* Flow=Director->Flow.Get();
    Flow->VisualTargets.Add(Rig);Flow->bSkipReceipt=true;Flow->bUsePlaceholderTimer=false;

    UStaticMesh* Meshes[]{
        LoadObject<UStaticMesh>(nullptr,TEXT("/Game/TripoModels/mask1/239f81c9-5b7c-41ef-8d54-116de3ecea54.239f81c9-5b7c-41ef-8d54-116de3ecea54")),
        LoadObject<UStaticMesh>(nullptr,TEXT("/Game/TripoModels/mask2/42222493-7cdb-4a65-8c65-a3656f0036a9.42222493-7cdb-4a65-8c65-a3656f0036a9")),
        LoadObject<UStaticMesh>(nullptr,TEXT("/Game/TripoModels/mask3/eb673182-2168-4c8a-baeb-76b62a7d7193.eb673182-2168-4c8a-baeb-76b62a7d7193"))
    };
    UMaterialInterface* Materials[]{
        LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TripoModels/mask1/239f81c9-5b7c-41ef-8d54-116de3ecea54_Mat.239f81c9-5b7c-41ef-8d54-116de3ecea54_Mat")),
        LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TripoModels/mask2/42222493-7cdb-4a65-8c65-a3656f0036a9_Mat.42222493-7cdb-4a65-8c65-a3656f0036a9_Mat")),
        LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TripoModels/mask3/eb673182-2168-4c8a-baeb-76b62a7d7193_Mat.eb673182-2168-4c8a-baeb-76b62a7d7193_Mat"))
    };
    const EIndigoPattern Patterns[]{EIndigoPattern::A,EIndigoPattern::B,EIndigoPattern::C};
    for(int32 I=0;I<3;++I)
    {
        TestNotNull(TEXT("Pattern static mesh loads"),Meshes[I]);
        TestNotNull(TEXT("Pattern material loads"),Materials[I]);
        auto* Source=World->SpawnActor<AIndigoPatternSource>();
        Source->Pattern=Patterns[I];
        Source->BoardMesh->SetStaticMesh(Meshes[I]);
        Source->BoardMesh->SetMaterial(0,Materials[I]);
    }

    auto PlateCenter=[Rig](UStaticMeshComponent* Plate)
    {
        FVector Min,Max;Plate->GetLocalBounds(Min,Max);
        return Plate->GetComponentTransform().TransformPosition((Min+Max)*.5f);
    };
    const FVector LowerCenter=PlateCenter(Rig->LowerPlate);
    const FVector UpperCenter=PlateCenter(Rig->UpperPlate);
    Flow->Restart();
    TestTrue(TEXT("Left selection applied"),Flow->SetPattern(true,EIndigoPattern::B));
    TestTrue(TEXT("Right selection applied"),Flow->SetPattern(false,EIndigoPattern::C));
    TestTrue(TEXT("Lower plate uses left pattern mesh"),Rig->LowerPlate->GetStaticMesh()==Meshes[1]);
    TestTrue(TEXT("Upper plate uses right pattern mesh"),Rig->UpperPlate->GetStaticMesh()==Meshes[2]);
    TestTrue(TEXT("Lower plate uses left pattern material"),Rig->LowerPlate->GetMaterial(0)==Materials[1]);
    TestTrue(TEXT("Upper plate uses right pattern material"),Rig->UpperPlate->GetMaterial(0)==Materials[2]);
    TestTrue(TEXT("Lower plate keeps its movement center"),PlateCenter(Rig->LowerPlate).Equals(LowerCenter,.01f));
    TestTrue(TEXT("Upper plate keeps its movement center"),PlateCenter(Rig->UpperPlate).Equals(UpperCenter,.01f));

    auto PlateHasCanonicalFootprint=[Rig](UStaticMeshComponent* Plate,const FVector& CanonicalSize)
    {
        FVector Min,Max;Plate->GetLocalBounds(Min,Max);
        // GetLocalBounds is unscaled. Compare after component scaling in the
        // plate's own rotated frame; UpperPlate is parented to a rotating
        // 110-degree plate pivot, so its world AABB is not stable.
        const FVector ScaledSize=(Max-Min)*Plate->GetComponentScale().GetAbs();
        const FVector ExpectedSize=Plate->GetRelativeRotation().UnrotateVector(CanonicalSize).GetAbs();
        return ScaledSize.Equals(ExpectedSize,.2f);
    };
    TestTrue(TEXT("Lower plate keeps its authored footprint"),PlateHasCanonicalFootprint(Rig->LowerPlate,FVector(46,44,4)));
    TestTrue(TEXT("Upper plate keeps its authored footprint"),PlateHasCanonicalFootprint(Rig->UpperPlate,FVector(46,44,3.5f)));
    World->DestroyWorld(false);return true;
}
#endif
