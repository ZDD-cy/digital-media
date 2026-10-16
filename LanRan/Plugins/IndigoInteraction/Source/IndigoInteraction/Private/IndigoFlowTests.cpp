#include "IndigoFlowComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoFlowTest, "Indigo.Flow.GuardsAndReplay", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIndigoFlowTest::RunTest(const FString& Parameters)
{
    UIndigoFlowComponent* F = NewObject<UIndigoFlowComponent>();
    F->bUsePlaceholderTimer = false;
    const auto Act = [this, F](EIndigoAction A, int32 Index = 0)
    {
        TestTrue(TEXT("Accept legal action"), F->Perform(A, Index));
        if (F->bAnimationBusy)
        {
            const int32 Token = F->GetAnimationToken();
            TestFalse(TEXT("Reject stale callback"), F->CompleteAnimation(Token - 1));
            TestFalse(TEXT("Reject input during animation"), F->Perform(A, Index));
            TestTrue(TEXT("Complete current animation"), F->CompleteAnimation(Token));
            TestFalse(TEXT("Reject duplicate callback"), F->CompleteAnimation(Token));
        }
    };
    for (int32 Run = 0; Run < 2; ++Run)
    {
        F->Restart();
        TestEqual(TEXT("Reset depth"), F->Result.Depth, 0);
        TestTrue(TEXT("Reset pattern"), F->Result.Left == EIndigoPattern::None);
        Act(EIndigoAction::InspectCloth); Act(EIndigoAction::InspectOrder);
        TestFalse(TEXT("Cannot confirm empty selection"), F->Perform(EIndigoAction::Confirm));
        TestTrue(TEXT("Set left"), F->SetPattern(true, EIndigoPattern::A));
        TestTrue(TEXT("Set right"), F->SetPattern(false, Run == 0 ? EIndigoPattern::A : EIndigoPattern::B));
        Act(EIndigoAction::Swap);
        TestTrue(TEXT("Swap updates actual result"), F->Result.Left == (Run == 0 ? EIndigoPattern::A : EIndigoPattern::B));
        Act(EIndigoAction::Swap); Act(EIndigoAction::Confirm);
        TestFalse(TEXT("Confirmed selection locked"), F->SetPattern(true, EIndigoPattern::C));
        TestFalse(TEXT("Reject out of order fold"), F->Perform(EIndigoAction::Fold, 1));
        for (int32 Fold = 0; Fold < F->FoldCount; ++Fold) Act(EIndigoAction::Fold, Fold);
        TestFalse(TEXT("Must place cloth first"), F->Perform(EIndigoAction::ClosePlate));
        Act(EIndigoAction::PlaceCloth); Act(EIndigoAction::ClosePlate); Act(EIndigoAction::ClampLeft);
        TestFalse(TEXT("Both clamps required"), F->Perform(EIndigoAction::Dye));
        Act(EIndigoAction::ClampRight);
        TestFalse(TEXT("Depth zero cannot finish"), F->Perform(EIndigoAction::FinishDye));
        F->SetCameraBusy(true);
        TestFalse(TEXT("Camera blend locks input"), F->Perform(EIndigoAction::Dye)); F->SetCameraBusy(false);
        const int32 Depth = Run == 0 ? 1 : 4;
        for (int32 D = 0; D < Depth; ++D) Act(EIndigoAction::Dye);
        TestEqual(TEXT("Expected dye depth"), F->Result.Depth, Depth);
        if (Run == 1) TestFalse(TEXT("No fifth stage"), F->Perform(EIndigoAction::Dye));
        Act(EIndigoAction::FinishDye); Act(EIndigoAction::Wash); Act(EIndigoAction::Drain);
        TestFalse(TEXT("Cannot unfold while clamped"), F->Perform(EIndigoAction::Unfold));
        Act(EIndigoAction::UnclampRight); Act(EIndigoAction::UnclampLeft); Act(EIndigoAction::Unfold);
        TestTrue(TEXT("Reveal can stay"), F->Step == EIndigoStep::Reveal);
        Act(EIndigoAction::Deliver); Act(EIndigoAction::Review); Act(EIndigoAction::Back);
        TestTrue(TEXT("Return to end"), F->Step == EIndigoStep::End);
        TestEqual(TEXT("Review preserves result"), F->Result.Depth, Depth);
    }
    F->Restart(); Act(EIndigoAction::InspectCloth); Act(EIndigoAction::InspectOrder);
    F->SetPattern(true, EIndigoPattern::A); F->SetPattern(false, EIndigoPattern::B); Act(EIndigoAction::Confirm);
    F->Perform(EIndigoAction::Fold, 0);
    const int32 OldToken = F->GetAnimationToken();
    F->Restart();
    TestFalse(TEXT("Restart invalidates old callbacks"), F->CompleteAnimation(OldToken));
    TestEqual(TEXT("Restart resets folding"), F->FoldIndex, 0);
    Act(EIndigoAction::InspectCloth); Act(EIndigoAction::InspectOrder);
    F->SetPattern(true, EIndigoPattern::C); F->SetPattern(false, EIndigoPattern::C); Act(EIndigoAction::Confirm);
    F->Perform(EIndigoAction::Fold, 0);
    TestFalse(TEXT("Old run cannot complete new animation"), F->CompleteAnimation(OldToken));
    TestTrue(TEXT("New animation remains locked"), F->bAnimationBusy);
    F->CompleteAnimation(F->GetAnimationToken());
    return true;
}
#endif
