#include "IndigoCraftPreview.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/Script.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoTemperatureTest,"Indigo.P1.TrialFuelAndDelivery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoTemperatureTest::RunTest(const FString& Parameters)
{
    FEditorScriptExecutionGuard ScriptGuard;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Director=World->SpawnActor<AIndigoDirector>();auto* Rig=World->SpawnActor<AIndigoCraftPreview>();auto* F=Director->Flow.Get();
    Rig->Flow=F;F->VisualTargets.Add(Rig);F->bUsePlaceholderTimer=false;F->bEnableTemperatureRules=true;F->bEnableDeliveryStory=true;
    Rig->MotionSeconds=.1f;Rig->DyeSeconds=.2f;
    auto Act=[&](EIndigoAction A,int32 I=0)
    {
        TestTrue(TEXT("Legal action accepted"),F->Perform(A,I));
        if(F->bAnimationBusy)
        {
            const int32 Token=F->GetAnimationToken();TestFalse(TEXT("Animation blocks repeat"),F->Perform(A,I));
            Rig->Tick(.3f);TestFalse(TEXT("Animation callback completes"),F->bAnimationBusy);TestFalse(TEXT("Old completion rejected"),F->CompleteAnimation(Token));
        }
    };
    for(int32 Depth:{1,4})
    {
        F->Restart();Act(EIndigoAction::InspectCloth);Act(EIndigoAction::InspectOrder);F->SetPattern(true,EIndigoPattern::A);F->SetPattern(false,Depth==1?EIndigoPattern::A:EIndigoPattern::B);Act(EIndigoAction::Confirm);
        TestTrue(TEXT("Trial required before folding"),F->Step==EIndigoStep::Trial);
        TestFalse(TEXT("Cannot heat before seeing cold sample"),F->Perform(EIndigoAction::AddFuel));
        TestFalse(TEXT("Cannot skip test order"),F->Perform(EIndigoAction::LiftTest));
        for(auto A:{EIndigoAction::ScoopTest,EIndigoAction::DipTest,EIndigoAction::LiftTest,EIndigoAction::OxidizeTest})Act(A);
        TestTrue(TEXT("Cold sample recorded"),F->bColdTrialSeen && !F->bWarmTrialSeen);TestEqual(TEXT("Trial does not consume fuel"),F->Fuel,5);
        TestFalse(TEXT("Cannot retry cold without heating"),F->Perform(EIndigoAction::ScoopTest));
        Act(EIndigoAction::AddFuel);TestEqual(TEXT("One fuel consumed"),F->Fuel,4);TestTrue(TEXT("Heat ends ready"),F->Temperature==EIndigoTemperature::Ready);
        TestFalse(TEXT("Ready cannot waste fuel"),F->Perform(EIndigoAction::AddFuel));
        for(auto A:{EIndigoAction::ScoopTest,EIndigoAction::DipTest,EIndigoAction::LiftTest,EIndigoAction::OxidizeTest})Act(A);
        TestTrue(TEXT("Warm trial unlocks fold"),F->bWarmTrialSeen && F->Step==EIndigoStep::Fold);TestEqual(TEXT("Test never adds result depth"),F->Result.Depth,0);
        for(int32 I=0;I<3;++I)Act(EIndigoAction::Fold,I);
        for(auto A:{EIndigoAction::PlaceCloth,EIndigoAction::ClosePlate,EIndigoAction::ClampLeft,EIndigoAction::ClampRight})Act(A);
        TestFalse(TEXT("Zero cannot finish"),F->Perform(EIndigoAction::FinishDye));
        for(int32 I=0;I<Depth;++I)
        {
            if(I) { TestFalse(TEXT("Cold blocks dye"),F->Perform(EIndigoAction::Dye));Act(EIndigoAction::AddFuel); }
            Act(EIndigoAction::Dye);TestTrue(TEXT("Completed cycle cools"),F->Temperature==EIndigoTemperature::Cold);TestEqual(TEXT("Shared depth retained"),F->Result.Depth,I+1);
        }
        TestEqual(TEXT("At most four fuel consumed"),F->Fuel,5-Depth);
        if(Depth==4){TestFalse(TEXT("Fourth cannot heat or dye"),F->Perform(EIndigoAction::AddFuel));TestFalse(TEXT("No fifth cycle"),F->Perform(EIndigoAction::Dye));}
        Act(EIndigoAction::FinishDye);
        for(auto A:{EIndigoAction::Wash,EIndigoAction::Drain,EIndigoAction::UnclampLeft,EIndigoAction::UnclampRight,EIndigoAction::Unfold})Act(A);
        Act(EIndigoAction::Deliver);TestTrue(TEXT("Story appears after reveal"),F->Step==EIndigoStep::Delivery);
        TestFalse(TEXT("Reading cannot be skipped immediately"),F->Perform(EIndigoAction::Ship));F->DeliveryReadingSeconds=0;Act(EIndigoAction::Ship);F->DeliveryReadingSeconds=8;
        TestTrue(TEXT("Shipping reaches end"),F->Step==EIndigoStep::End);Act(EIndigoAction::Review);TestEqual(TEXT("Review retains depth"),F->Result.Depth,Depth);Act(EIndigoAction::Back);
    }
    F->Restart();TestEqual(TEXT("Reset fuel"),F->Fuel,5);TestTrue(TEXT("Reset temperature and samples"),F->Temperature==EIndigoTemperature::Cold && !F->bColdTrialSeen && !F->bWarmTrialSeen);
    F->Step=EIndigoStep::Dye;F->Fuel=0;TestFalse(TEXT("Empty fuel cannot heat"),F->Perform(EIndigoAction::AddFuel));TestFalse(TEXT("Empty cold cannot dye"),F->Perform(EIndigoAction::Dye));
    F->Fuel=5;F->Perform(EIndigoAction::AddFuel);int32 Old=F->GetAnimationToken();F->Restart();Rig->Tick(1);TestFalse(TEXT("Old heat does not warm new run"),F->CompleteAnimation(Old));TestTrue(TEXT("New run remains cold"),F->Temperature==EIndigoTemperature::Cold);
    World->DestroyWorld(false);return true;
}
#endif
