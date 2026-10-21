#include "IndigoCraftPreview.h"
#include "IndigoWorkbenchBridge.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/Script.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoDyeTest,"Indigo.Workbench.DyeDepthCallbacks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoDyeTest::RunTest(const FString& Parameters)
{
    FEditorScriptExecutionGuard ScriptGuard;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Director=World->SpawnActor<AIndigoDirector>();
    auto* Bridge=World->SpawnActor<AIndigoWorkbenchBridge>();
    auto* Rig=World->SpawnActor<AIndigoCraftPreview>();
    auto* F=Director->Flow.Get();
    Bridge->Director=Director;Bridge->bWorkbenchActive=true;Rig->Flow=F;
    F->VisualTargets.Add(Rig);F->bUsePlaceholderTimer=false;Rig->MotionSeconds=.2f;Rig->DyeSeconds=4.f;
    auto Prepare=[&]()
    {
        Bridge->RestartWorkbench();
        Bridge->PerformAction(EIndigoAction::InspectCloth);Bridge->PerformAction(EIndigoAction::InspectOrder);
        F->SetPattern(true,EIndigoPattern::A);F->SetPattern(false,EIndigoPattern::B);Bridge->PerformAction(EIndigoAction::Confirm);
        for(int32 I=0;I<3;++I){Bridge->PerformAction(EIndigoAction::Fold,I);Rig->Tick(.3f);}
        for(auto A:{EIndigoAction::PlaceCloth,EIndigoAction::ClosePlate,EIndigoAction::ClampLeft,EIndigoAction::ClampRight}){Bridge->PerformAction(A);Rig->Tick(.3f);}
        TestTrue(TEXT("Prepared at dye stage"),F->Step==EIndigoStep::Dye);
    };
    for(int32 TargetDepth:{1,4})
    {
        Prepare();
        TestFalse(TEXT("Zero depth cannot finish"),Bridge->PerformAction(EIndigoAction::FinishDye));
        const FVector Home=Rig->LowerPlate->GetRelativeLocation();
        for(int32 D=0;D<TargetDepth;++D)
        {
            TestTrue(TEXT("Start next depth"),Bridge->PerformAction(EIndigoAction::Dye));
            const int32 Token=F->GetAnimationToken();
            Rig->Tick(2.f);
            TestEqual(TEXT("Immersion alone does not count"),F->Result.Depth,D);
            TestTrue(TEXT("Assembly enters the basin above the tabletop"),Rig->LowerPlate->GetRelativeLocation().X>Home.X+80.f && Rig->LowerPlate->GetRelativeLocation().Z<Home.Z+20.f);
            TestFalse(TEXT("Rapid repeat rejected"),Bridge->PerformAction(EIndigoAction::Dye));
            TestFalse(TEXT("Cannot finish halfway"),Bridge->PerformAction(EIndigoAction::FinishDye));
            Rig->Tick(1.f);
            TestTrue(TEXT("Oxidation progress reported"),Rig->DyeProgress>0.f && Rig->DyeProgress<1.f);
            TestEqual(TEXT("Depth still unchanged during oxidation"),F->Result.Depth,D);
            Rig->Tick(1.1f);
            TestEqual(TEXT("Full cycle adds exactly one"),F->Result.Depth,D+1);
            TestEqual(TEXT("Preview receives completed shared depth"),Rig->PreviewDepth,D+1);
            TestTrue(TEXT("Assembly returns to workbench"),Rig->LowerPlate->GetRelativeLocation().Equals(Home,.001f));
            TestFalse(TEXT("Duplicate completion rejected"),F->CompleteAnimation(Token));
        }
        if(TargetDepth==4)TestFalse(TEXT("No fifth depth"),Bridge->PerformAction(EIndigoAction::Dye));
        TestTrue(TEXT("Completed depth can finish"),Bridge->PerformAction(EIndigoAction::FinishDye));
        TestTrue(TEXT("Finish preserves data and enters wash"),F->Step==EIndigoStep::Wash && F->Result.Depth==TargetDepth && F->Result.Left==EIndigoPattern::A);
    }
    Prepare();Bridge->PerformAction(EIndigoAction::Dye);const int32 Old=F->GetAnimationToken();Rig->Tick(2.f);
    Bridge->RestartWorkbench();Rig->Tick(5.f);
    TestEqual(TEXT("Restart resets depth and shade"),Rig->PreviewDepth,0);
    TestTrue(TEXT("Restart restores boards from basin"),Rig->LowerPlate->GetRelativeLocation().Equals(FVector(0,52,2),.001f));
    TestFalse(TEXT("Old dye callback invalidated"),F->CompleteAnimation(Old));
    TestFalse(TEXT("Basin hidden outside dye"),Rig->DyeWater->IsVisible());
    Bridge->bWorkbenchActive=false;World->DestroyWorld(false);return true;
}
#endif
