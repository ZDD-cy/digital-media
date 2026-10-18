#include "IndigoCraftPreview.h"
#include "IndigoWorkbenchBridge.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
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
    TestTrue(TEXT("Restart restores cloth size"),Preview->Cloth->GetRelativeScale3D().Equals(FVector(1.08,.76,.0035),.0001));
    Select(); Bridge->PerformAction(EIndigoAction::Fold,0);
    TestFalse(TEXT("Cancelled run cannot complete new motion"),F->CompleteAnimation(CancelledToken));
    Preview->Tick(1.1f);
    TestEqual(TEXT("New run folds once"),F->FoldIndex,1);
    Bridge->bWorkbenchActive=false; World->DestroyWorld(false);
    return true;
}
#endif
