#include "IndigoCraftPreview.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/Script.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoFinishTest,"Indigo.Workbench.WashRevealCallbacks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoFinishTest::RunTest(const FString& Parameters)
{
    FEditorScriptExecutionGuard ScriptGuard;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Director=World->SpawnActor<AIndigoDirector>(); auto* Rig=World->SpawnActor<AIndigoCraftPreview>();
    auto* F=Director->Flow.Get(); Rig->Flow=F;F->VisualTargets.Add(Rig);F->bUsePlaceholderTimer=false;Rig->MotionSeconds=1.f;Rig->DyeSeconds=1.f;
    auto Complete=[&](EIndigoAction Action,int32 Index=0)
    {
        TestTrue(TEXT("Action accepted in order"),F->Perform(Action,Index)); const int32 Token=F->GetAnimationToken();
        Rig->Tick(.5f);TestTrue(TEXT("Waiting for motion end"),F->bAnimationBusy);
        TestFalse(TEXT("Repeat during motion rejected"),F->Perform(Action,Index));
        Rig->Tick(.6f);TestFalse(TEXT("Motion completed"),F->bAnimationBusy);TestFalse(TEXT("Duplicate rejected"),F->CompleteAnimation(Token));
    };
    for(int32 D:{1,4})
    {
        F->Restart();F->Perform(EIndigoAction::InspectCloth);F->Perform(EIndigoAction::InspectOrder);
        F->SetPattern(true,EIndigoPattern::A);F->SetPattern(false,D==1?EIndigoPattern::A:EIndigoPattern::B);F->Perform(EIndigoAction::Confirm);
        for(int32 I=0;I<3;++I) Complete(EIndigoAction::Fold,I);
        for(auto A:{EIndigoAction::PlaceCloth,EIndigoAction::ClosePlate,EIndigoAction::ClampLeft,EIndigoAction::ClampRight}) Complete(A);
        for(int32 I=0;I<D;++I) Complete(EIndigoAction::Dye);
        F->Perform(EIndigoAction::FinishDye);
        TestFalse(TEXT("Cannot unfold before washing"),F->Perform(EIndigoAction::Unfold));
        TestFalse(TEXT("Cannot drain before washing"),F->Perform(EIndigoAction::Drain));
        Complete(EIndigoAction::Wash);TestFalse(TEXT("Cannot release before drain"),F->Perform(EIndigoAction::UnclampLeft));Complete(EIndigoAction::Drain);
        Complete(D==1?EIndigoAction::UnclampLeft:EIndigoAction::UnclampRight);
        TestFalse(TEXT("One clamp still prevents unfold"),F->Perform(EIndigoAction::Unfold));
        Complete(D==1?EIndigoAction::UnclampRight:EIndigoAction::UnclampLeft);Complete(EIndigoAction::Unfold);
        TestTrue(TEXT("Reveal waits for delivery"),F->Step==EIndigoStep::Reveal && Rig->bUnfolded);
        TestEqual(TEXT("Shared depth displayed"),Rig->DisplayedResult.Depth,D);
        TestTrue(TEXT("Shared pair displayed"),Rig->DisplayedResult.Left==F->Result.Left && Rig->DisplayedResult.Right==F->Result.Right);
        TestTrue(TEXT("Full cloth restored"),Rig->Cloth->GetRelativeScale3D().Equals(FVector(1.08,.76,.0035),.0001));
        Rig->Tick(20.f);TestTrue(TEXT("No automatic delivery"),F->Step==EIndigoStep::Reveal);
        Complete(EIndigoAction::Deliver);TestTrue(TEXT("Delivery reaches end"),F->Step==EIndigoStep::End);
        F->Perform(EIndigoAction::Review);TestTrue(TEXT("Review preserves result"),F->IsReview() && F->Result.Depth==D);
        TestFalse(TEXT("Review cannot deliver again"),F->Perform(EIndigoAction::Deliver));F->Perform(EIndigoAction::Back);
        TestTrue(TEXT("Back returns to end"),F->Step==EIndigoStep::End);F->Restart();Rig->Tick(.1f);
        TestFalse(TEXT("Restart clears unfolded state"),Rig->bUnfolded);TestEqual(TEXT("Restart clears displayed result"),Rig->DisplayedResult.Depth,0);
        TestFalse(TEXT("Wash basin hidden after reset"),Rig->WashWater->IsVisible());
    }
    // Interrupt a real wash; its old completion must not affect the next run.
    F->Step=EIndigoStep::Wash;F->Perform(EIndigoAction::Wash);const int32 Old=F->GetAnimationToken();Rig->Tick(.5f);F->Restart();Rig->Tick(5.f);
    TestFalse(TEXT("Old wash completion rejected"),F->CompleteAnimation(Old));
    TestTrue(TEXT("Reset restores assembly"),Rig->LowerPlate->GetRelativeLocation().Equals(FVector(0,52,2),.001f));
    World->DestroyWorld(false);return true;
}
#endif
