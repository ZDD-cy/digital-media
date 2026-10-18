#include "IndigoWorkbenchBridge.h"
#include "IndigoPatternSource.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoWorkbenchDropTest,"Indigo.Workbench.SelectionGuards",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoWorkbenchDropTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    AIndigoDirector* Director=World->SpawnActor<AIndigoDirector>();
    AIndigoWorkbenchBridge* Bridge=World->SpawnActor<AIndigoWorkbenchBridge>();
    AIndigoPatternSource* Source=World->SpawnActor<AIndigoPatternSource>();
    AIndigoInteractable* Left=World->SpawnActor<AIndigoInteractable>();
    AIndigoInteractable* Right=World->SpawnActor<AIndigoInteractable>();
    Bridge->Director=Director;
    Left->Kind=Right->Kind=EIndigoInteractionKind::DropTarget;
    Left->bLeftSlot=true;Right->bLeftSlot=false;
    UIndigoFlowComponent* Flow=Director->Flow;
    Flow->bUsePlaceholderTimer=false;
    TestFalse(TEXT("Walking mode cannot drop"),Bridge->TryDrop(Source,Left));
    Bridge->bWorkbenchActive=true;
    TestFalse(TEXT("Receipt phase cannot drop"),Bridge->TryDrop(Source,Left));
    Bridge->PerformAction(EIndigoAction::InspectCloth);Bridge->PerformAction(EIndigoAction::InspectOrder);
    TestFalse(TEXT("Empty selection cannot confirm"),Bridge->PerformAction(EIndigoAction::Confirm));
    TestTrue(TEXT("A into left"),Bridge->TryDrop(Source,Left));
    TestTrue(TEXT("Same A into right"),Bridge->TryDrop(Source,Right));
    TestTrue(TEXT("A/A retained"),Flow->Result.Left==EIndigoPattern::A && Flow->Result.Right==EIndigoPattern::A);
    Source->Pattern=EIndigoPattern::B;
    TestTrue(TEXT("Replace right with B"),Bridge->TryDrop(Source,Right));
    Source->DragGroup=TEXT("Wrong");
    TestFalse(TEXT("Wrong group rejected"),Bridge->TryDrop(Source,Left));
    Source->DragGroup=TEXT("Pattern");
    TestFalse(TEXT("Empty-space drop rejected"),Bridge->TryDrop(Source,nullptr));
    TestTrue(TEXT("Swap"),Bridge->PerformAction(EIndigoAction::Swap));
    TestTrue(TEXT("Swap changes shared result"),Flow->Result.Left==EIndigoPattern::B && Flow->Result.Right==EIndigoPattern::A);
    TestTrue(TEXT("Clear right"),Bridge->PerformAction(EIndigoAction::ClearRight));
    TestFalse(TEXT("One empty slot cannot confirm"),Bridge->PerformAction(EIndigoAction::Confirm));
    TestTrue(TEXT("Refill right"),Bridge->TryDrop(Source,Right));
    TestTrue(TEXT("Confirm complete pair"),Bridge->PerformAction(EIndigoAction::Confirm));
    TestFalse(TEXT("Cannot alter confirmed pair"),Bridge->TryDrop(Source,Left));
    Bridge->bWorkbenchActive=false;
    World->DestroyWorld(false);
    return true;
}
#endif
