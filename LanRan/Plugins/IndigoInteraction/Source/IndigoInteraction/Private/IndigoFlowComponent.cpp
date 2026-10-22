#include "IndigoFlowComponent.h"
#include "IndigoVisualInterface.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UIndigoFlowComponent::UIndigoFlowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    const auto Add = [this](EIndigoStep S, const TCHAR* Text, EIndigoCamera Camera)
    {
        FIndigoStepConfig C; C.Subtitle = FText::FromString(Text); C.Camera = Camera; StepConfig.Add(S, C);
    };
    Add(EIndigoStep::Receive, TEXT("先看看旧布角和委托单。"), EIndigoCamera::Table);
    Add(EIndigoStep::Select, TEXT("把花版拖到左右区域，确认前可以交换或重选。"), EIndigoCamera::Table);
    Add(EIndigoStep::Fold, TEXT("沿当前折线叠布，等这一折完成。"), EIndigoCamera::Closeup);
    Add(EIndigoStep::Clamp, TEXT("放布、合版，再扣紧左右夹具。"), EIndigoCamera::Closeup);
    Add(EIndigoStep::Dye, TEXT("完成一段浸染和显色后，可以继续或收工。"), EIndigoCamera::Table);
    Add(EIndigoStep::Wash, TEXT("清洗、沥水、松开两侧夹具，然后展开。"), EIndigoCamera::Closeup);
    Add(EIndigoStep::Reveal, TEXT("看看自己的作品，准备好了再交付。"), EIndigoCamera::Quilt);
    Add(EIndigoStep::End, TEXT("可以回看作品，或者重新开始。"), EIndigoCamera::Quilt);
    Add(EIndigoStep::Trial,TEXT("先舀些染液，浸入试布，提出后等它显色。"),EIndigoCamera::Closeup);
    Add(EIndigoStep::Delivery,TEXT("原来，这床被面是要送给她即将成婚的女儿。读完委托，再把它寄出。"),EIndigoCamera::Quilt);
}

bool UIndigoFlowComponent::CanPerform(EIndigoAction A, int32 Index) const
{
    if (IsInputLocked()) return false;
    switch (Step)
    {
    case EIndigoStep::Receive:
        return (A == EIndigoAction::InspectCloth && !bClothSeen) || (A == EIndigoAction::InspectOrder && !bOrderSeen);
    case EIndigoStep::Select:
        return A == EIndigoAction::Swap || A == EIndigoAction::ClearLeft || A == EIndigoAction::ClearRight ||
            (A == EIndigoAction::Confirm && Result.Left != EIndigoPattern::None && Result.Right != EIndigoPattern::None);
    case EIndigoStep::Fold: return A == EIndigoAction::Fold && Index == FoldIndex;
    case EIndigoStep::Clamp:
        return (A == EIndigoAction::PlaceCloth && !bPlaced) ||
            (A == EIndigoAction::ClosePlate && bPlaced && !bPlateClosed) ||
            (A == EIndigoAction::ClampLeft && bPlateClosed && !bLeftClamp) ||
            (A == EIndigoAction::ClampRight && bPlateClosed && !bRightClamp);
    case EIndigoStep::Dye:
        return (A == EIndigoAction::Dye && Result.Depth < 4 && (!bEnableTemperatureRules || Temperature==EIndigoTemperature::Ready)) ||
            (A==EIndigoAction::AddFuel && bEnableTemperatureRules && Temperature==EIndigoTemperature::Cold && Fuel>0 && Result.Depth<4) ||
            (A == EIndigoAction::FinishDye && Result.Depth > 0);
    case EIndigoStep::Trial:
        return (A==EIndigoAction::AddFuel && bColdTrialSeen && TrialIndex==0 && Temperature==EIndigoTemperature::Cold && Fuel>0) ||
            (A==EIndigoAction::ScoopTest && TrialIndex==0 && (!bColdTrialSeen || Temperature==EIndigoTemperature::Ready)) ||
            (A==EIndigoAction::DipTest && TrialIndex==1) || (A==EIndigoAction::LiftTest && TrialIndex==2) ||
            (A==EIndigoAction::OxidizeTest && TrialIndex==3);
    case EIndigoStep::Wash:
        return (A == EIndigoAction::Wash && WashPhase == 0) || (A == EIndigoAction::Drain && WashPhase == 1) ||
            (A == EIndigoAction::UnclampLeft && WashPhase == 2 && bLeftClamp) ||
            (A == EIndigoAction::UnclampRight && WashPhase == 2 && bRightClamp) ||
            (A == EIndigoAction::Unfold && WashPhase == 2 && !bLeftClamp && !bRightClamp);
    case EIndigoStep::Reveal: return bReview ? A == EIndigoAction::Back : A == EIndigoAction::Deliver;
    case EIndigoStep::End: return A == EIndigoAction::Review;
    case EIndigoStep::Delivery: return A==EIndigoAction::Ship && GetDeliveryReadingRemaining()<=0.f;
    default: return false;
    }
}

bool UIndigoFlowComponent::SetPattern(bool bLeft, EIndigoPattern Pattern)
{
    if (IsInputLocked() || Step != EIndigoStep::Select || static_cast<uint8>(Pattern) > static_cast<uint8>(EIndigoPattern::C)) return false;
    (bLeft ? Result.Left : Result.Right) = Pattern;
    PublishResult(); return true;
}

bool UIndigoFlowComponent::Perform(EIndigoAction A, int32 Index)
{
    if (!CanPerform(A, Index)) return false;
    switch (A)
    {
    case EIndigoAction::InspectCloth: bClothSeen = true; break;
    case EIndigoAction::InspectOrder: bOrderSeen = true; break;
    case EIndigoAction::Swap: Swap(Result.Left, Result.Right); PublishResult(); break;
    case EIndigoAction::ClearLeft: return SetPattern(true, EIndigoPattern::None);
    case EIndigoAction::ClearRight: return SetPattern(false, EIndigoPattern::None);
    case EIndigoAction::Confirm: Enter(bEnableTemperatureRules?EIndigoStep::Trial:EIndigoStep::Fold); break;
    case EIndigoAction::AddFuel: --Fuel; StartAnimation(A,TEXT("AddFuel")); break;
    case EIndigoAction::Deliver:
        if(bEnableDeliveryStory) Enter(EIndigoStep::Delivery);
        else StartAnimation(A,TEXT("Deliver"));
        break;
    case EIndigoAction::FinishDye: Enter(EIndigoStep::Wash); break;
    case EIndigoAction::Review: bReview = true; Enter(EIndigoStep::Reveal); break;
    case EIndigoAction::Back: bReview = false; Enter(EIndigoStep::End); break;
    default:
        StartAnimation(A, FName(*StaticEnum<EIndigoAction>()->GetNameStringByValue(static_cast<int64>(A))));
        break;
    }
    if (Step == EIndigoStep::Receive && bClothSeen && bOrderSeen) Enter(EIndigoStep::Select);
    return true;
}

void UIndigoFlowComponent::StartAnimation(EIndigoAction Action, FName Cue)
{
    Pending = Action; ActiveToken = ++Serial; bAnimationBusy = true;
    const int32 Token = ActiveToken;
    OnInteractionInvalidated.Broadcast();
    if (bUsePlaceholderTimer && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(AnimationTimer,
            FTimerDelegate::CreateWeakLambda(this, [this, Token]() { CompleteAnimation(Token); }),
            FMath::Max(0.05f, PlaceholderSeconds), false);
    }
    // Events may complete synchronously; iterate a copy and re-check the token.
    const auto Targets = VisualTargets;
    for (AActor* Target : Targets)
    {
        if (!bAnimationBusy || ActiveToken != Token) break;
        if (IsValid(Target) && Target->Implements<UIndigoVisualInterface>())
            IIndigoVisualInterface::Execute_PlayIndigoAnimation(Target, Cue, Token);
    }
    if (bAnimationBusy && ActiveToken == Token) OnAnimationRequested.Broadcast(Cue, Token);
}

bool UIndigoFlowComponent::CompleteAnimation(int32 Token)
{
    if (!bAnimationBusy || Token != ActiveToken) return false;
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(AnimationTimer);
    bAnimationBusy = false; ActiveToken = 0;
    switch (Pending)
    {
    case EIndigoAction::Fold: if (++FoldIndex >= FMath::Max(1, FoldCount)) Enter(EIndigoStep::Clamp); break;
    case EIndigoAction::PlaceCloth: bPlaced = true; break;
    case EIndigoAction::ClosePlate: bPlateClosed = true; break;
    case EIndigoAction::ClampLeft: bLeftClamp = true; break;
    case EIndigoAction::ClampRight: bRightClamp = true; break;
    case EIndigoAction::Dye:
        if(bEnableTemperatureRules) Temperature=EIndigoTemperature::Cold;
        Result.Depth = FMath::Min(4, Result.Depth + 1); PublishResult();
        OnSubtitle.Broadcast(FText::FromString(Result.Depth == 4 ? TEXT("已经到第四档，点击收工。") : bEnableTemperatureRules?TEXT("这一档显色了。若还想更深，先添一份谷壳补温；也可以就此收工。"):TEXT("这一档显色完成，可以继续或收工。")));
        break;
    case EIndigoAction::AddFuel:
        Temperature=EIndigoTemperature::Ready; OnSubtitle.Broadcast(FText::FromString(Step==EIndigoStep::Trial?TEXT("缸温回来了，再试一次，看看前后的颜色。 "):TEXT("补温好了，可以继续入染。"))); break;
    case EIndigoAction::ScoopTest: TrialIndex=1;break;
    case EIndigoAction::DipTest: TrialIndex=2;break;
    case EIndigoAction::LiftTest: TrialIndex=3;break;
    case EIndigoAction::OxidizeTest:
        TrialIndex=0;
        if(Temperature==EIndigoTemperature::Ready) { bWarmTrialSeen=true;Enter(EIndigoStep::Fold); }
        else { bColdTrialSeen=true;OnSubtitle.Broadcast(FText::FromString(TEXT("这条试布上色偏淡，是缸温不足。添一份谷壳，等补温后再试。"))); }
        break;
    case EIndigoAction::Wash: WashPhase = 1; break;
    case EIndigoAction::Drain: WashPhase = 2; break;
    case EIndigoAction::UnclampLeft: bLeftClamp = false; break;
    case EIndigoAction::UnclampRight: bRightClamp = false; break;
    case EIndigoAction::Unfold: Enter(EIndigoStep::Reveal); break;
    case EIndigoAction::Deliver: Enter(EIndigoStep::End); break;
    case EIndigoAction::Ship: Enter(EIndigoStep::End); break;
    default: break;
    }
    if (Step == EIndigoStep::Clamp && bLeftClamp && bRightClamp) Enter(EIndigoStep::Dye);
    return true;
}

bool UIndigoFlowComponent::ReportDyeProgress(int32 Token, float Progress)
{
    if (!bAnimationBusy || Token != ActiveToken || Pending != EIndigoAction::Dye || !FMath::IsFinite(Progress)) return false;
    for (AActor* Target : VisualTargets)
        if (IsValid(Target) && Target->Implements<UIndigoVisualInterface>())
            IIndigoVisualInterface::Execute_SetDyeProgress(Target, Result.Depth, FMath::Clamp(Progress, 0.f, 1.f));
    return true;
}

void UIndigoFlowComponent::Enter(EIndigoStep Next)
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(AnimationTimer);
    ++Serial; ActiveToken = 0; bAnimationBusy = false; Step = Next;
    if(Next==EIndigoStep::Delivery) DeliveryOpenedAt=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
    OnInteractionInvalidated.Broadcast();
    OnStepChanged.Broadcast(Step);
    if (const FIndigoStepConfig* C = StepConfig.Find(Step)) OnSubtitle.Broadcast(C->Subtitle);
    RefreshVisuals();
}
float UIndigoFlowComponent::GetDeliveryReadingRemaining() const
{
    if(Step!=EIndigoStep::Delivery || !bEnableDeliveryStory) return 0.f;
    return FMath::Max(0.f,DeliveryReadingSeconds-((GetWorld()?GetWorld()->GetTimeSeconds():0.f)-DeliveryOpenedAt));
}

void UIndigoFlowComponent::SetCameraBusy(bool bBusy)
{
    bCameraBusy = bBusy;
    if (bBusy) OnInteractionInvalidated.Broadcast();
}

void UIndigoFlowComponent::PublishResult() { RefreshVisuals(); OnResultChanged.Broadcast(Result); }

void UIndigoFlowComponent::RefreshVisuals()
{
    for (AActor* Target : VisualTargets)
    {
        if (!IsValid(Target) || !Target->Implements<UIndigoVisualInterface>()) continue;
        IIndigoVisualInterface::Execute_SetPattern(Target, Result.Left, Result.Right);
        IIndigoVisualInterface::Execute_SetDyeProgress(Target, Result.Depth, 0.f);
        if (Step == EIndigoStep::Reveal || Step == EIndigoStep::End || Step==EIndigoStep::Delivery)
            IIndigoVisualInterface::Execute_ShowFinalQuilt(Target, Result);
    }
}

void UIndigoFlowComponent::Restart()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(AnimationTimer);
    ++Serial; ActiveToken = 0; bAnimationBusy = false; bCameraBusy = false;
    // Restore the drag origin before the scene's ResetVisuals restores its initial pose.
    OnInteractionInvalidated.Broadcast();
    bClothSeen = bOrderSeen = bPlaced = bPlateClosed = bLeftClamp = bRightClamp = bReview = false;
    FoldIndex = 0; WashPhase = 0; Result = FIndigoResult();
    Temperature=EIndigoTemperature::Cold;Fuel=5;TrialIndex=0;bColdTrialSeen=bWarmTrialSeen=false;
    DeliveryOpenedAt=0.f;
    for (AActor* Target : VisualTargets)
        if (IsValid(Target) && Target->Implements<UIndigoVisualInterface>()) IIndigoVisualInterface::Execute_ResetVisuals(Target);
    Enter(bSkipReceipt ? EIndigoStep::Select : EIndigoStep::Receive); OnResultChanged.Broadcast(Result);
}

void UIndigoFlowComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(AnimationTimer);
    ++Serial; ActiveToken = 0; bAnimationBusy = false;
    Super::EndPlay(Reason);
}
