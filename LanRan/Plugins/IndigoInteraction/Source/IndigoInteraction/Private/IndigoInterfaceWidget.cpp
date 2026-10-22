#include "IndigoInterfaceWidget.h"

#include "IndigoFlowComponent.h"
#include "IndigoWorkbenchBridge.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

namespace
{
    const FLinearColor Navy(0.025f, 0.13f, 0.25f, 1.f);
    const FLinearColor DeepNavy(0.012f, 0.07f, 0.15f, 1.f);
    const FLinearColor Cream(0.92f, 0.90f, 0.84f, 1.f);
    const FLinearColor Paper(0.98f, 0.975f, 0.94f, 0.96f);
    const FLinearColor Ink(0.07f, 0.15f, 0.24f, 1.f);
    const FLinearColor MutedInk(0.20f, 0.27f, 0.33f, 1.f);
    const FLinearColor Bronze(0.68f, 0.53f, 0.31f, 1.f);
    const FLinearColor PaleBlue(0.78f, 0.82f, 0.85f, 1.f);

    FSlateFontInfo UiFont(const int32 Size, const bool bBold = false)
    {
        return FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size);
    }

    FSlateBrush BoxBrush(const FLinearColor Fill, const FLinearColor Outline, const float OutlineWidth = 1.f)
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Box;
        Brush.TintColor = Fill;
        Brush.OutlineSettings.Color = Outline;
        Brush.OutlineSettings.Width = OutlineWidth;
        return Brush;
    }

    FButtonStyle ButtonStyle(
        const FLinearColor Normal,
        const FLinearColor Hovered,
        const FLinearColor Pressed,
        const FLinearColor Outline)
    {
        FButtonStyle Style;
        Style.Normal = BoxBrush(Normal, Outline);
        Style.Hovered = BoxBrush(Hovered, Outline, 2.f);
        Style.Pressed = BoxBrush(Pressed, Outline, 2.f);
        Style.Disabled = BoxBrush(FLinearColor(.82f, .82f, .79f, .78f), FLinearColor(.70f, .70f, .67f, .5f));
        Style.NormalPadding = FMargin(0.f);
        Style.PressedPadding = FMargin(0.f);
        return Style;
    }

    UTextBlock* MakeText(
        UWidgetTree* Tree,
        const FString& Value,
        const int32 Size,
        const FLinearColor Color = Ink,
        const bool bBold = false)
    {
        UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Value));
        Text->SetFont(UiFont(Size, bBold));
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetAutoWrapText(true);
        return Text;
    }

    void FillCanvas(UCanvasPanel* Canvas, UWidget* Child, const int32 ZOrder = 0)
    {
        UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Child);
        Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        Slot->SetOffsets(FMargin(0.f));
        Slot->SetZOrder(ZOrder);
    }

    void AddCanvas(
        UCanvasPanel* Canvas,
        UWidget* Child,
        const FAnchors Anchors,
        const FVector2D Alignment,
        const FVector2D Position,
        const FVector2D Size,
        const bool bAutoSize = false,
        const int32 ZOrder = 0)
    {
        UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Child);
        Slot->SetAnchors(Anchors);
        Slot->SetAlignment(Alignment);
        Slot->SetPosition(Position);
        Slot->SetSize(Size);
        Slot->SetAutoSize(bAutoSize);
        Slot->SetZOrder(ZOrder);
    }

    const TCHAR* PatternText(const EIndigoPattern Pattern)
    {
        switch (Pattern)
        {
        case EIndigoPattern::A: return TEXT("A");
        case EIndigoPattern::B: return TEXT("B");
        case EIndigoPattern::C: return TEXT("C");
        default: return TEXT("未选择");
        }
    }

    const TCHAR* PatternAsset(const EIndigoPattern Pattern)
    {
        switch (Pattern)
        {
        case EIndigoPattern::A: return TEXT("/Game/UI/Textures/T_Mask_A_Blue.T_Mask_A_Blue");
        case EIndigoPattern::B: return TEXT("/Game/UI/Textures/T_Mask_B_Blue.T_Mask_B_Blue");
        case EIndigoPattern::C: return TEXT("/Game/UI/Textures/T_Mask_C_Blue.T_Mask_C_Blue");
        default: return TEXT("");
        }
    }

    void SetPatternImage(UImage* Image, const EIndigoPattern Pattern)
    {
        if (!Image) return;
        UTexture2D* Texture = Pattern == EIndigoPattern::None
            ? nullptr
            : LoadObject<UTexture2D>(nullptr, PatternAsset(Pattern));
        if (Texture)
        {
            Image->SetBrushFromTexture(Texture, false);
            Image->SetColorAndOpacity(FLinearColor::White);
        }
        else
        {
            Image->SetBrush(BoxBrush(Pattern == EIndigoPattern::None ? FLinearColor(.88f, .87f, .82f, .8f) : Navy, FLinearColor::Transparent));
            Image->SetColorAndOpacity(FLinearColor::White);
        }
    }
}

void UIndigoInterfaceWidget::SetBridge(AIndigoWorkbenchBridge* InBridge)
{
    Bridge = InBridge;
    Refresh();
}

void UIndigoInterfaceWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);

    Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("IndigoUiRoot"));
    WidgetTree->RootWidget = Root;
    Root->SetVisibility(ESlateVisibility::Visible);

    BuildStartScreen();
    BuildGameScreen();
    Refresh();
}

void UIndigoInterfaceWidget::BuildStartScreen()
{
    StartScreen = WidgetTree->ConstructWidget<UBorder>();
    StartScreen->SetBrushColor(Cream);
    StartScreen->SetPadding(FMargin(0.f));
    FillCanvas(Root, StartScreen, 10);

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    StartScreen->SetContent(Canvas);

    UBorder* Sidebar = WidgetTree->ConstructWidget<UBorder>();
    Sidebar->SetBrushColor(Navy);
    AddCanvas(Canvas, Sidebar, FAnchors(0.f, 0.f, 0.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(88.f, 0.f));

    UTextBlock* SidebarTitle = MakeText(WidgetTree, TEXT("蓝染工房"), 14, FLinearColor(0.82f, 0.86f, 0.86f, 1.f));
    AddCanvas(Canvas, SidebarTitle, FAnchors(0.f, 0.f), FVector2D::ZeroVector, FVector2D(12.f, 18.f), FVector2D(74.f, 30.f), true);

    StartLogo = MakeText(WidgetTree, TEXT("蓝  染"), 96, Navy, true);
    StartLogo->SetJustification(ETextJustify::Center);
    AddCanvas(Canvas, StartLogo, FAnchors(.5f, .42f), FVector2D(.5f, .5f), FVector2D(0.f, -150.f), FVector2D(620.f, 150.f));

    StartSubtitle = MakeText(WidgetTree, TEXT("手 工 染 坊"), 17, MutedInk);
    StartSubtitle->SetJustification(ETextJustify::Center);
    AddCanvas(Canvas, StartSubtitle, FAnchors(.5f, .42f), FVector2D(.5f, .5f), FVector2D(0.f, -46.f), FVector2D(300.f, 32.f));

    UBorder* Rule = WidgetTree->ConstructWidget<UBorder>();
    Rule->SetBrushColor(Navy);
    AddCanvas(Canvas, Rule, FAnchors(.5f, .42f), FVector2D(.5f, .5f), FVector2D(0.f, -12.f), FVector2D(300.f, 2.f));

    StartButtons = WidgetTree->ConstructWidget<UVerticalBox>();
    AddCanvas(Canvas, StartButtons, FAnchors(.5f, .58f), FVector2D(.5f, 0.f), FVector2D::ZeroVector, FVector2D(380.f, 160.f), false);
    StartGameButton = AddStartButton(StartButtons, TEXT("01"), TEXT("开始游戏"), TEXT("→"));
    QuitGameButton = AddStartButton(StartButtons, TEXT("02"), TEXT("退出"), TEXT("—"));
    StartGameButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnStartGame);
    QuitGameButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnQuitGame);

    UTextBlock* Version = MakeText(WidgetTree, TEXT("版本 1.0.0"), 11, MutedInk);
    AddCanvas(Canvas, Version, FAnchors(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(112.f, -24.f), FVector2D(120.f, 20.f), true);
    UTextBlock* Credit = MakeText(WidgetTree, TEXT("青布作 · 独立制作"), 11, MutedInk);
    Credit->SetJustification(ETextJustify::Right);
    AddCanvas(Canvas, Credit, FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-46.f, -24.f), FVector2D(220.f, 20.f));
}

void UIndigoInterfaceWidget::BuildGameScreen()
{
    GameScreen = WidgetTree->ConstructWidget<UBorder>();
    GameScreen->SetBrushColor(FLinearColor::Transparent);
    GameScreen->SetPadding(FMargin(0.f));
    GameScreen->SetVisibility(ESlateVisibility::Collapsed);
    FillCanvas(Root, GameScreen, 20);

    UCanvasPanel* GameCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    GameScreen->SetContent(GameCanvas);

    UCanvasPanel* ChromeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    GameChrome = ChromeCanvas;
    ChromeCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    FillCanvas(GameCanvas, ChromeCanvas, 0);
    BuildSidebar(ChromeCanvas);

    HintText = MakeText(WidgetTree, TEXT("靠近工作台  ·  按 E 开始制作"), 20, Paper, true);
    HintText->SetJustification(ETextJustify::Center);
    HintText->SetShadowOffset(FVector2D(1.f, 2.f));
    HintText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .75f));
    AddCanvas(ChromeCanvas, HintText, FAnchors(.5f, .82f), FVector2D(.5f, .5f), FVector2D::ZeroVector, FVector2D(620.f, 54.f), true);

    UCanvasPanel* Layer = WidgetTree->ConstructWidget<UCanvasPanel>();
    WorkbenchLayer = Layer;
    Layer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    FillCanvas(GameCanvas, Layer, 10);

    BuildHeader(Layer);
    BuildPatternPanel(Layer);
    BuildTemperatureCard(Layer);
    BuildActionPanel(Layer);
    BuildResultCard(Layer);

    SubtitlePanel = WidgetTree->ConstructWidget<UBorder>();
    SubtitlePanel->SetBrushColor(FLinearColor(.015f, .06f, .12f, .88f));
    SubtitlePanel->SetPadding(FMargin(20.f, 10.f));
    SubtitleText = MakeText(WidgetTree, FString(), 18, Paper);
    SubtitleText->SetJustification(ETextJustify::Center);
    SubtitleText->SetAutoWrapText(true);
    SubtitleText->SetWrapTextAt(720.f);
    SubtitlePanel->SetContent(SubtitleText);
    AddCanvas(Layer, SubtitlePanel, FAnchors(.5f, 1.f), FVector2D(.5f, 1.f), FVector2D(0.f, -34.f), FVector2D(760.f, 56.f), true, 20);

    NarrativeCard = WidgetTree->ConstructWidget<UBorder>();
    NarrativeCard->SetBrushColor(FLinearColor(.98f, .97f, .93f, .96f));
    NarrativeCard->SetPadding(FMargin(22.f, 18.f));
    NarrativeText = MakeText(WidgetTree, FString(), 16, Ink);
    NarrativeText->SetAutoWrapText(true);
    NarrativeText->SetWrapTextAt(420.f);
    NarrativeCard->SetContent(NarrativeText);
    AddCanvas(Layer, NarrativeCard, FAnchors(.5f, .5f), FVector2D(.5f, .5f), FVector2D(0.f, 0.f), FVector2D(470.f, 210.f), true, 30);
}

void UIndigoInterfaceWidget::BuildSidebar(UCanvasPanel* Canvas)
{
    UBorder* Sidebar = WidgetTree->ConstructWidget<UBorder>();
    Sidebar->SetBrushColor(Navy);
    Sidebar->SetPadding(FMargin(0.f));
    AddCanvas(Canvas, Sidebar, FAnchors(0.f, 0.f, 0.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(88.f, 0.f), false, 2);

    UTextBlock* Brand = MakeText(WidgetTree, TEXT("蓝染工房"), 13, FLinearColor(.82f, .86f, .86f, 1.f));
    AddCanvas(Canvas, Brand, FAnchors(0.f, 0.f), FVector2D::ZeroVector, FVector2D(12.f, 18.f), FVector2D(76.f, 28.f), true, 3);

    UVerticalBox* Nav = WidgetTree->ConstructWidget<UVerticalBox>();
    AddCanvas(Canvas, Nav, FAnchors(0.f, .5f), FVector2D(0.f, .5f), FVector2D(22.f, 0.f), FVector2D(44.f, 270.f), true, 3);
    const TCHAR* Labels[] = { TEXT("01"), TEXT("02"), TEXT("03"), TEXT("04") };
    for (const TCHAR* Label : Labels)
    {
        UTextBlock* Text = MakeText(WidgetTree, Label, 15, PaleBlue);
        Text->SetJustification(ETextJustify::Center);
        UVerticalBoxSlot* TextSlot = Nav->AddChildToVerticalBox(Text);
        TextSlot->SetPadding(FMargin(0.f, 8.f));
        TextSlot->SetHorizontalAlignment(HAlign_Fill);
        TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        NavLabels.Add(Text);
    }
}

void UIndigoInterfaceWidget::BuildHeader(UCanvasPanel* Canvas)
{
    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>();
    Outer->SetBrushColor(Navy);
    Outer->SetPadding(FMargin(1.f));
    TitleCard = Outer;
    AddCanvas(Canvas, TitleCard, FAnchors(.5f, 0.f), FVector2D(.5f, 0.f), FVector2D(0.f, 22.f), FVector2D(420.f, 70.f), false, 20);

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>();
    Inner->SetBrushColor(Paper);
    Inner->SetPadding(FMargin(20.f, 12.f));
    Outer->SetContent(Inner);
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    Inner->SetContent(Row);

    StepCounter = MakeText(WidgetTree, TEXT("01 / 05"), 12, MutedInk);
    StepCounter->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* CounterSlot = Row->AddChildToHorizontalBox(StepCounter);
    CounterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    CounterSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
    CounterSlot->SetHorizontalAlignment(HAlign_Center);
    CounterSlot->SetVerticalAlignment(VAlign_Center);

    TitleText = MakeText(WidgetTree, TEXT("沿提示线折布"), 26, Navy, true);
    TitleText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* TitleSlot = Row->AddChildToHorizontalBox(TitleText);
    TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TitleSlot->SetHorizontalAlignment(HAlign_Center);
    TitleSlot->SetVerticalAlignment(VAlign_Center);

    UTextBlock* Seal = MakeText(WidgetTree, TEXT("染"), 14, Bronze, true);
    Seal->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* SealSlot = Row->AddChildToHorizontalBox(Seal);
    SealSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    SealSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
    SealSlot->SetHorizontalAlignment(HAlign_Center);
    SealSlot->SetVerticalAlignment(VAlign_Center);

    ProgressRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    AddCanvas(Canvas, ProgressRow, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-38.f, 34.f), FVector2D(390.f, 72.f), true, 20);

    const TCHAR* ProgressNames[] = { TEXT("折布"), TEXT("夹版"), TEXT("入缸"), TEXT("清洗"), TEXT("展开") };
    for (int32 Index = 0; Index < 5; ++Index)
    {
        UVerticalBox* Item = WidgetTree->ConstructWidget<UVerticalBox>();
        UHorizontalBoxSlot* ItemSlot = ProgressRow->AddChildToHorizontalBox(Item);
        ItemSlot->SetVerticalAlignment(VAlign_Center);
        ItemSlot->SetPadding(FMargin(0.f));

        USizeBox* CircleSize = WidgetTree->ConstructWidget<USizeBox>();
        CircleSize->SetWidthOverride(26.f);
        CircleSize->SetHeightOverride(26.f);
        UBorder* Circle = WidgetTree->ConstructWidget<UBorder>();
        Circle->SetBrushColor(FLinearColor(.04f, .16f, .28f, .92f));
        Circle->SetPadding(FMargin(0.f));
        CircleSize->SetContent(Circle);
        UTextBlock* Number = MakeText(WidgetTree, FString::FromInt(Index + 1), 12, Navy, true);
        Number->SetJustification(ETextJustify::Center);
        Circle->SetContent(Number);
        UVerticalBoxSlot* CircleSlot = Item->AddChildToVerticalBox(CircleSize);
        CircleSlot->SetHorizontalAlignment(HAlign_Center);

        ProgressLabels.Add(Number);
        ProgressCircles.Add(Circle);
        UTextBlock* Label = MakeText(WidgetTree, ProgressNames[Index], 11, Paper);
        Label->SetJustification(ETextJustify::Center);
        UVerticalBoxSlot* LabelSlot = Item->AddChildToVerticalBox(Label);
        LabelSlot->SetHorizontalAlignment(HAlign_Center);
        LabelSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));

        if (Index < 4)
        {
            UBorder* Connector = WidgetTree->ConstructWidget<UBorder>();
            Connector->SetBrushColor(FLinearColor(.74f, .72f, .66f, .7f));
            USizeBox* ConnectorSize = WidgetTree->ConstructWidget<USizeBox>();
            ConnectorSize->SetWidthOverride(24.f);
            ConnectorSize->SetHeightOverride(1.f);
            ConnectorSize->SetContent(Connector);
            UHorizontalBoxSlot* ConnectorSlot = ProgressRow->AddChildToHorizontalBox(ConnectorSize);
            ConnectorSlot->SetVerticalAlignment(VAlign_Center);
            ConnectorSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
        }
    }
}

void UIndigoInterfaceWidget::BuildPatternPanel(UCanvasPanel* Canvas)
{
    UCanvasPanel* PanelCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    PatternPanel = PanelCanvas;
    AddCanvas(Canvas, PatternPanel, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-40.f, 22.f), FVector2D(430.f, 112.f), false, 20);

    PatternRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    AddCanvas(PanelCanvas, PatternRow, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D::ZeroVector, FVector2D(430.f, 102.f), true);
    AddPatternCard(PatternRow, TEXT("A"), TEXT("/Game/UI/Textures/T_Mask_A_Blue.T_Mask_A_Blue"), PatternAButton);
    AddPatternCard(PatternRow, TEXT("B"), TEXT("/Game/UI/Textures/T_Mask_B_Blue.T_Mask_B_Blue"), PatternBButton);
    AddPatternCard(PatternRow, TEXT("C"), TEXT("/Game/UI/Textures/T_Mask_C_Blue.T_Mask_C_Blue"), PatternCButton);
    PatternAButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnPatternA);
    PatternBButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnPatternB);
    PatternCButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnPatternC);

    SelectionSummary = WidgetTree->ConstructWidget<UBorder>();
    SelectionSummary->SetBrushColor(FLinearColor(.98f, .97f, .93f, .93f));
    SelectionSummary->SetPadding(FMargin(14.f, 12.f));
    AddCanvas(Canvas, SelectionSummary, FAnchors(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(120.f, -42.f), FVector2D(360.f, 142.f), false, 20);

    UHorizontalBox* SummaryRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    SelectionSummary->SetContent(SummaryRow);

    auto AddChoice = [this, SummaryRow](const TCHAR* Label, TObjectPtr<UImage>& Image, TObjectPtr<UTextBlock>& Text)
    {
        UVerticalBox* Choice = WidgetTree->ConstructWidget<UVerticalBox>();
        UHorizontalBoxSlot* ChoiceSlot = SummaryRow->AddChildToHorizontalBox(Choice);
        ChoiceSlot->SetPadding(FMargin(8.f, 0.f));
        ChoiceSlot->SetVerticalAlignment(VAlign_Center);
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
        Size->SetWidthOverride(88.f);
        Size->SetHeightOverride(82.f);
        Image = WidgetTree->ConstructWidget<UImage>();
        Image->SetBrush(BoxBrush(FLinearColor(.92f, .91f, .86f), Bronze, 2.f));
        Size->SetContent(Image);
        UVerticalBoxSlot* ImageSlot = Choice->AddChildToVerticalBox(Size);
        ImageSlot->SetHorizontalAlignment(HAlign_Center);
        Text = MakeText(WidgetTree, Label, 11, MutedInk);
        Text->SetJustification(ETextJustify::Center);
        UVerticalBoxSlot* TextSlot = Choice->AddChildToVerticalBox(Text);
        TextSlot->SetHorizontalAlignment(HAlign_Center);
        TextSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    };
    AddChoice(TEXT("左版"), LeftPatternImage, LeftPatternLabel);
    UTextBlock* Multiply = MakeText(WidgetTree, TEXT("×"), 23, Bronze);
    Multiply->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* MultiplySlot = SummaryRow->AddChildToHorizontalBox(Multiply);
    MultiplySlot->SetVerticalAlignment(VAlign_Center);
    AddChoice(TEXT("右版"), RightPatternImage, RightPatternLabel);
}

void UIndigoInterfaceWidget::BuildTemperatureCard(UCanvasPanel* Canvas)
{
    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>();
    Outer->SetBrushColor(Bronze);
    Outer->SetPadding(FMargin(2.f));
    TemperatureCard = Outer;
    AddCanvas(Canvas, TemperatureCard, FAnchors(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(120.f, -42.f), FVector2D(280.f, 142.f), false, 20);

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>();
    Inner->SetBrushColor(FLinearColor(.04f, .09f, .16f, .84f));
    Inner->SetPadding(FMargin(16.f, 12.f));
    Outer->SetContent(Inner);

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    Inner->SetContent(Row);
    UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* CopySlot = Row->AddChildToHorizontalBox(Copy);
    CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UTextBlock* Caption = MakeText(WidgetTree, TEXT("染缸温度"), 11, PaleBlue);
    Copy->AddChildToVerticalBox(Caption);
    TemperatureValue = MakeText(WidgetTree, TEXT("合适"), 25, Paper, true);
    UVerticalBoxSlot* ValueSlot = Copy->AddChildToVerticalBox(TemperatureValue);
    ValueSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
    TemperatureDescription = MakeText(WidgetTree, TEXT("可以继续入染"), 12, PaleBlue);
    TemperatureDescription->SetAutoWrapText(true);
    TemperatureDescription->SetWrapTextAt(170.f);
    Copy->AddChildToVerticalBox(TemperatureDescription);

    USizeBox* HeatSize = WidgetTree->ConstructWidget<USizeBox>();
    HeatSize->SetWidthOverride(74.f);
    HeatSize->SetHeightOverride(74.f);
    HeatButton = WidgetTree->ConstructWidget<UButton>();
    HeatButton->SetStyle(ButtonStyle(FLinearColor(.75f, .70f, .62f, .18f), FLinearColor(.75f, .70f, .62f, .35f), Navy, PaleBlue));
    HeatButton->SetContent(MakeText(WidgetTree, TEXT("加 热"), 13, Paper, true));
    HeatButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnAddFuel);
    HeatSize->SetContent(HeatButton);
    UHorizontalBoxSlot* HeatSlot = Row->AddChildToHorizontalBox(HeatSize);
    HeatSlot->SetVerticalAlignment(VAlign_Center);
    HeatSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
}

void UIndigoInterfaceWidget::BuildActionPanel(UCanvasPanel* Canvas)
{
    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>();
    Outer->SetBrushColor(Navy);
    Outer->SetPadding(FMargin(1.f));
    ActionPanel = Outer;
    AddCanvas(Canvas, ActionPanel, FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-40.f, -42.f), FVector2D(360.f, 0.f), true, 20);

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>();
    Inner->SetBrushColor(FLinearColor(.97f, .96f, .92f, .95f));
    Inner->SetPadding(FMargin(12.f, 10.f));
    Outer->SetContent(Inner);
    ActionList = WidgetTree->ConstructWidget<UVerticalBox>();
    Inner->SetContent(ActionList);

    ClothButton = AddActionButton(ActionList, TEXT("01"), TEXT("查看旧布"));
    OrderButton = AddActionButton(ActionList, TEXT("02"), TEXT("查看委托单"));
    ConfirmButton = AddActionButton(ActionList, TEXT("→"), TEXT("确认搭配"));
    SwapButton = AddActionButton(ActionList, TEXT("↔"), TEXT("左右交换"));
    ClearLeftButton = AddActionButton(ActionList, TEXT("×"), TEXT("重选左版"));
    ClearRightButton = AddActionButton(ActionList, TEXT("×"), TEXT("重选右版"));
    FoldButton = AddActionButton(ActionList, TEXT("01"), TEXT("沿提示线折布"));
    PlaceButton = AddActionButton(ActionList, TEXT("01"), TEXT("放置染布"));
    ClosePlateButton = AddActionButton(ActionList, TEXT("02"), TEXT("合上花版"));
    ClampLeftButton = AddActionButton(ActionList, TEXT("03"), TEXT("扣紧左夹"));
    ClampRightButton = AddActionButton(ActionList, TEXT("04"), TEXT("扣紧右夹"));
    DyeButton = AddActionButton(ActionList, TEXT("01"), TEXT("继续浸染"));
    FinishDyeButton = AddActionButton(ActionList, TEXT("02"), TEXT("收工"));
    ScoopButton = AddActionButton(ActionList, TEXT("01"), TEXT("舀取染液"));
    DipButton = AddActionButton(ActionList, TEXT("02"), TEXT("浸入试布"));
    LiftButton = AddActionButton(ActionList, TEXT("03"), TEXT("提出试布"));
    OxidizeButton = AddActionButton(ActionList, TEXT("04"), TEXT("等待显色"));
    WashButton = AddActionButton(ActionList, TEXT("01"), TEXT("用清水漂洗"));
    DrainButton = AddActionButton(ActionList, TEXT("02"), TEXT("提起沥水"));
    UnclampLeftButton = AddActionButton(ActionList, TEXT("03"), TEXT("松开左夹"));
    UnclampRightButton = AddActionButton(ActionList, TEXT("03"), TEXT("松开右夹"));
    UnfoldButton = AddActionButton(ActionList, TEXT("04"), TEXT("展开作品"));
    DeliverButton = AddActionButton(ActionList, TEXT("→"), TEXT("交付作品"));
    ReviewButton = AddActionButton(ActionList, TEXT("01"), TEXT("回看作品"));
    RestartButton = AddActionButton(ActionList, TEXT("02"), TEXT("重新开始"));
    QuitFromResultButton = AddActionButton(ActionList, TEXT("03"), TEXT("退出游戏"));
    BackButton = AddActionButton(ActionList, TEXT("←"), TEXT("返回完成页"));
    ShipButton = AddActionButton(ActionList, TEXT("→"), TEXT("叠布寄件"));
    ExitWorkbenchButton = AddActionButton(ActionList, TEXT("E"), TEXT("返回行走"));
    CameraTableButton = AddActionButton(ActionList, TEXT("01"), TEXT("工作台全景"));
    CameraCloseupButton = AddActionButton(ActionList, TEXT("02"), TEXT("操作近景"));
    CameraQuiltButton = AddActionButton(ActionList, TEXT("03"), TEXT("作品展示"));

    ClothButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnCloth);
    OrderButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnOrder);
    ConfirmButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnConfirm);
    SwapButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnSwap);
    ClearLeftButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnClearLeft);
    ClearRightButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnClearRight);
    FoldButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnFold);
    PlaceButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnPlace);
    ClosePlateButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnClosePlate);
    ClampLeftButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnClampLeft);
    ClampRightButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnClampRight);
    DyeButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnDye);
    FinishDyeButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnFinishDye);
    ScoopButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnScoop);
    DipButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnDip);
    LiftButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnLift);
    OxidizeButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnOxidize);
    WashButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnWash);
    DrainButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnDrain);
    UnclampLeftButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnUnclampLeft);
    UnclampRightButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnUnclampRight);
    UnfoldButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnUnfold);
    DeliverButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnDeliver);
    ReviewButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnReview);
    RestartButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnRestart);
    QuitFromResultButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnQuitGame);
    BackButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnBack);
    ShipButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnShip);
    ExitWorkbenchButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnExitWorkbench);
    CameraTableButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnCameraTable);
    CameraCloseupButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnCameraCloseup);
    CameraQuiltButton->OnClicked.AddDynamic(this, &UIndigoInterfaceWidget::OnCameraQuilt);
}

void UIndigoInterfaceWidget::BuildResultCard(UCanvasPanel* Canvas)
{
    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>();
    Outer->SetBrushColor(Bronze);
    Outer->SetPadding(FMargin(4.f));
    ResultCard = Outer;
    AddCanvas(Canvas, ResultCard, FAnchors(.5f, .5f), FVector2D(.5f, .5f), FVector2D(0.f, 12.f), FVector2D(610.f, 430.f), false, 10);

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>();
    Inner->SetBrushColor(Paper);
    Inner->SetPadding(FMargin(28.f, 20.f));
    Outer->SetContent(Inner);
    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    Inner->SetContent(Column);

    ResultEyebrow = MakeText(WidgetTree, TEXT("作 品 完 成"), 12, Bronze);
    ResultEyebrow->SetJustification(ETextJustify::Center);
    UVerticalBoxSlot* EyebrowSlot = Column->AddChildToVerticalBox(ResultEyebrow);
    EyebrowSlot->SetHorizontalAlignment(HAlign_Center);
    ResultTitle = MakeText(WidgetTree, TEXT("染 成"), 48, Navy, true);
    ResultTitle->SetJustification(ETextJustify::Center);
    UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(ResultTitle);
    TitleSlot->SetHorizontalAlignment(HAlign_Center);
    TitleSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 16.f));

    UBorder* ClothOuter = WidgetTree->ConstructWidget<UBorder>();
    ClothOuter->SetBrushColor(Bronze);
    ClothOuter->SetPadding(FMargin(8.f));
    ClothPreview = ClothOuter;
    UVerticalBoxSlot* ClothSlot = Column->AddChildToVerticalBox(ClothPreview);
    ClothSlot->SetHorizontalAlignment(HAlign_Fill);
    UBorder* ClothInner = WidgetTree->ConstructWidget<UBorder>();
    ClothInner->SetBrushColor(Navy);
    ClothInner->SetPadding(FMargin(16.f));
    ClothOuter->SetContent(ClothInner);
    ClothPreviewContent = WidgetTree->ConstructWidget<UHorizontalBox>();
    ClothInner->SetContent(ClothPreviewContent);

    ResultQuote = MakeText(WidgetTree, TEXT("「 一缸一染，皆有温度 」"), 14, MutedInk);
    ResultQuote->SetJustification(ETextJustify::Center);
    UVerticalBoxSlot* QuoteSlot = Column->AddChildToVerticalBox(ResultQuote);
    QuoteSlot->SetHorizontalAlignment(HAlign_Center);
    QuoteSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
}

UButton* UIndigoInterfaceWidget::AddActionButton(UVerticalBox* Parent, const FString& Index, const FString& Label)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    Button->SetStyle(ButtonStyle(FLinearColor(.99f, .98f, .95f, 1.f), FLinearColor(.88f, .87f, .82f, 1.f), Navy, Navy));

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    UTextBlock* IndexText = MakeText(WidgetTree, Index, 11, MutedInk);
    IndexText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* IndexSlot = Row->AddChildToHorizontalBox(IndexText);
    IndexSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    IndexSlot->SetHorizontalAlignment(HAlign_Center);
    IndexSlot->SetVerticalAlignment(VAlign_Center);
    UTextBlock* LabelText = MakeText(WidgetTree, Label, 16, Navy, true);
    LabelText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetHorizontalAlignment(HAlign_Center);
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    UTextBlock* Arrow = MakeText(WidgetTree, TEXT("→"), 14, MutedInk);
    Arrow->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* ArrowSlot = Row->AddChildToHorizontalBox(Arrow);
    ArrowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ArrowSlot->SetHorizontalAlignment(HAlign_Center);
    ArrowSlot->SetVerticalAlignment(VAlign_Center);

    USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
    Height->SetWidthOverride(360.f);
    Height->SetHeightOverride(49.f);
    Height->SetContent(Row);
    Button->SetContent(Height);
    UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
    ButtonSlot->SetPadding(FMargin(0.f, 4.f));
    ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
    return Button;
}

UButton* UIndigoInterfaceWidget::AddStartButton(
    UVerticalBox* Parent,
    const FString& Index,
    const FString& Label,
    const FString& Glyph)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    Button->SetStyle(ButtonStyle(FLinearColor(.99f, .98f, .95f, .98f), FLinearColor(.93f, .91f, .85f, 1.f), Navy, Navy));
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    UTextBlock* IndexText = MakeText(WidgetTree, Index, 11, MutedInk);
    IndexText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* IndexSlot = Row->AddChildToHorizontalBox(IndexText);
    IndexSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    IndexSlot->SetHorizontalAlignment(HAlign_Center);
    IndexSlot->SetVerticalAlignment(VAlign_Center);
    UTextBlock* LabelText = MakeText(WidgetTree, Label, 18, Navy, true);
    LabelText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetHorizontalAlignment(HAlign_Center);
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    UTextBlock* GlyphText = MakeText(WidgetTree, Glyph, 15, MutedInk);
    GlyphText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* GlyphSlot = Row->AddChildToHorizontalBox(GlyphText);
    GlyphSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    GlyphSlot->SetHorizontalAlignment(HAlign_Center);
    GlyphSlot->SetVerticalAlignment(VAlign_Center);
    USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
    Height->SetWidthOverride(380.f);
    Height->SetHeightOverride(58.f);
    Height->SetContent(Row);
    Button->SetContent(Height);
    UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
    ButtonSlot->SetPadding(FMargin(0.f, 7.f));
    ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
    return Button;
}

UImage* UIndigoInterfaceWidget::AddPatternCard(
    UHorizontalBox* Parent,
    const FString& Card,
    const FString& AssetName,
    TObjectPtr<UButton>& OutButton)
{
    OutButton = WidgetTree->ConstructWidget<UButton>();
    OutButton->SetStyle(ButtonStyle(FLinearColor(.96f, .95f, .90f, .96f), FLinearColor::White, Navy, Bronze));
    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
    Size->SetWidthOverride(126.f);
    Size->SetHeightOverride(96.f);
    OutButton->SetContent(Size);

    UCanvasPanel* CardCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    Size->SetContent(CardCanvas);
    UImage* Pattern = WidgetTree->ConstructWidget<UImage>();
    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetName);
    if (Texture) Pattern->SetBrushFromTexture(Texture, false);
    else Pattern->SetBrush(BoxBrush(FLinearColor(.94f, .93f, .88f), FLinearColor::Transparent));
    FillCanvas(CardCanvas, Pattern);

    UBorder* Badge = WidgetTree->ConstructWidget<UBorder>();
    Badge->SetBrushColor(Navy);
    Badge->SetPadding(FMargin(7.f, 1.f));
    UTextBlock* Letter = MakeText(WidgetTree, Card, 12, Paper, true);
    Badge->SetContent(Letter);
    AddCanvas(CardCanvas, Badge, FAnchors(0.f, 0.f), FVector2D::ZeroVector, FVector2D(3.f, 3.f), FVector2D(25.f, 23.f), false, 2);

    UHorizontalBoxSlot* CardSlot = Parent->AddChildToHorizontalBox(OutButton);
    CardSlot->SetPadding(FMargin(8.f, 0.f));
    CardSlot->SetVerticalAlignment(VAlign_Top);
    return Pattern;
}

void UIndigoInterfaceWidget::Refresh()
{
    if (!Bridge)
    {
        SetGameStarted(false);
        return;
    }

    SetGameStarted(Bridge->bGameStarted);
    const bool bActive = Bridge->bGameStarted && Bridge->bWorkbenchActive && Bridge->GetFlow();
    WorkbenchLayer->SetVisibility(bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    HintText->SetVisibility(Bridge->bGameStarted && !bActive && Bridge->IsPlayerNear()
        ? ESlateVisibility::HitTestInvisible
        : ESlateVisibility::Collapsed);
    HintText->SetText(FText::FromString(Bridge->bGameStarted ? TEXT("靠近工作台  ·  按 E 开始制作") : FString()));

    if (!bActive)
    {
        return;
    }

    UIndigoFlowComponent* Flow = Bridge->GetFlow();
    if (Flow->Step != LastStep)
    {
        LastStep = Flow->Step;
        PageAnimationTime = 0.f;
    }

    const auto Stage = [Flow](int32& OutProgress, FString& OutTitle, FString& OutCounter)
    {
        switch (Flow->Step)
        {
        case EIndigoStep::Receive:
        case EIndigoStep::Select:
            OutProgress = 0;
            OutTitle = TEXT("纹样选择");
            OutCounter = Flow->Step == EIndigoStep::Receive ? TEXT("STEP 00") : TEXT("STEP 01");
            break;
        case EIndigoStep::Trial:
            OutProgress = 1;
            OutTitle = TEXT("试布查温");
            OutCounter = TEXT("准备 / 05");
            break;
        case EIndigoStep::Fold:
            OutProgress = 1;
            OutTitle = TEXT("沿提示线折布");
            OutCounter = FString::Printf(TEXT("%02d / 05"), 1);
            break;
        case EIndigoStep::Clamp:
            OutProgress = 2;
            OutTitle = TEXT("夹紧两侧花版");
            OutCounter = FString::Printf(TEXT("%02d / 05"), 2);
            break;
        case EIndigoStep::Dye:
            OutProgress = 3;
            OutTitle = TEXT("缓慢浸入染缸");
            OutCounter = FString::Printf(TEXT("%02d / 05"), 3);
            break;
        case EIndigoStep::Wash:
            OutProgress = 4;
            OutTitle = TEXT("用清水轻轻漂洗");
            OutCounter = FString::Printf(TEXT("%02d / 05"), 4);
            break;
        case EIndigoStep::Reveal:
        case EIndigoStep::Delivery:
            OutProgress = 5;
            OutTitle = Flow->Step == EIndigoStep::Delivery ? TEXT("寄出这一床蓝染") : TEXT("展开你的蓝染");
            OutCounter = FString::Printf(TEXT("%02d / 05"), 5);
            break;
        case EIndigoStep::End:
        default:
            OutProgress = 5;
            OutTitle = TEXT("染成");
            OutCounter = TEXT("05 / 05");
            break;
        }
    };

    int32 Progress = 0;
    FString Title;
    FString Counter;
    Stage(Progress, Title, Counter);
    StepCounter->SetText(FText::FromString(Counter));
    TitleText->SetText(FText::FromString(Title));

    for (int32 Index = 0; Index < ProgressLabels.Num(); ++Index)
    {
        const bool bDone = Progress > 0 && Index + 1 < Progress;
        const bool bCurrent = Progress > 0 && Index + 1 == Progress;
        ProgressLabels[Index]->SetColorAndOpacity(FSlateColor(Paper));
        if (ProgressCircles.IsValidIndex(Index))
            ProgressCircles[Index]->SetBrushColor(bCurrent ? Bronze : bDone ? Navy : FLinearColor(.04f, .16f, .28f, .55f));
    }

    const bool bSelection = Flow->Step == EIndigoStep::Receive || Flow->Step == EIndigoStep::Select;
    PatternPanel->SetVisibility(bSelection && Flow->Step == EIndigoStep::Select ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ProgressRow->SetVisibility(Progress > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    SelectionSummary->SetVisibility(Flow->Step == EIndigoStep::Select ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    TitleCard->SetVisibility(ESlateVisibility::HitTestInvisible);

    const bool bTemperatureVisible =
        Flow->Step == EIndigoStep::Trial || Flow->Step == EIndigoStep::Fold ||
        Flow->Step == EIndigoStep::Clamp || Flow->Step == EIndigoStep::Dye ||
        Flow->Step == EIndigoStep::Wash;
    TemperatureCard->SetVisibility(bTemperatureVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    const bool bReady = Flow->Temperature == EIndigoTemperature::Ready;
    TemperatureValue->SetText(FText::FromString(bReady ? TEXT("合适") : TEXT("偏低")));
    TemperatureValue->SetColorAndOpacity(FSlateColor(bReady ? Paper : FLinearColor(.86f, .73f, .56f, 1.f)));
    TemperatureDescription->SetText(FText::FromString(
        bReady ? TEXT("可以继续入染") : Flow->bColdTrialSeen ? TEXT("添一份谷壳后可继续") : TEXT("染液尚未醒透")));
    HeatButton->SetVisibility(Flow->CanPerform(EIndigoAction::AddFuel) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    // Reveal is a scene-only presentation. Keep the finished-work popup for End.
    const bool bResult = Flow->Step == EIndigoStep::End;
    ResultCard->SetVisibility(bResult ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (bResult)
    {
        ResultTitle->SetText(FText::FromString(TEXT("染 成")));
        ResultEyebrow->SetText(FText::FromString(TEXT("作 品 完 成")));
        ResultQuote->SetText(FText::FromString(FString::Printf(TEXT("「 一缸一染，皆有温度 」   ·   染色 %d / 4"), Flow->Result.Depth)));
    }

    const bool bNarrative = (Flow->Step == EIndigoStep::Select && Flow->HasSeenOrder()) || Flow->Step == EIndigoStep::Delivery;
    NarrativeCard->SetVisibility(bNarrative ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bNarrative)
    {
        NarrativeText->SetText(Flow->Step == EIndigoStep::Delivery
            ? FText::Format(FText::FromString(TEXT("委托单背面\n\n{0}\n\n{1}")), Flow->OrderBack, Flow->ShippingText)
            : FText::Format(FText::FromString(TEXT("委托单\n\n{0}")), Flow->OrderFront));
    }

    const bool bSubtitle = !Bridge->CurrentSubtitle.IsEmpty();
    SubtitlePanel->SetVisibility(bSubtitle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    SubtitleText->SetText(Bridge->CurrentSubtitle);

    const int32 ActiveNav = Flow->Step == EIndigoStep::End || Flow->Step == EIndigoStep::Reveal || Flow->Step == EIndigoStep::Delivery ? 3
        : (Flow->Step == EIndigoStep::Receive || Flow->Step == EIndigoStep::Select) ? 1 : 2;
    for (int32 Index = 0; Index < NavLabels.Num(); ++Index)
    {
        NavLabels[Index]->SetColorAndOpacity(FSlateColor(Index == ActiveNav ? Paper : FLinearColor(.65f, .73f, .77f, .72f)));
        NavLabels[Index]->SetFont(UiFont(Index == ActiveNav ? 17 : 15, Index == ActiveNav));
    }

    SetPatternImage(LeftPatternImage, Flow->Result.Left);
    SetPatternImage(RightPatternImage, Flow->Result.Right);
    LeftPatternLabel->SetText(FText::FromString(FString::Printf(TEXT("左版 · %s"), PatternText(Flow->Result.Left))));
    RightPatternLabel->SetText(FText::FromString(FString::Printf(TEXT("右版 · %s"), PatternText(Flow->Result.Right))));

    UpdateActions();
    UpdateClothPreview();
}

void UIndigoInterfaceWidget::UpdateActions()
{
    if (!Bridge || !Bridge->GetFlow()) return;
    UIndigoFlowComponent* Flow = Bridge->GetFlow();
    for (UButton* Button : {
        ClothButton.Get(), OrderButton.Get(), ConfirmButton.Get(), SwapButton.Get(), ClearLeftButton.Get(),
        ClearRightButton.Get(), FoldButton.Get(), PlaceButton.Get(), ClosePlateButton.Get(), ClampLeftButton.Get(),
        ClampRightButton.Get(), DyeButton.Get(), FinishDyeButton.Get(), ScoopButton.Get(), DipButton.Get(),
        LiftButton.Get(), OxidizeButton.Get(), WashButton.Get(), DrainButton.Get(), UnclampLeftButton.Get(),
        UnclampRightButton.Get(), UnfoldButton.Get(), DeliverButton.Get(), ReviewButton.Get(), RestartButton.Get(),
        QuitFromResultButton.Get(), BackButton.Get(), ShipButton.Get(), ExitWorkbenchButton.Get(), CameraTableButton.Get(),
        CameraCloseupButton.Get(), CameraQuiltButton.Get() })
    {
        if (Button) Button->SetVisibility(ESlateVisibility::Collapsed);
    }

    auto Show = [this](UButton* Button, const bool bEnabled)
    {
        if (Button)
        {
            Button->SetVisibility(ESlateVisibility::Visible);
            Button->SetIsEnabled(bEnabled);
        }
    };

    switch (Flow->Step)
    {
    case EIndigoStep::Receive:
        Show(ClothButton, Flow->CanPerform(EIndigoAction::InspectCloth));
        Show(OrderButton, Flow->CanPerform(EIndigoAction::InspectOrder));
        break;
    case EIndigoStep::Select:
        Show(ConfirmButton, Flow->CanPerform(EIndigoAction::Confirm));
        Show(SwapButton, Flow->CanPerform(EIndigoAction::Swap));
        Show(ClearLeftButton, Flow->Result.Left != EIndigoPattern::None);
        Show(ClearRightButton, Flow->Result.Right != EIndigoPattern::None);
        break;
    case EIndigoStep::Trial:
        Show(ScoopButton, Flow->CanPerform(EIndigoAction::ScoopTest));
        Show(DipButton, Flow->CanPerform(EIndigoAction::DipTest));
        Show(LiftButton, Flow->CanPerform(EIndigoAction::LiftTest));
        Show(OxidizeButton, Flow->CanPerform(EIndigoAction::OxidizeTest));
        break;
    case EIndigoStep::Fold:
        Show(FoldButton, Flow->CanPerform(EIndigoAction::Fold, Flow->FoldIndex));
        break;
    case EIndigoStep::Clamp:
        Show(PlaceButton, Flow->CanPerform(EIndigoAction::PlaceCloth));
        Show(ClosePlateButton, Flow->CanPerform(EIndigoAction::ClosePlate));
        Show(ClampLeftButton, Flow->CanPerform(EIndigoAction::ClampLeft));
        Show(ClampRightButton, Flow->CanPerform(EIndigoAction::ClampRight));
        break;
    case EIndigoStep::Dye:
        Show(DyeButton, Flow->CanPerform(EIndigoAction::Dye));
        Show(FinishDyeButton, Flow->CanPerform(EIndigoAction::FinishDye));
        break;
    case EIndigoStep::Wash:
        Show(WashButton, Flow->CanPerform(EIndigoAction::Wash));
        Show(DrainButton, Flow->CanPerform(EIndigoAction::Drain));
        Show(UnclampLeftButton, Flow->CanPerform(EIndigoAction::UnclampLeft));
        Show(UnclampRightButton, Flow->CanPerform(EIndigoAction::UnclampRight));
        Show(UnfoldButton, Flow->CanPerform(EIndigoAction::Unfold));
        break;
    case EIndigoStep::Reveal:
        Show(DeliverButton, Flow->CanPerform(EIndigoAction::Deliver));
        Show(BackButton, Flow->IsReview());
        Show(CameraTableButton, Bridge->CanSwitchWorkbenchCamera(EIndigoCamera::Table));
        Show(CameraCloseupButton, Bridge->CanSwitchWorkbenchCamera(EIndigoCamera::Closeup));
        Show(CameraQuiltButton, Bridge->CanSwitchWorkbenchCamera(EIndigoCamera::Quilt));
        break;
    case EIndigoStep::Delivery:
        Show(ShipButton, Flow->CanPerform(EIndigoAction::Ship));
        break;
    case EIndigoStep::End:
        Show(ReviewButton, Flow->CanPerform(EIndigoAction::Review));
        Show(RestartButton, true);
        Show(QuitFromResultButton, true);
        break;
    default:
        break;
    }
    Show(ExitWorkbenchButton, !Flow->IsInputLocked());
    ActionPanel->SetVisibility(ESlateVisibility::Visible);
}

void UIndigoInterfaceWidget::UpdateClothPreview()
{
    if (!Bridge || !Bridge->GetFlow() || !ClothPreviewContent) return;
    UIndigoFlowComponent* Flow = Bridge->GetFlow();
    if (LastLeft == Flow->Result.Left && LastRight == Flow->Result.Right && LastDepth == Flow->Result.Depth) return;
    LastLeft = Flow->Result.Left;
    LastRight = Flow->Result.Right;
    LastDepth = Flow->Result.Depth;
    ClothPreviewContent->ClearChildren();

    const TArray<EIndigoPattern> Patterns = { Flow->Result.Left, EIndigoPattern::B, Flow->Result.Right };
    for (int32 Index = 0; Index < Patterns.Num(); ++Index)
    {
        UVerticalBox* Piece = WidgetTree->ConstructWidget<UVerticalBox>();
        UHorizontalBoxSlot* PieceSlot = ClothPreviewContent->AddChildToHorizontalBox(Piece);
        PieceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        PieceSlot->SetPadding(FMargin(5.f));
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
        Size->SetHeightOverride(126.f);
        UImage* Image = WidgetTree->ConstructWidget<UImage>();
        SetPatternImage(Image, Patterns[Index]);
        Size->SetContent(Image);
        UVerticalBoxSlot* ImageSlot = Piece->AddChildToVerticalBox(Size);
        ImageSlot->SetHorizontalAlignment(HAlign_Fill);
        UTextBlock* Label = MakeText(WidgetTree, Index == 1 ? TEXT("中心留样") : PatternText(Patterns[Index]), 11, Paper);
        Label->SetJustification(ETextJustify::Center);
        Piece->AddChildToVerticalBox(Label);
    }
}

void UIndigoInterfaceWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateAnimatedElements(InDeltaTime);
    if (Bridge) Refresh();
}

void UIndigoInterfaceWidget::UpdateAnimatedElements(const float DeltaTime)
{
    if (bStartScreenVisible)
    {
        StartAnimationTime += DeltaTime;
        const float LogoA = FMath::Clamp(StartAnimationTime / .7f, 0.f, 1.f);
        const float LogoEase = LogoA * LogoA * (3.f - 2.f * LogoA);
        StartLogo->SetRenderOpacity(LogoEase);
        StartLogo->SetRenderTranslation(FVector2D(0.f, 18.f * (1.f - LogoEase)));
        StartSubtitle->SetRenderOpacity(FMath::Clamp((StartAnimationTime - .15f) / .6f, 0.f, 1.f));
        const float ButtonsA = FMath::Clamp((StartAnimationTime - .3f) / .55f, 0.f, 1.f);
        StartButtons->SetRenderOpacity(ButtonsA);
        StartButtons->SetRenderTranslation(FVector2D(0.f, 12.f * (1.f - ButtonsA)));
        StartGameButton->SetRenderScale(FVector2D(1.f, 1.f));
        return;
    }

    PageAnimationTime += DeltaTime;
    const float A = FMath::Clamp(PageAnimationTime / .38f, 0.f, 1.f);
    const float Ease = A * A * (3.f - 2.f * A);
    if (TitleCard) TitleCard->SetRenderTranslation(FVector2D(0.f, -10.f * (1.f - Ease)));
    if (PatternPanel) PatternPanel->SetRenderTranslation(FVector2D(18.f * (1.f - Ease), 0.f));
    if (ActionPanel) ActionPanel->SetRenderTranslation(FVector2D(20.f * (1.f - Ease), 0.f));
    if (TemperatureCard) TemperatureCard->SetRenderTranslation(FVector2D(-16.f * (1.f - Ease), 0.f));
    if (ResultCard && ResultCard->GetVisibility() == ESlateVisibility::HitTestInvisible)
    {
        const float ResultA = FMath::Clamp(PageAnimationTime / .55f, 0.f, 1.f);
        const float ResultEase = ResultA * ResultA * (3.f - 2.f * ResultA);
        ResultCard->SetRenderOpacity(ResultEase);
        ResultCard->SetRenderScale(FVector2D(FMath::Lerp(.96f, 1.f, ResultEase)));
        ResultCard->SetRenderTransformPivot(FVector2D(.5f, .5f));
    }
}

void UIndigoInterfaceWidget::SetGameStarted(const bool bStarted)
{
    bStartScreenVisible = !bStarted;
    StartScreen->SetVisibility(bStarted ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    GameScreen->SetVisibility(bStarted ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UIndigoInterfaceWidget::SetActionVisible(UButton* Button, const bool bVisible, const bool bEnabled)
{
    if (!Button) return;
    Button->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    Button->SetIsEnabled(bEnabled);
}

bool UIndigoInterfaceWidget::IsPointerOverPanel() const
{
    return (ActionPanel && ActionPanel->IsHovered()) ||
        (PatternPanel && PatternPanel->IsHovered()) ||
        (TemperatureCard && TemperatureCard->IsHovered()) ||
        (SelectionSummary && SelectionSummary->IsHovered()) ||
        (NarrativeCard && NarrativeCard->GetVisibility() == ESlateVisibility::Visible && NarrativeCard->IsHovered());
}

FReply UIndigoInterfaceWidget::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent)
{
    if (Bridge && Bridge->bWorkbenchActive && !IsPointerOverPanel())
    {
        if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
        {
            Bridge->CancelDrag();
            return FReply::Handled().ReleaseMouseCapture();
        }
        if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
        {
            FVector2D Pixel;
            FVector2D Viewport;
            USlateBlueprintLibrary::AbsoluteToViewport(this, InMouseEvent.GetScreenSpacePosition(), Pixel, Viewport);
            if (Bridge->PointerDown(Pixel)) return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    return Super::NativeOnMouseButtonDown(MyGeometry, InMouseEvent);
}

FReply UIndigoInterfaceWidget::NativeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent)
{
    if (Bridge && Bridge->bWorkbenchActive && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        FVector2D Pixel;
        FVector2D Viewport;
        USlateBlueprintLibrary::AbsoluteToViewport(this, InMouseEvent.GetScreenSpacePosition(), Pixel, Viewport);
        Bridge->PointerUp(Pixel);
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(MyGeometry, InMouseEvent);
}

FReply UIndigoInterfaceWidget::NativeOnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent)
{
    if (Bridge && Bridge->bWorkbenchActive)
    {
        FVector2D Pixel;
        FVector2D Viewport;
        USlateBlueprintLibrary::AbsoluteToViewport(this, InMouseEvent.GetScreenSpacePosition(), Pixel, Viewport);
        Bridge->PointerMove(Pixel);
    }
    return Super::NativeOnMouseMove(MyGeometry, InMouseEvent);
}

void UIndigoInterfaceWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& InMouseEvent)
{
    if (Bridge) Bridge->CancelDrag();
    Super::NativeOnMouseCaptureLost(InMouseEvent);
}

void UIndigoInterfaceWidget::OnStartGame()
{
    if (Bridge) Bridge->StartGame();
}

void UIndigoInterfaceWidget::OnQuitGame()
{
    if (Bridge) Bridge->QuitGame();
}

void UIndigoInterfaceWidget::OnCloth() { if (Bridge) Bridge->PerformAction(EIndigoAction::InspectCloth); }
void UIndigoInterfaceWidget::OnOrder() { if (Bridge) Bridge->PerformAction(EIndigoAction::InspectOrder); }
void UIndigoInterfaceWidget::OnPatternA() { if (Bridge) Bridge->SelectNextPattern(EIndigoPattern::A); }
void UIndigoInterfaceWidget::OnPatternB() { if (Bridge) Bridge->SelectNextPattern(EIndigoPattern::B); }
void UIndigoInterfaceWidget::OnPatternC() { if (Bridge) Bridge->SelectNextPattern(EIndigoPattern::C); }
void UIndigoInterfaceWidget::OnSwap() { if (Bridge) Bridge->PerformAction(EIndigoAction::Swap); }
void UIndigoInterfaceWidget::OnClearLeft() { if (Bridge) Bridge->PerformAction(EIndigoAction::ClearLeft); }
void UIndigoInterfaceWidget::OnClearRight() { if (Bridge) Bridge->PerformAction(EIndigoAction::ClearRight); }
void UIndigoInterfaceWidget::OnConfirm() { if (Bridge) Bridge->PerformAction(EIndigoAction::Confirm); }
void UIndigoInterfaceWidget::OnFold()
{
    if (Bridge && Bridge->GetFlow()) Bridge->PerformAction(EIndigoAction::Fold, Bridge->GetFlow()->FoldIndex);
}
void UIndigoInterfaceWidget::OnPlace() { if (Bridge) Bridge->PerformAction(EIndigoAction::PlaceCloth); }
void UIndigoInterfaceWidget::OnClosePlate() { if (Bridge) Bridge->PerformAction(EIndigoAction::ClosePlate); }
void UIndigoInterfaceWidget::OnClampLeft() { if (Bridge) Bridge->PerformAction(EIndigoAction::ClampLeft); }
void UIndigoInterfaceWidget::OnClampRight() { if (Bridge) Bridge->PerformAction(EIndigoAction::ClampRight); }
void UIndigoInterfaceWidget::OnDye() { if (Bridge) Bridge->PerformAction(EIndigoAction::Dye); }
void UIndigoInterfaceWidget::OnFinishDye() { if (Bridge) Bridge->PerformAction(EIndigoAction::FinishDye); }
void UIndigoInterfaceWidget::OnAddFuel() { if (Bridge) Bridge->PerformAction(EIndigoAction::AddFuel); }
void UIndigoInterfaceWidget::OnScoop() { if (Bridge) Bridge->PerformAction(EIndigoAction::ScoopTest); }
void UIndigoInterfaceWidget::OnDip() { if (Bridge) Bridge->PerformAction(EIndigoAction::DipTest); }
void UIndigoInterfaceWidget::OnLift() { if (Bridge) Bridge->PerformAction(EIndigoAction::LiftTest); }
void UIndigoInterfaceWidget::OnOxidize() { if (Bridge) Bridge->PerformAction(EIndigoAction::OxidizeTest); }
void UIndigoInterfaceWidget::OnWash() { if (Bridge) Bridge->PerformAction(EIndigoAction::Wash); }
void UIndigoInterfaceWidget::OnDrain() { if (Bridge) Bridge->PerformAction(EIndigoAction::Drain); }
void UIndigoInterfaceWidget::OnUnclampLeft() { if (Bridge) Bridge->PerformAction(EIndigoAction::UnclampLeft); }
void UIndigoInterfaceWidget::OnUnclampRight() { if (Bridge) Bridge->PerformAction(EIndigoAction::UnclampRight); }
void UIndigoInterfaceWidget::OnUnfold() { if (Bridge) Bridge->PerformAction(EIndigoAction::Unfold); }
void UIndigoInterfaceWidget::OnDeliver() { if (Bridge) Bridge->PerformAction(EIndigoAction::Deliver); }
void UIndigoInterfaceWidget::OnReview() { if (Bridge) Bridge->PerformAction(EIndigoAction::Review); }
void UIndigoInterfaceWidget::OnBack() { if (Bridge) Bridge->PerformAction(EIndigoAction::Back); }
void UIndigoInterfaceWidget::OnShip() { if (Bridge) Bridge->PerformAction(EIndigoAction::Ship); }
void UIndigoInterfaceWidget::OnRestart() { if (Bridge) Bridge->RestartWorkbench(); }
void UIndigoInterfaceWidget::OnExitWorkbench() { if (Bridge) Bridge->LeaveWorkbench(); }
void UIndigoInterfaceWidget::OnCameraTable() { if (Bridge) Bridge->SwitchWorkbenchCamera(EIndigoCamera::Table); }
void UIndigoInterfaceWidget::OnCameraCloseup() { if (Bridge) Bridge->SwitchWorkbenchCamera(EIndigoCamera::Closeup); }
void UIndigoInterfaceWidget::OnCameraQuilt() { if (Bridge) Bridge->SwitchWorkbenchCamera(EIndigoCamera::Quilt); }
