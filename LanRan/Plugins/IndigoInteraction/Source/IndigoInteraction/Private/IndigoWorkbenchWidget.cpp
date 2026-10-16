#include "IndigoWorkbenchWidget.h"
#include "IndigoWorkbenchBridge.h"
#include "IndigoFlowComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "InputCoreTypes.h"

void UIndigoWorkbenchWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UCanvasPanel* Root=WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget=Root;
    Root->SetVisibility(ESlateVisibility::Visible);
    Panel=WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(FLinearColor(.015f,.035f,.08f,.92f));
    Panel->SetPadding(FMargin(16.f));
    UCanvasPanelSlot* PanelSlot=Root->AddChildToCanvas(Panel);
    PanelSlot->SetPosition(FVector2D(22,22)); PanelSlot->SetAutoSize(true);
    UVerticalBox* Column=WidgetTree->ConstructWidget<UVerticalBox>(); Panel->SetContent(Column);
    auto AddText=[this,Column](const TCHAR* Text,int32 Size)
    {
        UTextBlock* T=WidgetTree->ConstructWidget<UTextBlock>();
        T->SetText(FText::FromString(Text)); FSlateFontInfo Font=T->GetFont(); Font.Size=Size; T->SetFont(Font);
        T->SetWrapTextAt(380.f); Column->AddChildToVerticalBox(T); return T;
    };
    Title=AddText(TEXT("蓝染 · 选花版"),22);
    Status=AddText(TEXT(""),18); Help=AddText(TEXT(""),15);
    UHorizontalBox* Row1=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(Row1);
    UHorizontalBox* Row2=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(Row2);
    SelectionRow1=Row1; SelectionRow2=Row2;
    CraftRow1=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(CraftRow1);
    CraftRow2=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(CraftRow2);
    DyeRow=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(DyeRow);
    WashRow1=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(WashRow1);
    WashRow2=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(WashRow2);
    ResultRow=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(ResultRow);
    UHorizontalBox* CommonRow=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(CommonRow);
    auto Button=[this](UHorizontalBox* Row,const TCHAR* Text)
    {
        UButton* B=WidgetTree->ConstructWidget<UIndigoWorkbenchButton>();
        UTextBlock* T=WidgetTree->ConstructWidget<UTextBlock>(); T->SetText(FText::FromString(Text));
        FSlateFontInfo Font=T->GetFont();Font.Size=16;T->SetFont(Font);B->SetContent(T);
        Row->AddChildToHorizontalBox(B)->SetPadding(FMargin(3,8,3,0)); return B;
    };
    Cloth=Button(Row1,TEXT(" 查看布料 ")); Cloth->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnCloth);
    Order=Button(Row1,TEXT(" 查看订单 ")); Order->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnOrder);
    Swap=Button(Row1,TEXT(" 左右交换 ")); Swap->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnSwap);
    ClearLeft=Button(Row2,TEXT(" 清空左槽 ")); ClearLeft->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnClearLeft);
    ClearRight=Button(Row2,TEXT(" 清空右槽 ")); ClearRight->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnClearRight);
    Confirm=Button(Row2,TEXT(" 确认花版 ")); Confirm->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnConfirm);
    Fold=Button(CraftRow1,TEXT(" 沿当前折线折布 ")); Fold->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnFold);
    Place=Button(CraftRow1,TEXT(" 放布 ")); Place->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnPlace);
    Close=Button(CraftRow1,TEXT(" 合版 ")); Close->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnClose);
    ClampLeft=Button(CraftRow2,TEXT(" 扣左夹具 ")); ClampLeft->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnClampLeft);
    ClampRight=Button(CraftRow2,TEXT(" 扣右夹具 ")); ClampRight->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnClampRight);
    Dye=Button(DyeRow,TEXT(" 浸染一档 / 继续染色 ")); Dye->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnDye);
    FinishDye=Button(DyeRow,TEXT(" 收工 ")); FinishDye->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnFinishDye);
    Wash=Button(WashRow1,TEXT(" 清洗 ")); Wash->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnWash);
    Drain=Button(WashRow1,TEXT(" 提起沥水 ")); Drain->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnDrain);
    UnclampLeft=Button(WashRow2,TEXT(" 松左夹具 ")); UnclampLeft->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnUnclampLeft);
    UnclampRight=Button(WashRow2,TEXT(" 松右夹具 ")); UnclampRight->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnUnclampRight);
    Unfold=Button(WashRow2,TEXT(" 展开作品 ")); Unfold->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnUnfold);
    Deliver=Button(ResultRow,TEXT(" 交付作品 ")); Deliver->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnDeliver);
    Review=Button(ResultRow,TEXT(" 回看作品 ")); Review->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnReview);
    Back=Button(ResultRow,TEXT(" 返回完成页 ")); Back->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnBack);
    Button(CommonRow,TEXT(" 重新开始 "))->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnRestart);
    Button(CommonRow,TEXT(" 返回行走 E "))->OnClicked.AddDynamic(this,&UIndigoWorkbenchWidget::OnExit);
    Hint=WidgetTree->ConstructWidget<UTextBlock>();Hint->SetText(FText::FromString(TEXT("按 E 开始制作")));
    FSlateFontInfo HintFont=Hint->GetFont();HintFont.Size=25;Hint->SetFont(HintFont);
    Hint->SetShadowOffset(FVector2D(2,2));Hint->SetShadowColorAndOpacity(FLinearColor::Black);
    UCanvasPanelSlot* HintSlot=Root->AddChildToCanvas(Hint);
    HintSlot->SetAnchors(FAnchors(.5f,.84f));HintSlot->SetAlignment(FVector2D(.5f,.5f));HintSlot->SetAutoSize(true);
    Refresh();
}
bool UIndigoWorkbenchWidget::IsPointerOverPanel() const { return Panel && Panel->GetVisibility()==ESlateVisibility::Visible && Panel->IsHovered(); }
void UIndigoWorkbenchWidget::Refresh()
{
    if (!Panel || !Bridge) return;
    const bool Active=Bridge->bWorkbenchActive;
    Panel->SetVisibility(Active?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Hint->SetVisibility(!Active && Bridge->IsPlayerNear()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    UIndigoFlowComponent* F=Bridge->GetFlow(); if (!Active || !F) return;
    const auto Pattern=[](EIndigoPattern P){return P==EIndigoPattern::A?TEXT("A"):P==EIndigoPattern::B?TEXT("B"):P==EIndigoPattern::C?TEXT("C"):TEXT("未选择");};
    Status->SetText(FText::FromString(FString::Printf(TEXT("左槽：%s     右槽：%s%s"),Pattern(F->Result.Left),Pattern(F->Result.Right),
        F->Step==EIndigoStep::Dye || F->Step==EIndigoStep::Wash || F->Step==EIndigoStep::Reveal || F->Step==EIndigoStep::End?*FString::Printf(TEXT("\n染色深度：%d / 4 档"),F->Result.Depth):TEXT(""))));
    const bool Selection=F->Step==EIndigoStep::Receive || F->Step==EIndigoStep::Select;
    const bool Craft=F->Step==EIndigoStep::Fold || F->Step==EIndigoStep::Clamp;
    SelectionRow1->SetVisibility(Selection?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    SelectionRow2->SetVisibility(Selection?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    CraftRow1->SetVisibility(Craft?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    CraftRow2->SetVisibility(Craft?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    DyeRow->SetVisibility(F->Step==EIndigoStep::Dye?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    WashRow1->SetVisibility(F->Step==EIndigoStep::Wash?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    WashRow2->SetVisibility(F->Step==EIndigoStep::Wash?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    const bool Final=F->Step==EIndigoStep::Reveal || F->Step==EIndigoStep::End;
    ResultRow->SetVisibility(Final?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Deliver->SetVisibility(F->Step==EIndigoStep::Reveal && !F->IsReview()?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Review->SetVisibility(F->Step==EIndigoStep::End?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Back->SetVisibility(F->IsReview()?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Title->SetText(FText::FromString(Selection?TEXT("蓝染 · 选花版"):F->Step==EIndigoStep::Fold?TEXT("蓝染 · 折布"):F->Step==EIndigoStep::Clamp?TEXT("蓝染 · 合版扣夹"):TEXT("蓝染 · 制作")));
    FString Instruction;
    switch(F->Step)
    {
    case EIndigoStep::Receive: Instruction=TEXT("先查看布料和订单，然后选择花版。"); break;
    case EIndigoStep::Select: Instruction=TEXT("将 A/B/C 拖到左右板子；可重复选同款。\n右键取消拖动，按 E 退出。"); break;
    case EIndigoStep::Fold:
        Instruction=FString::Printf(TEXT("折布：已完成 %d / %d 折。\n%s\n当前使用演示布料与夹具。"),F->FoldIndex,F->FoldCount,F->bAnimationBusy?TEXT("正在折布，请等动作结束……"):TEXT("点击“沿当前折线折布”，依次完成三折。")); break;
    case EIndigoStep::Clamp:
        Instruction=F->bAnimationBusy?TEXT("动作进行中，请等动作结束……"):F->CanPerform(EIndigoAction::PlaceCloth)?TEXT("三折完成，将布放到下木版上。"):F->CanPerform(EIndigoAction::ClosePlate)?TEXT("布已放好，合上木版。"):TEXT("木版已合上，分别扣紧左右夹具。\n已扣好的夹具不能重复操作。"); break;
    case EIndigoStep::Dye:
        Instruction=F->bAnimationBusy?TEXT("浸染与出缸显色中，请等整段动作结束……\n完成后才计入一档，不可连点跳过。"):
            F->Result.Depth==0?TEXT("左右夹具已扣紧，开始第一档浸染。\n至少完成一档才能收工，最多四档。"):
            F->Result.Depth==4?TEXT("已到第四档，请点击收工。\n下方色样为深度预览，当前使用演示染缸。"):
            TEXT("本档已完成。可继续加深，或点击收工。\n下方色样为深度预览，当前使用演示染缸。"); break;
    case EIndigoStep::Wash:
        Instruction=F->bAnimationBusy?TEXT("动作进行中，请等结束……"):
            F->CanPerform(EIndigoAction::Wash)?TEXT("先清洗夹版中的染布，再提起沥水。"):
            F->CanPerform(EIndigoAction::Drain)?TEXT("清洗完成，提起夹版沥水。"):
            F->CanPerform(EIndigoAction::Unfold)?TEXT("两侧夹具已松开，可以展开作品。"):
            TEXT("沥水完成，松开左右夹具，顺序不限。"); break;
    case EIndigoStep::Reveal: Instruction=F->bAnimationBusy?TEXT("正在交付作品……"):
        F->IsReview()?TEXT("回看本局作品，花版与深度保持不变。"):
        TEXT("作品已展开，可停留欣赏，再点击交付。\n当前为几何纹样演示，正式花纹待替换。"); break;
    case EIndigoStep::End: Instruction=TEXT("本次制作完成。可回看作品或重新开始。"); break;
    default: Instruction=TEXT("按 E 返回第一人称。"); break;
    }
    Help->SetText(FText::FromString(Instruction));
    Cloth->SetIsEnabled(F->CanPerform(EIndigoAction::InspectCloth)); Order->SetIsEnabled(F->CanPerform(EIndigoAction::InspectOrder));
    Swap->SetIsEnabled(F->CanPerform(EIndigoAction::Swap)); ClearLeft->SetIsEnabled(F->CanPerform(EIndigoAction::ClearLeft));
    ClearRight->SetIsEnabled(F->CanPerform(EIndigoAction::ClearRight));Confirm->SetIsEnabled(F->CanPerform(EIndigoAction::Confirm));
    Fold->SetIsEnabled(F->CanPerform(EIndigoAction::Fold,F->FoldIndex));
    Place->SetIsEnabled(F->CanPerform(EIndigoAction::PlaceCloth)); Close->SetIsEnabled(F->CanPerform(EIndigoAction::ClosePlate));
    ClampLeft->SetIsEnabled(F->CanPerform(EIndigoAction::ClampLeft)); ClampRight->SetIsEnabled(F->CanPerform(EIndigoAction::ClampRight));
    Dye->SetIsEnabled(F->CanPerform(EIndigoAction::Dye)); FinishDye->SetIsEnabled(F->CanPerform(EIndigoAction::FinishDye));
    Wash->SetIsEnabled(F->CanPerform(EIndigoAction::Wash)); Drain->SetIsEnabled(F->CanPerform(EIndigoAction::Drain));
    UnclampLeft->SetIsEnabled(F->CanPerform(EIndigoAction::UnclampLeft)); UnclampRight->SetIsEnabled(F->CanPerform(EIndigoAction::UnclampRight));
    Unfold->SetIsEnabled(F->CanPerform(EIndigoAction::Unfold)); Deliver->SetIsEnabled(F->CanPerform(EIndigoAction::Deliver));
    Review->SetIsEnabled(F->CanPerform(EIndigoAction::Review)); Back->SetIsEnabled(F->CanPerform(EIndigoAction::Back));
}
void UIndigoWorkbenchWidget::OnCloth(){if(Bridge)Bridge->PerformAction(EIndigoAction::InspectCloth);}
void UIndigoWorkbenchWidget::OnOrder(){if(Bridge)Bridge->PerformAction(EIndigoAction::InspectOrder);}
void UIndigoWorkbenchWidget::OnSwap(){if(Bridge)Bridge->PerformAction(EIndigoAction::Swap);}
void UIndigoWorkbenchWidget::OnClearLeft(){if(Bridge)Bridge->PerformAction(EIndigoAction::ClearLeft);}
void UIndigoWorkbenchWidget::OnClearRight(){if(Bridge)Bridge->PerformAction(EIndigoAction::ClearRight);}
void UIndigoWorkbenchWidget::OnConfirm(){if(Bridge)Bridge->PerformAction(EIndigoAction::Confirm);}
void UIndigoWorkbenchWidget::OnExit(){if(Bridge)Bridge->LeaveWorkbench();}
void UIndigoWorkbenchWidget::OnFold(){if(Bridge && Bridge->GetFlow())Bridge->PerformAction(EIndigoAction::Fold,Bridge->GetFlow()->FoldIndex);}
void UIndigoWorkbenchWidget::OnPlace(){if(Bridge)Bridge->PerformAction(EIndigoAction::PlaceCloth);}
void UIndigoWorkbenchWidget::OnClose(){if(Bridge)Bridge->PerformAction(EIndigoAction::ClosePlate);}
void UIndigoWorkbenchWidget::OnClampLeft(){if(Bridge)Bridge->PerformAction(EIndigoAction::ClampLeft);}
void UIndigoWorkbenchWidget::OnClampRight(){if(Bridge)Bridge->PerformAction(EIndigoAction::ClampRight);}
void UIndigoWorkbenchWidget::OnRestart(){if(Bridge)Bridge->RestartWorkbench();}
void UIndigoWorkbenchWidget::OnDye(){if(Bridge)Bridge->PerformAction(EIndigoAction::Dye);}
void UIndigoWorkbenchWidget::OnFinishDye(){if(Bridge)Bridge->PerformAction(EIndigoAction::FinishDye);}
void UIndigoWorkbenchWidget::OnWash(){if(Bridge)Bridge->PerformAction(EIndigoAction::Wash);}
void UIndigoWorkbenchWidget::OnDrain(){if(Bridge)Bridge->PerformAction(EIndigoAction::Drain);}
void UIndigoWorkbenchWidget::OnUnclampLeft(){if(Bridge)Bridge->PerformAction(EIndigoAction::UnclampLeft);}
void UIndigoWorkbenchWidget::OnUnclampRight(){if(Bridge)Bridge->PerformAction(EIndigoAction::UnclampRight);}
void UIndigoWorkbenchWidget::OnUnfold(){if(Bridge)Bridge->PerformAction(EIndigoAction::Unfold);}
void UIndigoWorkbenchWidget::OnDeliver(){if(Bridge)Bridge->PerformAction(EIndigoAction::Deliver);}
void UIndigoWorkbenchWidget::OnReview(){if(Bridge)Bridge->PerformAction(EIndigoAction::Review);}
void UIndigoWorkbenchWidget::OnBack(){if(Bridge)Bridge->PerformAction(EIndigoAction::Back);}

FReply UIndigoWorkbenchWidget::NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if (Bridge && Bridge->bWorkbenchActive && !IsPointerOverPanel())
    {
        if (Event.GetEffectingButton()==EKeys::RightMouseButton) { Bridge->CancelDrag(); return FReply::Handled().ReleaseMouseCapture(); }
        if (Event.GetEffectingButton()==EKeys::LeftMouseButton)
        {
            FVector2D Pixel,Viewport;
            USlateBlueprintLibrary::AbsoluteToViewport(this,Event.GetScreenSpacePosition(),Pixel,Viewport);
            if (Bridge->PointerDown(Pixel)) return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    return Super::NativeOnMouseButtonDown(Geometry,Event);
}
FReply UIndigoWorkbenchWidget::NativeOnMouseButtonUp(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if (Bridge && Bridge->bWorkbenchActive && Event.GetEffectingButton()==EKeys::LeftMouseButton)
    {
        FVector2D Pixel,Viewport;
        USlateBlueprintLibrary::AbsoluteToViewport(this,Event.GetScreenSpacePosition(),Pixel,Viewport);
        Bridge->PointerUp(Pixel);
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(Geometry,Event);
}
FReply UIndigoWorkbenchWidget::NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if (Bridge && Bridge->bWorkbenchActive)
    {
        FVector2D Pixel,Viewport;
        USlateBlueprintLibrary::AbsoluteToViewport(this,Event.GetScreenSpacePosition(),Pixel,Viewport);
        Bridge->PointerMove(Pixel);
    }
    return Super::NativeOnMouseMove(Geometry,Event);
}
void UIndigoWorkbenchWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    if (Bridge) Bridge->CancelDrag();
    Super::NativeOnMouseCaptureLost(Event);
}

