#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TripoBPLibrary.generated.h"

/** API v3 task states. Unknown states are terminal failures. */
UENUM(BlueprintType)
enum class ETaskStatus : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	Queued UMETA(DisplayName = "Queued"),
	Running UMETA(DisplayName = "Running"),
	Success UMETA(DisplayName = "Success"),
	Failed UMETA(DisplayName = "Failed"),
	Cancelled UMETA(DisplayName = "Cancelled"),
	Banned UMETA(DisplayName = "Banned"),
	Expired UMETA(DisplayName = "Expired")
};

/** Stable API model identifiers. Do not persist the enum ordinal in user settings. */
UENUM(BlueprintType)
enum class EModelVersion : uint8
{
	// Serialized values from the v2 plugin are retained so existing Blueprint
	// assets can load; requests map them to the corresponding v3 model or the
	// documented default rather than using ordinal semantics.
	Legacy_v3_0_20250812 = 0 UMETA(Hidden),
	Legacy_v2_5_20250123 = 1 UMETA(Hidden),
	Legacy_v2_0_20240919 = 2 UMETA(Hidden),
	Legacy_v1_4_20240625 = 3 UMETA(Hidden),
	Legacy_Turbo_v1_0_20250506 = 4 UMETA(Hidden),

	v3_1_20260211 = 10 UMETA(DisplayName = "v3.1-20260211"),
	v3_0_20250812 = 11 UMETA(DisplayName = "v3.0-20250812"),
	v2_5_20250123 = 12 UMETA(DisplayName = "v2.5-20250123"),
	P1_20260311 = 13 UMETA(DisplayName = "P1-20260311")
};

/** v2-only style values are retained for asset loading but are not sent to v3. */
UENUM(BlueprintType)
enum class EStyle : uint8
{
	None UMETA(DisplayName = "none"),
	Person_person2cartoon UMETA(Hidden),
	Object_clay UMETA(Hidden),
	Object_steampunk UMETA(Hidden),
	Animal_venom UMETA(Hidden),
	Object_barbie UMETA(Hidden),
	Object_christmas UMETA(Hidden),
	Gold UMETA(Hidden),
	Ancient_bronze UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ETextureQuality : uint8
{
	Standard UMETA(DisplayName = "standard"),
	Detailed UMETA(DisplayName = "detailed")
};

USTRUCT(BlueprintType)
struct TRIPORUNTIME_API FTaskStatusData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	ETaskStatus Status = ETaskStatus::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	int32 Progress = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	FString TaskID;

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	FString ModelURL;

	// Kept for existing Blueprint callers. API v3 returns one model URL, which
	// is mirrored into these legacy output pins on completion.
	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	FString BaseModelURL;

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	FString PBRModelURL;

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	FString RenderedImageURL;

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	FDateTime CreateTime;

	/** API v3 created_at value, retained as ISO 8601 text. */
	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	FString CreatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	int32 ErrorCode = 0;

	/** Server-requested delay for a throttled task lookup, in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	int32 RetryAfterSeconds = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tripo|Status")
	FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct TRIPORUNTIME_API FTextGenerationParams
{
	GENERATED_BODY()

	// These strings intentionally retain the original Blueprint pin types.
	// Native Slate code maps them to the validated v3 model identifiers.
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString ModelVersion = TEXT("v3.1-20260211");

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString Prompt;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString NegativePrompt;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool bUseNegativePrompt = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	int32 FaceLimit = 2000000;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool bUseFaceLimit = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	int32 ModelSeed = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "image_seed is not part of the v3 contract"))
	int32 ImageSeed = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool Texture = true;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool PBR = true;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	int32 TextureSeed = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "texture_alignment is not part of the verified v3 contract"))
	FString TextureAlignment = TEXT("auto");

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString TextureQuality = TEXT("standard");

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool AutoSize = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool Quad = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "orientation is not part of the verified v3 contract"))
	FString Orientation;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "style is not part of the verified v3 contract"))
	FString Style;
};

USTRUCT(BlueprintType)
struct TRIPORUNTIME_API FImageGenerationParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString ModelVersion = TEXT("v3.1-20260211");

	/** API v3 input. Use the file_token returned by UploadImage or a remote URL. */
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString Input;

	/** Kept as a source-compatible alias for existing Blueprint graphs. */
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString FileToken;

	/** Kept as a source-compatible alias for existing Blueprint graphs. */
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString URL;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "file_type is not part of the v3 request body"))
	FString FileType = TEXT("jpg");

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	int32 ModelSeed = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	int32 FaceLimit = 2000000;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool bUseFaceLimit = true;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool Texture = true;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool PBR = true;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	int32 TextureSeed = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString TextureAlignment = TEXT("auto");

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString TextureQuality = TEXT("standard");

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool AutoSize = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	bool Quad = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "style is not part of the verified v3 contract"))
	FString Style;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "orientation is not part of the verified v3 contract"))
	FString Orientation = TEXT("auto");
};

/** Legacy multiview input is retained only so old Blueprint assets can load. v3 has no supported endpoint here. */
USTRUCT(BlueprintType)
struct TRIPORUNTIME_API FMultiviewFileParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString Type = TEXT("jpg");

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString FileToken;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FString URL;
};

USTRUCT(BlueprintType)
struct TRIPORUNTIME_API FMultiviewGenerationParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	FString ModelVersion = TEXT("v3.1-20260211");

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FMultiviewFileParams FrontFile;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FMultiviewFileParams LeftFile;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FMultiviewFileParams BackFile;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation")
	FMultiviewFileParams RightFile;

	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	int32 FaceLimit = 2000000;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	bool Texture = true;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	bool PBR = true;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	int32 TextureSeed = -1;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	FString TextureAlignment;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	FString TextureQuality = TEXT("standard");
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	bool AutoSize = false;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	FString Orientation;
	UPROPERTY(BlueprintReadWrite, Category = "Tripo|Generation", meta = (DeprecatedProperty, DeprecationMessage = "multiview is not part of the verified v3 contract"))
	bool Quad = false;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnTripoRequestComplete, bool, bSuccess, int32, Balance);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnTripoBalanceReceived, bool, bSuccess, int32, Balance, int32, Frozen);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnTaskCreated, bool, bSuccess, const FString&, TaskID);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnImageTaskCreated, bool, bSuccess, const FString&, TaskID);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnTaskStatusReceived, bool, bSuccess, const FTaskStatusData&, StatusData);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnModelDownloaded, bool, bSuccess, const FString&, SavedPath);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnAssetImported, bool, bSuccess, UObject*, ImportedAsset);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnImageUploaded, bool, bSuccess, const FString&, ImageToken);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnMultiviewTaskCreated, bool, bSuccess, const FString&, TaskID);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnFileSelected, bool, bSuccess, const FString&, FilePath);

DECLARE_DELEGATE_FourParams(FOnTripoTaskCreatedNative, bool, const FString&, int32, const FString&);
DECLARE_DELEGATE_FourParams(FOnTripoImageTaskCreatedNative, bool, const FString&, int32, const FString&);
DECLARE_DELEGATE_TwoParams(FOnTripoTaskStatusNative, bool, const FTaskStatusData&);
DECLARE_DELEGATE_TwoParams(FOnTripoModelDownloadedNative, bool, const FString&);
DECLARE_DELEGATE_FourParams(FOnTripoImageUploadedNative, bool, const FString&, int32, const FString&);
DECLARE_DELEGATE_TwoParams(FOnTripoAssetImportedNative, bool, UObject*);

UCLASS(meta = (ScriptName = "TripoFunctions"))
class TRIPORUNTIME_API UTripoBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UTripoBPLibrary(const FObjectInitializer& ObjectInitializer);

	/** Cancels local HTTP requests during module teardown; v3 has no public task-cancel endpoint. */
	static void CancelAllRequests();

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Get User Balance"))
	static void GetUserBalance(const FString& ApiKey, const FOnTripoRequestComplete& Callback);

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Get User Balance Details"))
	static void GetUserBalanceDetails(const FString& ApiKey, const FOnTripoBalanceReceived& Callback);
	static void GetUserBalanceDetailsNative(const FString& ApiKey, TFunction<void(bool, int32, int32)> Callback);

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Create Text-to-Model Task"))
	static void CreateGenerationTask(const FString& ApiKey, const FTextGenerationParams& Params, const FOnTaskCreated& Callback);
	static void CreateGenerationTaskNative(const FString& ApiKey, const FTextGenerationParams& Params, FOnTripoTaskCreatedNative Callback);

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Create Image-to-Model Task"))
	static void CreateModelFromImage(const FString& ApiKey, const FImageGenerationParams& Params, const FOnImageTaskCreated& Callback);
	static void CreateModelFromImageNative(const FString& ApiKey, const FImageGenerationParams& Params, FOnTripoImageTaskCreatedNative Callback);

	/** v3 does not expose the old multiview endpoint; this returns failure without making a request. */
	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Create Multiview-to-Model Task"))
	static void CreateModelFromMultiview(const FString& ApiKey, const FMultiviewGenerationParams& Params, const FOnMultiviewTaskCreated& Callback);

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Get Task Status"))
	static void GetTaskStatus(const FString& ApiKey, const FString& TaskID, const FOnTaskStatusReceived& Callback);
	static void GetTaskStatusNative(const FString& ApiKey, const FString& TaskID, FOnTripoTaskStatusNative Callback);

	/** Polls one existing task at one-second intervals for at most five minutes. */
	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Poll Task Until Complete"))
	static void PollTaskUntilComplete(const FString& ApiKey, const FString& TaskID, const FOnTaskStatusReceived& Callback);

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Convert Model to FBX"))
	static void ConvertModel(const FString& ApiKey, const FString& GenerationTaskID, const FOnTaskCreated& Callback);
	static void ConvertModelNative(const FString& ApiKey, const FString& GenerationTaskID, FOnTripoTaskCreatedNative Callback);

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Upload Image"))
	static void UploadImage(const FString& ApiKey, const FString& ImagePath, const FOnImageUploaded& Callback);
	static void UploadImageNative(const FString& ApiKey, const FString& ImagePath, FOnTripoImageUploadedNative Callback);

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Download Model"))
	static void DownloadModel(const FString& ModelURL, const FOnModelDownloaded& Callback);
	static void DownloadModelNative(const FString& ModelURL, FOnTripoModelDownloadedNative Callback);
	/** Downloads a model into a caller-owned, collision-free file name. */
	static void DownloadModelNativeWithName(const FString& ModelURL, const FString& DesiredBaseName, FOnTripoModelDownloadedNative Callback);

	UFUNCTION(BlueprintCallable, Category = "Tripo|Runtime", meta = (DisplayName = "Load API Key From File"))
	static bool LoadAPIKeyFromFile(FString& OutApiKey);
};
