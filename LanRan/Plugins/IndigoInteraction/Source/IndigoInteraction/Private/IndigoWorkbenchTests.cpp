#include "IndigoWorkbenchBridge.h"
#include "IndigoPatternSource.h"
#include "IndigoDirector.h"
#include "IndigoFlowComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "IndigoCraftPreview.h"

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoSlotVisualTest,"Indigo.Workbench.SlotFollowsVisual",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoSlotVisualTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    AStaticMeshActor* Board=World->SpawnActor<AStaticMeshActor>();
    UStaticMeshComponent* Mesh=Board->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetWorldTransform(FTransform(FRotator(0,35,0),FVector(400,-250,120),FVector(2,.5,.1)));
    AIndigoInteractable* Slot=World->SpawnActor<AIndigoInteractable>();
    Slot->Kind=EIndigoInteractionKind::DropTarget;Slot->SlotVisual=Mesh;
    Slot->AlignSlotToVisual();
    TestTrue(TEXT("Slot uses visual component position"),Slot->GetActorLocation().Equals(Mesh->GetComponentLocation(),.01f));
    TestTrue(TEXT("Slot follows visual rotation"),Slot->GetActorQuat().Equals(Mesh->GetComponentQuat(),.001f));
    TestTrue(TEXT("Slot padding stays generous on a scaled board"),Slot->HitBox->GetScaledBoxExtent().Equals(FVector(128,53,25),.01f));
    Mesh->SetWorldLocation(FVector(700,-300,185));Slot->AlignSlotToVisual();
    TestTrue(TEXT("Animated movement cannot leave stale drop position"),Slot->GetActorLocation().Equals(FVector(700,-300,185),.01f));
    World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoAuthoredAppearanceTest,"Indigo.Workbench.AuthoredAppearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoAuthoredAppearanceTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Director=World->SpawnActor<AIndigoDirector>();
    auto* Flow=Director->Flow.Get();Flow->bSkipReceipt=true;Flow->Restart();
    TestTrue(TEXT("Removed receipt starts in selection"),Flow->Step==EIndigoStep::Select);
    auto* Source=World->SpawnActor<AIndigoPatternSource>();
    Source->Pattern=EIndigoPattern::B;
    Source->BoardMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));
    auto* Board=World->SpawnActor<AStaticMeshActor>();
    auto* Mesh=Board->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);
    auto* Original=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));Mesh->SetStaticMesh(Original);
    Mesh->SetWorldRotation(FRotator(0,-90,0));
    auto* Slot=World->SpawnActor<AIndigoInteractable>();Slot->Kind=EIndigoInteractionKind::DropTarget;Slot->SlotVisual=Mesh;
    Slot->ApplySelectedPattern(Flow);
    Flow->SetPattern(true,EIndigoPattern::B);Slot->ApplySelectedPattern(Flow);
    TestTrue(TEXT("Selection replaces the actual displayed mesh"),Mesh->GetStaticMesh()==Source->BoardMesh->GetStaticMesh());
    TestTrue(TEXT("Selection preserves authored minus 90 yaw"),Mesh->GetComponentQuat().Equals(FRotator(0,-90,0).Quaternion(),.001f));
    Flow->SetPattern(true,EIndigoPattern::None);Slot->ApplySelectedPattern(Flow);
    TestTrue(TEXT("Clearing restores original slot mesh"),Mesh->GetStaticMesh()==Original);
    const float Expected[]={4.f,1.f,.1f,.03f};
    for(int32 I=0;I<4;++I) TestEqual(TEXT("Authored dye depth"),AIndigoCraftPreview::ColorDepthForStage(I+1),Expected[I]);
    Flow->Restart();TestTrue(TEXT("Restart also skips removed receipt"),Flow->Step==EIndigoStep::Select);
    World->DestroyWorld(false);return true;
}
#endif

