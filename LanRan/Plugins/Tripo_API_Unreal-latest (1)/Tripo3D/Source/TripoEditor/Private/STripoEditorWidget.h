#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Views/SListView.h"
#include "TripoBPLibrary.h"

class FAssetThumbnail;
class FAssetThumbnailPool;
class STextBlock;
struct FSlateDynamicImageBrush;

/** Slate implementation of the Unity Tripo editor workflow. */
class STripoEditorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STripoEditorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STripoEditorWidget() override;

private:
	using FStringItem = TSharedPtr<FString>;

	TSharedRef<SWidget> BuildApiHeader();
	TSharedRef<SWidget> BuildGenerationPanel();
	TSharedRef<SWidget> BuildTextPanel();
	TSharedRef<SWidget> BuildImagePanel();
	TSharedRef<SWidget> BuildSettingsPanel();
	TSharedRef<SWidget> BuildTaskQueue();
	TSharedRef<SWidget> BuildResultPanel();
	TSharedRef<SWidget> MakeComboItem(FStringItem Item) const;

	FReply OnTextTabClicked();
	FReply OnImageTabClicked();
	FReply OnConfirmApiKeyClicked();
	FReply OnChooseImageClicked();
	FReply OnGenerateClicked();
	FReply OnClearFinishedClicked();

	void SetActiveTab(int32 TabIndex);
	void SetStatus(const FString& Message, bool bError = false);
	void StopPolling();
	void StopJobPolling(const TSharedPtr<struct FTripoEditorJob>& Job);
	void ScheduleBalanceRefresh(float InitialDelaySeconds = 1.0f);
	void StopBalanceRefresh();
	void SavePreferences() const;
	void RefreshBalance();
	void StartTextGeneration(const TSharedPtr<struct FTripoEditorJob>& Job);
	void StartImageUpload(const TSharedPtr<struct FTripoEditorJob>& Job);
	void StartGenerationPolling(const TSharedPtr<struct FTripoEditorJob>& Job, bool bConversion);
	void HandleGenerationCreated(const TSharedPtr<struct FTripoEditorJob>& Job, bool bSuccess, const FString& TaskID, int32 ErrorCode, const FString& ErrorMessage);
	void HandleImageUploaded(const TSharedPtr<struct FTripoEditorJob>& Job, bool bSuccess, const FString& FileToken, int32 ErrorCode, const FString& ErrorMessage);
	void HandleTaskStatus(const TSharedPtr<struct FTripoEditorJob>& Job, bool bRequestSuccess, const FTaskStatusData& StatusData, bool bConversion);
	void HandleConversionCreated(const TSharedPtr<struct FTripoEditorJob>& Job, bool bSuccess, const FString& TaskID, int32 ErrorCode, const FString& ErrorMessage);
	void HandleModelDownloaded(const TSharedPtr<struct FTripoEditorJob>& Job, bool bSuccess, const FString& SavedPath);
	void HandleAssetImported(const TSharedPtr<struct FTripoEditorJob>& Job, bool bSuccess, UObject* ImportedAsset);
	void UpdateSelectedImagePreview();
	void SetPreviewAsset(UObject* ImportedAsset);
	void UpdateTaskQueue();
	void RefreshTaskSummary();
	TSharedRef<SWidget> MakeTaskWidget(const TSharedPtr<struct FTripoEditorJob>& Job) const;
	TSharedPtr<struct FTripoEditorJob> CreateGenerationJob(bool bTextToModel);
	int32 GetActiveJobCount() const;
	void MarkJobFailed(const TSharedPtr<struct FTripoEditorJob>& Job, const FString& ErrorMessage);

	FString GetSelectedModelId() const;
	bool IsAdvancedModel() const;
	bool IsQuadModel() const;
	int32 GetFaceLimit() const;
	int32 GetPreferredFaceLimit(const FString& ModelId, bool bForQuad) const;
	void SetPreferredFaceLimit(const FString& ModelId, bool bForQuad, int32 Value);

	TSharedPtr<SEditableTextBox> ApiKeyTextBox;
	TSharedPtr<SMultiLineEditableTextBox> PromptTextBox;
	TSharedPtr<SEditableTextBox> ImagePathTextBox;
	TSharedPtr<SComboBox<FStringItem>> ModelComboBox;
	TSharedPtr<SComboBox<FStringItem>> TextureQualityComboBox;
	TSharedPtr<SSpinBox<int32>> FaceLimitSpinBox;
	TSharedPtr<SWidgetSwitcher> GenerationSwitcher;
	TSharedPtr<STextBlock> BalanceText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<STextBlock> PreviewText;
	TSharedPtr<STextBlock> QueueText;
	TSharedPtr<STextBlock> TaskQueueHeader;
	TSharedPtr<SVerticalBox> TaskListBox;
	TSharedPtr<SBox> SelectedImagePreviewBox;
	TSharedPtr<SBox> PreviewBox;
	TArray<FStringItem> ModelOptions;
	TArray<FStringItem> TextureQualityOptions;
	FString ApiKey;
	FString TextPrompt;
	FString ImagePath;
	FString SelectedModelId;
	FString SelectedTextureQuality;
	FString CurrentTaskID;
	FString LastSavedPath;
	int32 ActiveTab = 0;
	int32 Progress = 0;
	int32 NextJobNumber = 1;
	int32 V3TriangleFaceLimit = 2000000;
	int32 V3QuadFaceLimit = 150000;
	int32 V25FaceLimit = 500000;
	int32 V25QuadFaceLimit = 150000;
	int32 P1FaceLimit = 20000;
	bool bApiKeyConfirmed = false;
	bool bTexture = true;
	bool bPbr = true;
	bool bAutoSize = false;
	bool bQuad = false;
	bool bAdvancedExpanded = true;
	bool bBalanceRefreshRequestInFlight = false;
	bool bBalanceRefreshPending = false;
	float BalanceRefreshElapsed = 0.0f;
	float BalanceRefreshDelay = 1.0f;
	FTSTicker::FDelegateHandle BalanceRefreshTickerHandle;
	TArray<TSharedPtr<struct FTripoEditorJob>> GenerationJobs;
	TSharedPtr<FSlateDynamicImageBrush> SelectedImageBrush;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<FAssetThumbnail> ResultThumbnail;
};
