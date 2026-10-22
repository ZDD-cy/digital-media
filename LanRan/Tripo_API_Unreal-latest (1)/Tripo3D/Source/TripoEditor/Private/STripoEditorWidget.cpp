
#include "STripoEditorWidget.h"
#include "AssetThumbnail.h"
#include "Styling/AppStyle.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "TripoAssetPaths.h"
#include "TripoBPEditorLibrary.h"
#include "TripoBPLibrary.h"
#include "TripoEditor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/SlateTypes.h"
#include "IImageWrapperModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

#define LOCTEXT_NAMESPACE "STripoEditorWidget"

namespace TripoEditorUI
{
	static const TCHAR* SettingsSection = TEXT("/Script/TripoEditor.TripoEditorSettings");
	static const FName PollTickerName(TEXT("TripoEditorPoll"));
	// BorderBackgroundColor treats these as LINEAR colors and gamma-corrects them on display,
	// so specify the intended sRGB swatch and let the engine convert back to linear.
	static const FLinearColor SurfaceColor = FLinearColor::FromSRGBColor(FColor(19, 19, 19));
	static const FLinearColor SurfaceHoverColor = FLinearColor::FromSRGBColor(FColor(28, 28, 28));
	static const FLinearColor BorderColor = FLinearColor::FromSRGBColor(FColor(19, 19, 19));
	static const FLinearColor PrimaryColor(250.0f / 255.0f, 204.0f / 255.0f, 21.0f / 255.0f);
	static const FLinearColor TextPrimaryColor(224.0f / 255.0f, 224.0f / 255.0f, 224.0f / 255.0f);
	static const FLinearColor TextSecondaryColor(153.0f / 255.0f, 153.0f / 255.0f, 153.0f / 255.0f);
	static const FLinearColor TextMutedColor(112.0f / 255.0f, 112.0f / 255.0f, 112.0f / 255.0f);
	static const FLinearColor ConfirmedColor(52.0f / 255.0f, 211.0f / 255.0f, 153.0f / 255.0f);

	// Unity's editor window is usable from roughly 420 px wide. Keep enough room
	// for the API balance row, two generation tabs, and settings controls before
	// Slate starts clipping the panel instead of compressing its labels.
	static constexpr float MinPanelWidth = 460.0f;
	static constexpr int32 QuadFaceLimit = 150000;
	static constexpr int32 MaxConsecutivePollFailures = 3;

	static int32 MaxFaceLimit(const FString& ModelId, bool bQuad)
	{
		if (ModelId == TEXT("P1-20260311")) return 20000;
		if (bQuad) return QuadFaceLimit;
		if (ModelId == TEXT("v2.5-20250123")) return 500000;
		return 2000000;
	}

	static FString FormatRequestError(int32 ErrorCode, const FString& ErrorMessage, const TCHAR* Fallback)
	{
		const FString Detail = ErrorMessage.IsEmpty() ? FString(Fallback) : ErrorMessage;
		if (ErrorCode == 2000) return FString::Printf(TEXT("Concurrency limit reached: %s"), *Detail);
		if (ErrorCode == 1007) return FString::Printf(TEXT("Rate limit exceeded: %s"), *Detail);
		return Detail;
	}

	static const FSlateBrush* SurfaceBrush()
	{
		return FAppStyle::Get().GetBrush("WhiteBrush");
	}

	static const FSlateBrush* IconBrush(const FName Name)
	{
		return FTripoEditorStyle::Get().GetBrush(Name);
	}

	// Neutral grey button whose hovered/pressed states stay grey instead of the
	// engine's blue highlight. Slightly lighter on hover/press for click feedback.
	static const FButtonStyle& GreyButtonStyle()
	{
		auto MakeBox = [](const FColor& Color)
		{
			FSlateColorBrush Brush(FLinearColor::White);
			Brush.TintColor = FLinearColor::FromSRGBColor(Color);
			return Brush;
		};
		static const FButtonStyle Style = FButtonStyle()
			.SetNormal(MakeBox(FColor(45, 45, 45)))
			.SetHovered(MakeBox(FColor(58, 58, 58)))
			.SetPressed(MakeBox(FColor(38, 38, 38)))
			.SetDisabled(MakeBox(FColor(30, 30, 30)))
			.SetNormalPadding(FMargin(8.0f, 6.0f))
			.SetPressedPadding(FMargin(8.0f, 6.0f));
		return Style;
	}

	static FText Label(const TCHAR* Text)
	{
		return FText::FromString(Text);
	}
}

enum class ETripoEditorJobStage : uint8
{
	Uploading,
	Submitting,
	Generating,
	Converting,
	Downloading,
	Importing,
	Completed,
	Failed
};

struct FTripoEditorJob
{
	int32 Number = 0;
	bool bTextToModel = true;
	FString InputSummary;
	FString Prompt;
	FString ImagePath;
	FString ApiKey;
	FString ModelId;
	FString AssetBaseName;
	int32 FaceLimit = 2000000;
	bool bTexture = true;
	bool bPbr = true;
	FString TextureQuality = TEXT("standard");
	bool bAutoSize = false;
	bool bQuad = false;
	FString GenerationTaskId;
	FString ConversionTaskId;
	FString OutputAssetPath;
	FString ErrorMessage;
	FString LastServerStatus;
	ETripoEditorJobStage Stage = ETripoEditorJobStage::Submitting;
	int32 GenerationProgress = 0;
	int32 ConversionProgress = 0;
	float Elapsed = 0.0f;
	float SinceLastPoll = 0.0f;
	float PollIntervalSeconds = 1.0f;
	int32 ConsecutivePollFailures = 0;
	bool bPollInFlight = false;
	FTSTicker::FDelegateHandle PollTickerHandle;

	bool IsTerminal() const
	{
		return Stage == ETripoEditorJobStage::Completed || Stage == ETripoEditorJobStage::Failed;
	}

	FString CurrentTaskId() const
	{
		return ConversionTaskId.IsEmpty() ? GenerationTaskId : ConversionTaskId;
	}

	float Progress() const
	{
		switch (Stage)
		{
		case ETripoEditorJobStage::Uploading: return 0.03f;
		case ETripoEditorJobStage::Submitting: return 0.08f;
		case ETripoEditorJobStage::Generating: return FMath::Lerp(0.10f, bQuad ? 0.88f : 0.50f, GenerationProgress / 100.0f);
		case ETripoEditorJobStage::Converting: return FMath::Lerp(0.50f, 0.88f, ConversionProgress / 100.0f);
		case ETripoEditorJobStage::Downloading: return 0.92f;
		case ETripoEditorJobStage::Importing: return 0.97f;
		case ETripoEditorJobStage::Completed: return 1.0f;
		default: return 0.0f;
		}
	}
};

static FString TripoJobStageText(const FTripoEditorJob& Job)
{
	switch (Job.Stage)
	{
	case ETripoEditorJobStage::Uploading: return TEXT("Uploading image...");
	case ETripoEditorJobStage::Submitting: return TEXT("Submitting...");
	case ETripoEditorJobStage::Generating: return FString::Printf(TEXT("Generating %d%%"), Job.GenerationProgress);
	case ETripoEditorJobStage::Converting: return FString::Printf(TEXT("Converting %d%%"), Job.ConversionProgress);
	case ETripoEditorJobStage::Downloading: return TEXT("Downloading...");
	case ETripoEditorJobStage::Importing: return TEXT("Importing...");
	case ETripoEditorJobStage::Completed: return TEXT("Completed");
	case ETripoEditorJobStage::Failed: return TEXT("Failed");
	default: return TEXT("Unknown");
	}
}

void STripoEditorWidget::Construct(const FArguments&)
{
	ModelOptions = {
		MakeShared<FString>(TEXT("v3.1-20260211")),
		MakeShared<FString>(TEXT("v3.0-20250812")),
		MakeShared<FString>(TEXT("v2.5-20250123")),
		MakeShared<FString>(TEXT("P1-20260311"))
	};
	TextureQualityOptions = { MakeShared<FString>(TEXT("standard")), MakeShared<FString>(TEXT("detailed")) };
	SelectedModelId = *ModelOptions[0];
	SelectedTextureQuality = *TextureQualityOptions[0];
	GConfig->GetString(TripoEditorUI::SettingsSection, TEXT("ApiKey"), ApiKey, GEditorPerProjectIni);
	GConfig->GetString(TripoEditorUI::SettingsSection, TEXT("Model"), SelectedModelId, GEditorPerProjectIni);
	GConfig->GetBool(TripoEditorUI::SettingsSection, TEXT("Texture"), bTexture, GEditorPerProjectIni);
	GConfig->GetBool(TripoEditorUI::SettingsSection, TEXT("Pbr"), bPbr, GEditorPerProjectIni);
	GConfig->GetBool(TripoEditorUI::SettingsSection, TEXT("AutoSize"), bAutoSize, GEditorPerProjectIni);
	GConfig->GetBool(TripoEditorUI::SettingsSection, TEXT("Quad"), bQuad, GEditorPerProjectIni);
	GConfig->GetBool(TripoEditorUI::SettingsSection, TEXT("AdvancedExpanded"), bAdvancedExpanded, GEditorPerProjectIni);
	GConfig->GetString(TripoEditorUI::SettingsSection, TEXT("TextureQuality"), SelectedTextureQuality, GEditorPerProjectIni);
	GConfig->GetInt(TripoEditorUI::SettingsSection, TEXT("ActiveTab"), ActiveTab, GEditorPerProjectIni);
	GConfig->GetInt(TripoEditorUI::SettingsSection, TEXT("V3TriangleFaceLimit"), V3TriangleFaceLimit, GEditorPerProjectIni);
	GConfig->GetInt(TripoEditorUI::SettingsSection, TEXT("V3QuadFaceLimit"), V3QuadFaceLimit, GEditorPerProjectIni);
	GConfig->GetInt(TripoEditorUI::SettingsSection, TEXT("V25FaceLimit"), V25FaceLimit, GEditorPerProjectIni);
	GConfig->GetInt(TripoEditorUI::SettingsSection, TEXT("V25QuadFaceLimit"), V25QuadFaceLimit, GEditorPerProjectIni);
	GConfig->GetInt(TripoEditorUI::SettingsSection, TEXT("P1FaceLimit"), P1FaceLimit, GEditorPerProjectIni);
	if (!ModelOptions.ContainsByPredicate([this](const FStringItem& Item) { return Item.IsValid() && *Item == SelectedModelId; })) SelectedModelId = *ModelOptions[0];
	if (!TextureQualityOptions.ContainsByPredicate([this](const FStringItem& Item) { return Item.IsValid() && *Item == SelectedTextureQuality; })) SelectedTextureQuality = *TextureQualityOptions[0];
	bQuad = bQuad && IsQuadModel();
	V3TriangleFaceLimit = FMath::Clamp(V3TriangleFaceLimit, 500, 2000000);
	V3QuadFaceLimit = FMath::Clamp(V3QuadFaceLimit, 500, TripoEditorUI::QuadFaceLimit);
	V25FaceLimit = FMath::Clamp(V25FaceLimit, 500, 500000);
	V25QuadFaceLimit = FMath::Clamp(V25QuadFaceLimit, 500, TripoEditorUI::QuadFaceLimit);
	P1FaceLimit = FMath::Clamp(P1FaceLimit, 500, 20000);
	ThumbnailPool = MakeShared<FAssetThumbnailPool>(8);

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(16.0f)
		[
			// Keep the content's minimum width while retaining the normal vertical scroll behavior.
			SNew(SBox)
			.MinDesiredWidth(TripoEditorUI::MinPanelWidth)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[BuildApiHeader()]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)[BuildGenerationPanel()]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[BuildSettingsPanel()]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[BuildTaskQueue()]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[BuildResultPanel()]
			]
		]
	];

	SetActiveTab(FMath::Clamp(ActiveTab, 0, 1));
	if (!ApiKey.IsEmpty())
	{
		bApiKeyConfirmed = true;
		RefreshBalance();
	}
	UpdateTaskQueue();
}

STripoEditorWidget::~STripoEditorWidget()
{
	StopPolling();
	StopBalanceRefresh();
	SavePreferences();
	GenerationJobs.Empty();
	SelectedImageBrush.Reset();
	ResultThumbnail.Reset();
	ThumbnailPool.Reset();
}

TSharedRef<SWidget> STripoEditorWidget::BuildApiHeader()
{
	return SNew(SBorder)
		.Padding(12.0f)
		.BorderImage(TripoEditorUI::SurfaceBrush())
		// Confirmation is signalled by the key and check icons turning green. The
		// header keeps the neutral surface fill so the whole bar does not tint.
		.BorderBackgroundColor(TripoEditorUI::SurfaceColor)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(TripoEditorUI::IconBrush(TEXT("Tripo.Icon.Key")))
				.ColorAndOpacity_Lambda([this]() { return bApiKeyConfirmed ? TripoEditorUI::ConfirmedColor : TripoEditorUI::TextMutedColor; })
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(10.0f, 0.0f)
			[
				SAssignNew(ApiKeyTextBox, SEditableTextBox)
				.Text_Lambda([this]() { return FText::FromString(ApiKey); })
				.OnTextChanged_Lambda([this](const FText& Text) { ApiKey = Text.ToString(); })
				.IsReadOnly_Lambda([this]() { return bApiKeyConfirmed; })
				.IsPassword(true)
				.HintText(LOCTEXT("ApiKeyHint", "API key, begins with tsk_..."))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.ToolTipText_Lambda([this]() { return bApiKeyConfirmed ? LOCTEXT("EditApiKey", "Edit API key") : LOCTEXT("ConfirmApiKey", "Confirm API key"); })
				.OnClicked(this, &STripoEditorWidget::OnConfirmApiKeyClicked)
				[
					SNew(SImage)
					.Image_Lambda([this]() { return TripoEditorUI::IconBrush(bApiKeyConfirmed ? TEXT("Tripo.Icon.Check") : TEXT("Tripo.Icon.Enter")); })
					.ColorAndOpacity_Lambda([this]() { return bApiKeyConfirmed ? TripoEditorUI::ConfirmedColor : TripoEditorUI::PrimaryColor; })
				]
			]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10.0f, 0.0f).VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([this]() { return bApiKeyConfirmed ? EVisibility::Visible : EVisibility::Collapsed; })
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SImage).Image(TripoEditorUI::IconBrush(TEXT("Tripo.Icon.Credits"))).ColorAndOpacity(TripoEditorUI::TextSecondaryColor)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f).VAlign(VAlign_Center)
					[
						SAssignNew(BalanceText, STextBlock).Text(LOCTEXT("Balance", "----")).ColorAndOpacity(TripoEditorUI::PrimaryColor)
					]
				]
		];
}

TSharedRef<SWidget> STripoEditorWidget::BuildGenerationPanel()
{
	return SNew(SBorder)
		.Padding(14.0f)
		.BorderImage(TripoEditorUI::SurfaceBrush())
		.BorderBackgroundColor(TripoEditorUI::SurfaceColor)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton")).OnClicked(this, &STripoEditorWidget::OnTextTabClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SImage).Image(TripoEditorUI::IconBrush(TEXT("Tripo.Icon.Text"))).ColorAndOpacity_Lambda([this]() { return ActiveTab == 0 ? TripoEditorUI::TextPrimaryColor : TripoEditorUI::TextSecondaryColor; })]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("TextTab", "TEXT TO MODEL")).ColorAndOpacity_Lambda([this]() { return ActiveTab == 0 ? TripoEditorUI::TextPrimaryColor : TripoEditorUI::TextSecondaryColor; })]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton")).OnClicked(this, &STripoEditorWidget::OnImageTabClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SImage).Image(TripoEditorUI::IconBrush(TEXT("Tripo.Icon.Image"))).ColorAndOpacity_Lambda([this]() { return ActiveTab == 1 ? TripoEditorUI::TextPrimaryColor : TripoEditorUI::TextSecondaryColor; })]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("ImageTab", "IMAGE TO MODEL")).ColorAndOpacity_Lambda([this]() { return ActiveTab == 1 ? TripoEditorUI::TextPrimaryColor : TripoEditorUI::TextSecondaryColor; })]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
			[
				SAssignNew(GenerationSwitcher, SWidgetSwitcher)
				+ SWidgetSwitcher::Slot()[BuildTextPanel()]
				+ SWidgetSwitcher::Slot()[BuildImagePanel()]
			]
		];
}

TSharedRef<SWidget> STripoEditorWidget::BuildTextPanel()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("PromptLabel", "PROMPT")).ColorAndOpacity(TripoEditorUI::TextSecondaryColor)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
		[
			SNew(SBorder).Padding(12.0f).BorderImage(TripoEditorUI::SurfaceBrush()).BorderBackgroundColor(TripoEditorUI::SurfaceHoverColor)
			[
				SNew(SBox).MinDesiredHeight(120.0f)
				[
					SAssignNew(PromptTextBox, SMultiLineEditableTextBox)
					.Text_Lambda([this]() { return FText::FromString(TextPrompt); })
					.OnTextChanged_Lambda([this](const FText& Text) { TextPrompt = Text.ToString(); })
					.HintText(LOCTEXT("PromptHint", "Enter the text prompt here..."))
					.AllowMultiLine(true)
					.ClearKeyboardFocusOnCommit(false)
				]
			]
		]
		;
}

TSharedRef<SWidget> STripoEditorWidget::BuildImagePanel()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("ImageLabel", "REFERENCE IMAGE")).ColorAndOpacity(TripoEditorUI::TextSecondaryColor)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
		[
			SNew(SButton)
			.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
			.OnClicked(this, &STripoEditorWidget::OnChooseImageClicked)
			[
				SNew(SBorder)
				.Padding(8.0f)
				.BorderImage(TripoEditorUI::SurfaceBrush())
				.BorderBackgroundColor(TripoEditorUI::SurfaceHoverColor)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBox).HeightOverride(160.0f)
						[
							SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
							[
								SNew(SImage).Image_Lambda([this]() { return SelectedImageBrush.Get(); })
							]
						]
					]
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Visibility_Lambda([this]() { return SelectedImageBrush.IsValid() ? EVisibility::Collapsed : EVisibility::Visible; })
						.Image(TripoEditorUI::IconBrush(TEXT("Tripo.Icon.ImagePlus")))
						.ColorAndOpacity(TripoEditorUI::TextMutedColor)
					]
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(4.0f)
					[
						SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ImagePath.IsEmpty() ? TEXT("Choose a PNG or JPEG image") : FPaths::GetCleanFilename(ImagePath)); }).ColorAndOpacity(TripoEditorUI::TextSecondaryColor)
					]
				]
			]
		];
}

TSharedRef<SWidget> STripoEditorWidget::BuildSettingsPanel()
{
	int32 SelectedModelIndex = 0;
	for (int32 Index = 0; Index < ModelOptions.Num(); ++Index)
	{
		if (ModelOptions[Index].IsValid() && *ModelOptions[Index] == SelectedModelId)
		{
			SelectedModelIndex = Index;
			break;
		}
	}
	int32 SelectedTextureQualityIndex = 0;
	for (int32 Index = 0; Index < TextureQualityOptions.Num(); ++Index)
	{
		if (TextureQualityOptions[Index].IsValid() && *TextureQualityOptions[Index] == SelectedTextureQuality)
		{
			SelectedTextureQualityIndex = Index;
			break;
		}
	}
	return SNew(SBorder)
		.Padding(14.0f)
		.BorderImage(TripoEditorUI::SurfaceBrush())
		.BorderBackgroundColor(TripoEditorUI::SurfaceColor)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("Settings", "GENERATION SETTINGS")).ColorAndOpacity(TripoEditorUI::TextPrimaryColor).Font(FAppStyle::Get().GetFontStyle("BoldFont"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder).Padding(10.0f).BorderImage(TripoEditorUI::SurfaceBrush()).BorderBackgroundColor(TripoEditorUI::SurfaceHoverColor)
				[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(LOCTEXT("Model", "MODEL")).ColorAndOpacity(TripoEditorUI::TextSecondaryColor)]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(12, 0, 0, 0)
				[
					SAssignNew(ModelComboBox, SComboBox<FStringItem>)
					.OptionsSource(&ModelOptions)
					.OnGenerateWidget(this, &STripoEditorWidget::MakeComboItem)
					.OnSelectionChanged_Lambda([this](FStringItem Item, ESelectInfo::Type)
					{
						if (Item.IsValid())
						{
							SelectedModelId = *Item;
							if (!IsQuadModel()) bQuad = false;
							SavePreferences();
						}
					})
					.InitiallySelectedItem(ModelOptions.IsValidIndex(SelectedModelIndex) ? ModelOptions[SelectedModelIndex] : ModelOptions[0])
					[
						SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedModelId); })
					]
				]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SNew(SButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton")).Text_Lambda([this]() { return FText::FromString(bAdvancedExpanded ? TEXT("v  ADVANCED") : TEXT(">  ADVANCED")); }).OnClicked_Lambda([this]() { bAdvancedExpanded = !bAdvancedExpanded; SavePreferences(); return FReply::Handled(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SNew(SBox).Visibility_Lambda([this]() { return bAdvancedExpanded ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("AdvancedHint", "Advanced fields follow the selected model capabilities.")).ColorAndOpacity(TripoEditorUI::TextMutedColor)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(LOCTEXT("FaceLimit", "Face limit"))]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(12, 0, 0, 0)
						[
							SAssignNew(FaceLimitSpinBox, SSpinBox<int32>)
							.MinValue(500)
							.MaxValue_Lambda([this]() { return TripoEditorUI::MaxFaceLimit(GetSelectedModelId(), bQuad); })
							.Value_Lambda([this]() { return GetPreferredFaceLimit(SelectedModelId, bQuad); })
							.OnValueChanged_Lambda([this](int32 Value) { SetPreferredFaceLimit(SelectedModelId, bQuad, Value); SavePreferences(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SCheckBox).IsChecked_Lambda([this]() { return bTexture ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }).OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTexture = State == ECheckBoxState::Checked; SavePreferences(); }).Content()[SNew(STextBlock).Text(LOCTEXT("Texture", "Texture"))]]
						+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SCheckBox).IsChecked_Lambda([this]() { return bPbr ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }).OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bPbr = State == ECheckBoxState::Checked; SavePreferences(); }).Content()[SNew(STextBlock).Text(LOCTEXT("Pbr", "PBR"))]]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(LOCTEXT("Quality", "Texture quality"))]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(12, 0, 0, 0)
						[
							SAssignNew(TextureQualityComboBox, SComboBox<FStringItem>)
							.OptionsSource(&TextureQualityOptions)
							.OnGenerateWidget_Lambda([](FStringItem Item) { return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : TEXT(""))); })
							.OnSelectionChanged_Lambda([this](FStringItem Item, ESelectInfo::Type) { if (Item.IsValid()) { SelectedTextureQuality = *Item; SavePreferences(); } })
							.InitiallySelectedItem(TextureQualityOptions[SelectedTextureQualityIndex])
							[
								SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedTextureQuality); })
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SCheckBox).IsEnabled_Lambda([this]() { return IsAdvancedModel(); }).IsChecked_Lambda([this]() { return bAutoSize ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }).OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bAutoSize = State == ECheckBoxState::Checked; SavePreferences(); }).Content()[SNew(STextBlock).Text(LOCTEXT("AutoSize", "Auto size"))]]
						+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SCheckBox).IsEnabled_Lambda([this]() { return IsQuadModel(); }).IsChecked_Lambda([this]() { return bQuad ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }).OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bQuad = State == ECheckBoxState::Checked; SavePreferences(); }).Content()[SNew(STextBlock).Text(LOCTEXT("Quad", "Quad topology"))]]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
			[
				SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(0.0f, 6.0f)).ButtonStyle(&TripoEditorUI::GreyButtonStyle()).IsEnabled_Lambda([this]() { return bApiKeyConfirmed; }).OnClicked(this, &STripoEditorWidget::OnGenerateClicked)
				[
					SNew(STextBlock).Text(LOCTEXT("Generate", "GENERATE")).ColorAndOpacity(FLinearColor::White).Font(FAppStyle::Get().GetFontStyle("BoldFont"))
				]
			]
		];
}

TSharedRef<SWidget> STripoEditorWidget::BuildResultPanel()
{
	return SNew(SBorder)
		.Padding(14.0f)
		.BorderImage(TripoEditorUI::SurfaceBrush())
		.BorderBackgroundColor(TripoEditorUI::SurfaceColor)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SImage).Image(TripoEditorUI::IconBrush(TEXT("Tripo.Icon.Preview"))).ColorAndOpacity(TripoEditorUI::TextSecondaryColor)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("Result", "RESULT PREVIEW")).ColorAndOpacity(TripoEditorUI::TextPrimaryColor).Font(FAppStyle::Get().GetFontStyle("BoldFont"))]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
			[
				SNew(SBorder).Padding(1.0f).BorderImage(TripoEditorUI::SurfaceBrush()).BorderBackgroundColor(TripoEditorUI::SurfaceHoverColor)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SAssignNew(PreviewBox, SBox)
						.Visibility_Lambda([this]() { return ResultThumbnail.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
						.WidthOverride(256.0f)
						.HeightOverride(256.0f)
					]
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SImage).Visibility_Lambda([this]() { return ResultThumbnail.IsValid() ? EVisibility::Collapsed : EVisibility::Visible; }).Image(TripoEditorUI::IconBrush(TEXT("Tripo.Icon.Box"))).ColorAndOpacity(TripoEditorUI::BorderColor)
					]
				]
			]
		];
}

TSharedRef<SWidget> STripoEditorWidget::BuildTaskQueue()
{
	return SNew(SBorder)
		.Padding(14.0f)
		.Visibility_Lambda([this]() { return GenerationJobs.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
		.BorderImage(TripoEditorUI::SurfaceBrush())
		.BorderBackgroundColor(TripoEditorUI::SurfaceColor)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SAssignNew(TaskQueueHeader, STextBlock).Text(LOCTEXT("TaskQueue", "TASK QUEUE (0)")).ColorAndOpacity(TripoEditorUI::TextPrimaryColor).Font(FAppStyle::Get().GetFontStyle("BoldFont"))]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(LOCTEXT("ClearFinished", "Clear Finished")).OnClicked(this, &STripoEditorWidget::OnClearFinishedClicked)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SAssignNew(TaskListBox, SVerticalBox)
			]
		];
}

TSharedRef<SWidget> STripoEditorWidget::MakeTaskWidget(const TSharedPtr<FTripoEditorJob>& Job) const
{
	const bool bComplete = Job.IsValid() && Job->Stage == ETripoEditorJobStage::Completed;
	return SNew(SBorder)
		.Padding(10.0f)
		.BorderImage(TripoEditorUI::SurfaceBrush())
		.BorderBackgroundColor(TripoEditorUI::SurfaceHoverColor)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.52f)[SNew(STextBlock).Text_Lambda([Job]() { return FText::FromString(FString::Printf(TEXT("#%d  %s  %s"), Job->Number, Job->bTextToModel ? TEXT("TEXT") : TEXT("IMAGE"), *Job->ModelId)); })]
				+ SHorizontalBox::Slot().FillWidth(0.48f)[SNew(STextBlock).Text_Lambda([Job]() { return FText::FromString(TripoJobStageText(*Job)); })]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)[SNew(STextBlock).Text_Lambda([Job]() { return FText::FromString(Job->InputSummary); }).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
			[
				SNew(SBox)
				.Visibility_Lambda([Job]() { return Job.IsValid() && !Job->IsTerminal() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SProgressBar).Percent_Lambda([Job]() { return Job.IsValid() ? Job->Progress() : 0.0f; })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Visibility_Lambda([Job]() { return Job.IsValid() && Job->Stage == ETripoEditorJobStage::Completed ? EVisibility::Visible : EVisibility::Collapsed; })
				.ColorAndOpacity(FLinearColor(0.20f, 0.85f, 0.35f, 1.0f))
				.Text(LOCTEXT("JobCompleted", "SUCCESS - Imported"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[SNew(STextBlock).Text_Lambda([Job]() { const FString Detail = !Job->ErrorMessage.IsEmpty() ? Job->ErrorMessage : !Job->OutputAssetPath.IsEmpty() ? Job->OutputAssetPath : Job->CurrentTaskId(); return FText::FromString(Detail); }).AutoWrapText(true)]
		];
}

FReply STripoEditorWidget::OnClearFinishedClicked()
{
	GenerationJobs.RemoveAll([](const TSharedPtr<FTripoEditorJob>& Job) { return !Job.IsValid() || Job->IsTerminal(); });
	UpdateTaskQueue();
	return FReply::Handled();
}

TSharedRef<SWidget> STripoEditorWidget::MakeComboItem(FStringItem Item) const
{
	return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : TEXT("")));
}

FReply STripoEditorWidget::OnTextTabClicked()
{
	SetActiveTab(0);
	SavePreferences();
	return FReply::Handled();
}

FReply STripoEditorWidget::OnImageTabClicked()
{
	SetActiveTab(1);
	SavePreferences();
	return FReply::Handled();
}

void STripoEditorWidget::SetActiveTab(int32 TabIndex)
{
	ActiveTab = TabIndex;
	if (GenerationSwitcher.IsValid()) GenerationSwitcher->SetActiveWidgetIndex(ActiveTab);
}

FReply STripoEditorWidget::OnConfirmApiKeyClicked()
{
	if (bApiKeyConfirmed)
	{
		bApiKeyConfirmed = false;
		StopBalanceRefresh();
		if (BalanceText.IsValid()) BalanceText->SetText(LOCTEXT("BalanceCleared", "Balance: -"));
		GConfig->SetString(TripoEditorUI::SettingsSection, TEXT("ApiKey"), TEXT(""), GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);
		SetStatus(TEXT("API key editing enabled"));
		return FReply::Handled();
	}
	ApiKey.TrimStartAndEndInline();
	bApiKeyConfirmed = ApiKey.StartsWith(TEXT("tsk_")) && ApiKey.Len() > 4;
	if (bApiKeyConfirmed)
	{
		GConfig->SetString(TripoEditorUI::SettingsSection, TEXT("ApiKey"), *ApiKey, GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);
		SetStatus(TEXT("API key confirmed"));
		RefreshBalance();
	}
	else
	{
		SetStatus(TEXT("Enter a valid Tripo API key (tsk_...)"), true);
	}
	return FReply::Handled();
}

FReply STripoEditorWidget::OnChooseImageClicked()
{
	const TWeakPtr<STripoEditorWidget> WidgetWeak = StaticCastSharedRef<STripoEditorWidget>(AsShared());
	UTripoBPEditorLibrary::ShowFileDialogNative(false, TEXT("Choose Tripo reference image"), FPaths::ProjectContentDir(), TEXT(""), TEXT("Images|*.png;*.jpg;*.jpeg"), UTripoBPEditorLibrary::FOnTripoFileSelectedNative::CreateLambda([WidgetWeak](bool bSuccess, const FString& FilePath)
	{
		if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak.Pin())
		{
			if (bSuccess)
			{
				Pinned->ImagePath = FilePath;
				Pinned->UpdateSelectedImagePreview();
			}
		}
	}));
	return FReply::Handled();
}

FReply STripoEditorWidget::OnGenerateClicked()
{
	if (!bApiKeyConfirmed) return FReply::Handled();
	const bool bTextToModel = ActiveTab == 0;
	if (bTextToModel && TextPrompt.TrimStartAndEnd().IsEmpty())
	{
		SetStatus(TEXT("Enter a prompt before generating"), true);
		return FReply::Handled();
	}
	if (!bTextToModel)
	{
		const FString Extension = FPaths::GetExtension(ImagePath).ToLower();
		if (ImagePath.IsEmpty() || !FPaths::FileExists(ImagePath) || (Extension != TEXT("png") && Extension != TEXT("jpg") && Extension != TEXT("jpeg")))
		{
			SetStatus(TEXT("Choose a valid PNG or JPEG image before generating"), true);
			return FReply::Handled();
		}
	}
	TSharedPtr<FTripoEditorJob> Job = CreateGenerationJob(bTextToModel);
	if (!Job.IsValid()) return FReply::Handled();
	SavePreferences();
	SetStatus(Job->bTextToModel ? TEXT("Submitting text generation...") : TEXT("Uploading image..."));
	ScheduleBalanceRefresh();
	if (Job->bTextToModel) StartTextGeneration(Job); else StartImageUpload(Job);
	return FReply::Handled();
}

void STripoEditorWidget::SetStatus(const FString& Message, bool bError)
{
	if (StatusText.IsValid()) StatusText->SetText(FText::FromString(Message));
	RefreshTaskSummary();
	if (bError)
	{
		UE_LOG(LogTemp, Error, TEXT("[Tripo Editor] %s"), *Message);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Tripo Editor] %s"), *Message);
	}
}

void STripoEditorWidget::StopPolling()
{
	for (const TSharedPtr<FTripoEditorJob>& Job : GenerationJobs)
	{
		StopJobPolling(Job);
	}
}

void STripoEditorWidget::StopJobPolling(const TSharedPtr<FTripoEditorJob>& Job)
{
	if (Job.IsValid() && Job->PollTickerHandle.IsValid())
	{
		FTSTicker::RemoveTicker(Job->PollTickerHandle);
		Job->PollTickerHandle.Reset();
	}
	if (Job.IsValid()) Job->bPollInFlight = false;
}

void STripoEditorWidget::SavePreferences() const
{
	GConfig->SetString(TripoEditorUI::SettingsSection, TEXT("ApiKey"), *ApiKey, GEditorPerProjectIni);
	GConfig->SetString(TripoEditorUI::SettingsSection, TEXT("Model"), *SelectedModelId, GEditorPerProjectIni);
	GConfig->SetString(TripoEditorUI::SettingsSection, TEXT("TextureQuality"), *SelectedTextureQuality, GEditorPerProjectIni);
	GConfig->SetBool(TripoEditorUI::SettingsSection, TEXT("Texture"), bTexture, GEditorPerProjectIni);
	GConfig->SetBool(TripoEditorUI::SettingsSection, TEXT("Pbr"), bPbr, GEditorPerProjectIni);
	GConfig->SetBool(TripoEditorUI::SettingsSection, TEXT("AutoSize"), bAutoSize, GEditorPerProjectIni);
	GConfig->SetBool(TripoEditorUI::SettingsSection, TEXT("Quad"), bQuad, GEditorPerProjectIni);
	GConfig->SetBool(TripoEditorUI::SettingsSection, TEXT("AdvancedExpanded"), bAdvancedExpanded, GEditorPerProjectIni);
	GConfig->SetInt(TripoEditorUI::SettingsSection, TEXT("V3TriangleFaceLimit"), V3TriangleFaceLimit, GEditorPerProjectIni);
	GConfig->SetInt(TripoEditorUI::SettingsSection, TEXT("V3QuadFaceLimit"), V3QuadFaceLimit, GEditorPerProjectIni);
	GConfig->SetInt(TripoEditorUI::SettingsSection, TEXT("V25FaceLimit"), V25FaceLimit, GEditorPerProjectIni);
	GConfig->SetInt(TripoEditorUI::SettingsSection, TEXT("V25QuadFaceLimit"), V25QuadFaceLimit, GEditorPerProjectIni);
	GConfig->SetInt(TripoEditorUI::SettingsSection, TEXT("P1FaceLimit"), P1FaceLimit, GEditorPerProjectIni);
	GConfig->SetInt(TripoEditorUI::SettingsSection, TEXT("ActiveTab"), ActiveTab, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

void STripoEditorWidget::RefreshBalance()
{
	const TWeakPtr<STripoEditorWidget> WidgetWeak = StaticCastSharedRef<STripoEditorWidget>(AsShared());
	UTripoBPLibrary::GetUserBalanceDetailsNative(ApiKey, [WidgetWeak](bool bSuccess, int32 Balance, int32 Frozen)
	{
		if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak.Pin())
		{
			if (Pinned->BalanceText.IsValid()) Pinned->BalanceText->SetText(FText::FromString(bSuccess ? FString::Printf(TEXT("Balance: %d  Frozen: %d"), Balance, Frozen) : TEXT("Balance: unavailable")));
		}
	});
}

void STripoEditorWidget::ScheduleBalanceRefresh(float InitialDelaySeconds)
{
	if (!bApiKeyConfirmed || ApiKey.IsEmpty()) return;

	BalanceRefreshElapsed = 0.0f;
	BalanceRefreshDelay = FMath::Max(0.0f, InitialDelaySeconds);
	bBalanceRefreshPending = true;
	if (BalanceRefreshTickerHandle.IsValid()) return;

	const TWeakPtr<STripoEditorWidget> WidgetWeak = StaticCastSharedRef<STripoEditorWidget>(AsShared());
	BalanceRefreshTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WidgetWeak](float DeltaSeconds)
	{
		const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak.Pin();
		if (!Pinned.IsValid() || !Pinned->bApiKeyConfirmed) return false;
		if (Pinned->bBalanceRefreshRequestInFlight) return true;
		if (!Pinned->bBalanceRefreshPending)
		{
			if (Pinned->GetActiveJobCount() == 0)
			{
				Pinned->BalanceRefreshTickerHandle.Reset();
				return false;
			}
			Pinned->bBalanceRefreshPending = true;
			Pinned->BalanceRefreshElapsed = 0.0f;
			Pinned->BalanceRefreshDelay = 5.0f;
		}

		Pinned->BalanceRefreshElapsed += DeltaSeconds;
		if (Pinned->BalanceRefreshElapsed < Pinned->BalanceRefreshDelay) return true;

		Pinned->bBalanceRefreshPending = false;
		Pinned->bBalanceRefreshRequestInFlight = true;
		const TWeakPtr<STripoEditorWidget> RequestWidgetWeak = Pinned;
		UTripoBPLibrary::GetUserBalanceDetailsNative(Pinned->ApiKey, [RequestWidgetWeak](bool bSuccess, int32 Balance, int32 Frozen)
		{
			if (const TSharedPtr<STripoEditorWidget> RequestWidget = RequestWidgetWeak.Pin())
			{
				RequestWidget->bBalanceRefreshRequestInFlight = false;
				if (RequestWidget->bApiKeyConfirmed && RequestWidget->BalanceText.IsValid())
				{
					RequestWidget->BalanceText->SetText(FText::FromString(bSuccess ? FString::Printf(TEXT("Balance: %d  Frozen: %d"), Balance, Frozen) : TEXT("Balance: unavailable")));
				}
				if (RequestWidget->GetActiveJobCount() > 0)
				{
					RequestWidget->bBalanceRefreshPending = true;
					RequestWidget->BalanceRefreshElapsed = 0.0f;
					RequestWidget->BalanceRefreshDelay = 5.0f;
				}
			}
		});
		return true;
	}), 0.0f);
}

void STripoEditorWidget::StopBalanceRefresh()
{
	if (BalanceRefreshTickerHandle.IsValid())
	{
		FTSTicker::RemoveTicker(BalanceRefreshTickerHandle);
		BalanceRefreshTickerHandle.Reset();
	}
	bBalanceRefreshRequestInFlight = false;
	bBalanceRefreshPending = false;
}

FString STripoEditorWidget::GetSelectedModelId() const { return SelectedModelId; }
bool STripoEditorWidget::IsAdvancedModel() const { return SelectedModelId == TEXT("v3.1-20260211") || SelectedModelId == TEXT("v3.0-20250812"); }
bool STripoEditorWidget::IsQuadModel() const { return IsAdvancedModel() || SelectedModelId == TEXT("v2.5-20250123"); }
int32 STripoEditorWidget::GetPreferredFaceLimit(const FString& ModelId, bool bForQuad) const
{
	if (ModelId == TEXT("v2.5-20250123")) return bForQuad ? V25QuadFaceLimit : V25FaceLimit;
	if (ModelId == TEXT("P1-20260311")) return P1FaceLimit;
	return bForQuad ? V3QuadFaceLimit : V3TriangleFaceLimit;
}

void STripoEditorWidget::SetPreferredFaceLimit(const FString& ModelId, bool bForQuad, int32 Value)
{
	if (ModelId == TEXT("v2.5-20250123") && bForQuad) V25QuadFaceLimit = FMath::Clamp(Value, 500, TripoEditorUI::QuadFaceLimit);
	else if (ModelId == TEXT("v2.5-20250123")) V25FaceLimit = FMath::Clamp(Value, 500, 500000);
	else if (ModelId == TEXT("P1-20260311")) P1FaceLimit = FMath::Clamp(Value, 500, 20000);
	else if (bForQuad) V3QuadFaceLimit = FMath::Clamp(Value, 500, TripoEditorUI::QuadFaceLimit);
	else V3TriangleFaceLimit = FMath::Clamp(Value, 500, 2000000);
}

int32 STripoEditorWidget::GetFaceLimit() const
{
	return GetPreferredFaceLimit(SelectedModelId, bQuad);
}

TSharedPtr<FTripoEditorJob> STripoEditorWidget::CreateGenerationJob(bool bTextToModel)
{
	TSharedPtr<FTripoEditorJob> Job = MakeShared<FTripoEditorJob>();
	Job->Number = NextJobNumber++;
	Job->bTextToModel = bTextToModel;
	Job->InputSummary = bTextToModel ? TextPrompt.TrimStartAndEnd() : FPaths::GetCleanFilename(ImagePath);
	Job->Prompt = TextPrompt.TrimStartAndEnd();
	Job->ImagePath = ImagePath;
	Job->ApiKey = ApiKey;
	Job->ModelId = SelectedModelId;
	Job->bQuad = IsQuadModel() && bQuad;
	Job->FaceLimit = FMath::Clamp(GetPreferredFaceLimit(Job->ModelId, Job->bQuad), 500, TripoEditorUI::MaxFaceLimit(Job->ModelId, Job->bQuad));
	Job->bTexture = bTexture;
	Job->bPbr = bPbr;
	Job->bAutoSize = IsAdvancedModel() && bAutoSize;
	Job->TextureQuality = SelectedTextureQuality;
	Job->Stage = bTextToModel ? ETripoEditorJobStage::Submitting : ETripoEditorJobStage::Uploading;
	GenerationJobs.Add(Job);
	UpdateTaskQueue();
	return Job;
}

int32 STripoEditorWidget::GetActiveJobCount() const
{
	int32 Count = 0;
	for (const TSharedPtr<FTripoEditorJob>& Job : GenerationJobs) if (Job.IsValid() && !Job->IsTerminal()) ++Count;
	return Count;
}

void STripoEditorWidget::MarkJobFailed(const TSharedPtr<FTripoEditorJob>& Job, const FString& ErrorMessage)
{
	if (!Job.IsValid() || Job->IsTerminal()) return;
	StopJobPolling(Job);
	Job->Stage = ETripoEditorJobStage::Failed;
	Job->ErrorMessage = ErrorMessage;
	UpdateTaskQueue();
	SetStatus(ErrorMessage, true);
	ScheduleBalanceRefresh();
}

void STripoEditorWidget::UpdateTaskQueue()
{
	if (!TaskListBox.IsValid()) return;
	TaskListBox->ClearChildren();
	for (int32 Index = GenerationJobs.Num() - 1; Index >= 0; --Index)
	{
		if (GenerationJobs[Index].IsValid()) TaskListBox->AddSlot().Padding(0, 0, 0, 6)[MakeTaskWidget(GenerationJobs[Index])];
	}
	RefreshTaskSummary();
}

void STripoEditorWidget::RefreshTaskSummary()
{
	const int32 ActiveCount = GetActiveJobCount();
	if (TaskQueueHeader.IsValid()) TaskQueueHeader->SetText(FText::FromString(FString::Printf(TEXT("Task Queue (%d)"), ActiveCount)));
	if (QueueText.IsValid()) QueueText->SetText(FText::FromString(ActiveCount == 0 ? TEXT("Task queue: idle") : FString::Printf(TEXT("Task queue: %d active job(s)"), ActiveCount)));
	}

void STripoEditorWidget::StartTextGeneration(const TSharedPtr<FTripoEditorJob>& Job)
{
	FTextGenerationParams Params;
	Params.ModelVersion = Job->ModelId;
	Params.Prompt = Job->Prompt;
	Params.FaceLimit = Job->FaceLimit;
	Params.bUseFaceLimit = true;
	Params.Texture = Job->bTexture;
	Params.PBR = Job->bPbr;
	Params.AutoSize = Job->bAutoSize;
	Params.Quad = Job->bQuad;
	Params.TextureQuality = Job->TextureQuality;
	const TWeakPtr<STripoEditorWidget> WidgetWeak = StaticCastSharedRef<STripoEditorWidget>(AsShared());
	UTripoBPLibrary::CreateGenerationTaskNative(Job->ApiKey, Params, FOnTripoTaskCreatedNative::CreateLambda([WidgetWeak, Job](bool bSuccess, const FString& TaskID, int32 ErrorCode, const FString& ErrorMessage) { if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak.Pin()) Pinned->HandleGenerationCreated(Job, bSuccess, TaskID, ErrorCode, ErrorMessage); }));
}

void STripoEditorWidget::StartImageUpload(const TSharedPtr<FTripoEditorJob>& Job)
{
	const TWeakPtr<STripoEditorWidget> WidgetWeak = StaticCastSharedRef<STripoEditorWidget>(AsShared());
	UTripoBPLibrary::UploadImageNative(Job->ApiKey, Job->ImagePath, FOnTripoImageUploadedNative::CreateLambda([WidgetWeak, Job](bool bSuccess, const FString& Token, int32 ErrorCode, const FString& ErrorMessage) { if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak.Pin()) Pinned->HandleImageUploaded(Job, bSuccess, Token, ErrorCode, ErrorMessage); }));
}

void STripoEditorWidget::HandleImageUploaded(const TSharedPtr<FTripoEditorJob>& Job, bool bSuccess, const FString& FileToken, int32 ErrorCode, const FString& ErrorMessage)
{
	if (!Job.IsValid() || Job->IsTerminal()) return;
	if (!bSuccess) { MarkJobFailed(Job, TripoEditorUI::FormatRequestError(ErrorCode, ErrorMessage, TEXT("Image upload failed"))); return; }
	SetStatus(TEXT("Submitting image generation..."));
	FImageGenerationParams Params;
	Params.ModelVersion = Job->ModelId;
	Params.Input = FileToken;
	Params.FileToken = FileToken;
	Params.FaceLimit = Job->FaceLimit;
	Params.bUseFaceLimit = true;
	Params.Texture = Job->bTexture;
	Params.PBR = Job->bPbr;
	Params.AutoSize = Job->bAutoSize;
	Params.Quad = Job->bQuad;
	Params.TextureQuality = Job->TextureQuality;
	const TWeakPtr<STripoEditorWidget> WidgetWeak = StaticCastSharedRef<STripoEditorWidget>(AsShared());
	UTripoBPLibrary::CreateModelFromImageNative(Job->ApiKey, Params, FOnTripoImageTaskCreatedNative::CreateLambda([WidgetWeak, Job](bool bCreated, const FString& TaskID, int32 ErrorCode, const FString& ErrorMessage) { if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak.Pin()) Pinned->HandleGenerationCreated(Job, bCreated, TaskID, ErrorCode, ErrorMessage); }));
}

void STripoEditorWidget::HandleGenerationCreated(const TSharedPtr<FTripoEditorJob>& Job, bool bSuccess, const FString& TaskID, int32 ErrorCode, const FString& ErrorMessage)
{
	if (!Job.IsValid() || Job->IsTerminal()) return;
	if (!bSuccess || TaskID.IsEmpty()) { MarkJobFailed(Job, TripoEditorUI::FormatRequestError(ErrorCode, ErrorMessage, TEXT("Generation task creation failed"))); return; }
	Job->GenerationTaskId = TaskID;
	Job->Stage = ETripoEditorJobStage::Generating;
	UpdateTaskQueue();
	SetStatus(FString::Printf(TEXT("Generating task %s..."), *TaskID));
	ScheduleBalanceRefresh();
	StartGenerationPolling(Job, false);
}

void STripoEditorWidget::StartGenerationPolling(const TSharedPtr<FTripoEditorJob>& Job, bool bConversion)
{
	StopJobPolling(Job);
	const TWeakPtr<STripoEditorWidget> WidgetWeak = StaticCastSharedRef<STripoEditorWidget>(AsShared());
	Job->Elapsed = 0.0f;
	Job->SinceLastPoll = 0.0f;
	Job->PollIntervalSeconds = 1.0f;
	Job->ConsecutivePollFailures = 0;
	const FString TaskId = Job->CurrentTaskId();
	Job->PollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WidgetWeak, Job, TaskId, bConversion](float DeltaSeconds) mutable
	{
		if (!Job.IsValid() || Job->IsTerminal()) return false;
		Job->Elapsed += DeltaSeconds;
		Job->SinceLastPoll += DeltaSeconds;
		if (Job->Elapsed >= 300.0f)
		{
			if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak.Pin()) Pinned->MarkJobFailed(Job, TEXT("Task timed out after 300 seconds"));
			return false;
		}
		if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak.Pin())
		{
			// Keep aggregate Editor task lookup traffic near five requests per
			// second while retaining a one-second minimum for small queues.
			const float QueueIntervalSeconds = FMath::Max(1.0f, Pinned->GetActiveJobCount() * 0.2f);
			const float EffectiveIntervalSeconds = FMath::Max(QueueIntervalSeconds, Job->PollIntervalSeconds);
			if (Job->SinceLastPoll < EffectiveIntervalSeconds) return true;
			Job->SinceLastPoll = 0.0f;
			if (Job->bPollInFlight) return true;
			Job->bPollInFlight = true;
			UTripoBPLibrary::GetTaskStatusNative(Job->ApiKey, TaskId, FOnTripoTaskStatusNative::CreateLambda([WidgetWeak, Job, bConversion](bool bRequestSuccess, const FTaskStatusData& StatusData)
			{
				Job->bPollInFlight = false;
				Job->SinceLastPoll = 0.0f;
				if (const TSharedPtr<STripoEditorWidget> PinnedInner = WidgetWeak.Pin()) PinnedInner->HandleTaskStatus(Job, bRequestSuccess, StatusData, bConversion);
			}));
			return true;
		}
		return false;
	}), 0.0f);
}

void STripoEditorWidget::HandleTaskStatus(const TSharedPtr<FTripoEditorJob>& Job, bool bRequestSuccess, const FTaskStatusData& StatusData, bool bConversion)
{
	if (!Job.IsValid() || Job->IsTerminal()) return;
	if (!bRequestSuccess)
	{
		const bool bBackpressure = StatusData.ErrorCode == 429 || StatusData.ErrorCode == 1007 || StatusData.ErrorCode == 2000;
		if (bBackpressure)
		{
			Job->ConsecutivePollFailures = 0;
			Job->PollIntervalSeconds = StatusData.RetryAfterSeconds > 0 ? static_cast<float>(StatusData.RetryAfterSeconds) : 5.0f;
			SetStatus(FString::Printf(TEXT("Task status delayed: %s"), *TripoEditorUI::FormatRequestError(StatusData.ErrorCode, StatusData.ErrorMessage, TEXT("Service requested slower polling"))));
			return;
		}
		++Job->ConsecutivePollFailures;
		Job->PollIntervalSeconds = FMath::Min(4.0f, FMath::Pow(2.0f, static_cast<float>(Job->ConsecutivePollFailures)));
		if (Job->ConsecutivePollFailures >= TripoEditorUI::MaxConsecutivePollFailures)
		{
			MarkJobFailed(Job, TripoEditorUI::FormatRequestError(StatusData.ErrorCode, StatusData.ErrorMessage, TEXT("Task status request failed")));
		}
		return;
	}
	Job->ConsecutivePollFailures = 0;
	Job->PollIntervalSeconds = 1.0f;
	Job->LastServerStatus = StatusData.Status == ETaskStatus::Queued ? TEXT("queued") : StatusData.Status == ETaskStatus::Running ? TEXT("running") : TEXT("terminal");
	if (bConversion) Job->ConversionProgress = StatusData.Progress; else Job->GenerationProgress = StatusData.Progress;
	UpdateTaskQueue();
	if (StatusData.Status == ETaskStatus::Queued || StatusData.Status == ETaskStatus::Running)
	{
		SetStatus(FString::Printf(TEXT("%s %d%%"), bConversion ? TEXT("Converting") : TEXT("Generating"), StatusData.Progress));
		return;
	}
	if (StatusData.Status != ETaskStatus::Success)
	{
		MarkJobFailed(Job, TripoEditorUI::FormatRequestError(StatusData.ErrorCode, StatusData.ErrorMessage, TEXT("Task failed")));
		return;
	}
	if (!bConversion)
	{
		StopJobPolling(Job);
		SetStatus(TEXT("Generation complete; requesting FBX conversion..."));
		const TWeakPtr<STripoEditorWidget> WidgetWeak2 = StaticCastSharedRef<STripoEditorWidget>(AsShared());
		UTripoBPLibrary::ConvertModelNative(Job->ApiKey, Job->GenerationTaskId, FOnTripoTaskCreatedNative::CreateLambda([WidgetWeak2, Job](bool bSuccess, const FString& ConversionTaskID, int32 ErrorCode, const FString& ErrorMessage) { if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak2.Pin()) Pinned->HandleConversionCreated(Job, bSuccess, ConversionTaskID, ErrorCode, ErrorMessage); }));
	}
	else
	{
		StopJobPolling(Job);
		SetStatus(TEXT("Conversion complete; downloading FBX..."));
		// Queue labels reset each editor session, while imported model folders
		// continue from the next available TripoModel_N directory. Reserve the
		// folder only after the service has accepted and completed the task.
		Job->AssetBaseName = UTripoBPEditorLibrary::GetNextTripoModelBaseName();
		Job->Stage = ETripoEditorJobStage::Downloading;
		UpdateTaskQueue();
		const TWeakPtr<STripoEditorWidget> WidgetWeak2 = StaticCastSharedRef<STripoEditorWidget>(AsShared());
		UTripoBPLibrary::DownloadModelNativeWithName(StatusData.ModelURL, Job->AssetBaseName, FOnTripoModelDownloadedNative::CreateLambda([WidgetWeak2, Job](bool bSuccess, const FString& SavedPath) { if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak2.Pin()) Pinned->HandleModelDownloaded(Job, bSuccess, SavedPath); }));
	}
}

void STripoEditorWidget::HandleConversionCreated(const TSharedPtr<FTripoEditorJob>& Job, bool bSuccess, const FString& TaskID, int32 ErrorCode, const FString& ErrorMessage)
{
	if (!Job.IsValid() || Job->IsTerminal()) return;
	if (!bSuccess || TaskID.IsEmpty()) { MarkJobFailed(Job, TripoEditorUI::FormatRequestError(ErrorCode, ErrorMessage, TEXT("FBX conversion task creation failed"))); return; }
	Job->ConversionTaskId = TaskID;
	Job->Stage = ETripoEditorJobStage::Converting;
	UpdateTaskQueue();
	ScheduleBalanceRefresh();
	StartGenerationPolling(Job, true);
}

void STripoEditorWidget::HandleModelDownloaded(const TSharedPtr<FTripoEditorJob>& Job, bool bSuccess, const FString& SavedPath)
{
	if (!Job.IsValid() || Job->IsTerminal()) return;
	if (!bSuccess) { MarkJobFailed(Job, TEXT("Model download failed")); return; }
	Job->OutputAssetPath = SavedPath;
	Job->Stage = ETripoEditorJobStage::Importing;
	LastSavedPath = SavedPath;
	UpdateTaskQueue();
	SetStatus(FString::Printf(TEXT("Download complete; importing into %s..."), *TripoAssetPaths::AssetImportRoot));
	const TWeakPtr<STripoEditorWidget> WidgetWeak2 = StaticCastSharedRef<STripoEditorWidget>(AsShared());
	UTripoBPEditorLibrary::ImportModelToEditorNativeNamed(SavedPath, Job->AssetBaseName, FOnTripoAssetImportedNative::CreateLambda([WidgetWeak2, Job](bool bSuccess, UObject* ImportedAsset) { if (const TSharedPtr<STripoEditorWidget> Pinned = WidgetWeak2.Pin()) Pinned->HandleAssetImported(Job, bSuccess, ImportedAsset); }));
}

void STripoEditorWidget::HandleAssetImported(const TSharedPtr<FTripoEditorJob>& Job, bool bSuccess, UObject* ImportedAsset)
{
	if (!Job.IsValid() || Job->IsTerminal()) return;
	if (!bSuccess || !ImportedAsset)
	{
		MarkJobFailed(Job, TEXT("Asset import failed"));
		return;
	}
	Job->Stage = ETripoEditorJobStage::Completed;
	Job->OutputAssetPath = ImportedAsset->GetPathName();
	StopJobPolling(Job);
	UpdateTaskQueue();
	SetPreviewAsset(ImportedAsset);
	SetStatus(TEXT("SUCCESS - Model imported and ready to preview"));
	ScheduleBalanceRefresh();
}

void STripoEditorWidget::UpdateSelectedImagePreview()
{
	SelectedImageBrush.Reset();
	if (ImagePath.IsEmpty()) return;

	TArray64<uint8> CompressedImage;
	if (!FFileHelper::LoadFileToArray(CompressedImage, *ImagePath))
	{
		SetStatus(TEXT("Unable to load the selected image"), true);
		return;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	FImage Image;
	if (!ImageWrapperModule.DecompressImage(CompressedImage.GetData(), CompressedImage.Num(), Image))
	{
		SetStatus(TEXT("The selected image format is not supported"), true);
		return;
	}

	Image.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	TArray<uint8> RawImageData(MoveTemp(Image.RawData));
	const FName ResourceName(*FString::Printf(TEXT("TripoSelectedImage_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	if (FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(ResourceName, Image.SizeX, Image.SizeY, RawImageData))
	{
		SelectedImageBrush = MakeShared<FSlateDynamicImageBrush>(ResourceName, FVector2D(Image.SizeX, Image.SizeY));
	}
	else
	{
		SetStatus(TEXT("Unable to create image preview"), true);
	}
}

void STripoEditorWidget::SetPreviewAsset(UObject* ImportedAsset)
{
	if (ImportedAsset && ThumbnailPool.IsValid())
	{
		ResultThumbnail = MakeShared<FAssetThumbnail>(ImportedAsset, 256, 256, ThumbnailPool);
		if (PreviewBox.IsValid()) PreviewBox->SetContent(ResultThumbnail->MakeThumbnailWidget());
	}
	if (PreviewText.IsValid()) PreviewText->SetText(FText::FromString(FString::Printf(TEXT("Imported asset: %s\nSaved file: %s\n\nUE FBX import conversion is enabled for Z-up / centimetre space."), *ImportedAsset->GetPathName(), *LastSavedPath)));
}

#undef LOCTEXT_NAMESPACE
