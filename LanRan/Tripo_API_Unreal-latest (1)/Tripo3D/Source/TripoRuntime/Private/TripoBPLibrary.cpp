#include "TripoBPLibrary.h"

#include "TripoAssetPaths.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformFileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace TripoV3
{
	static FCriticalSection RequestMutex;
	static TArray<TSharedRef<IHttpRequest, ESPMode::ThreadSafe>> ActiveRequests;

	static void TrackRequest(const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& Request)
	{
		FScopeLock Lock(&RequestMutex);
		ActiveRequests.Add(Request);
	}

	static void UntrackRequest(const FHttpRequestPtr& Request)
	{
		FScopeLock Lock(&RequestMutex);
		ActiveRequests.RemoveAll([&Request](const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& Item) { return &Item.Get() == Request.Get(); });
	}
	static const FString BaseUrl = TEXT("https://openapi.tripo3d.ai/v3");
	static const FString TextToModelUrl = BaseUrl + TEXT("/generation/text-to-model");
	static const FString ImageToModelUrl = BaseUrl + TEXT("/generation/image-to-model");
	static const FString UploadUrl = BaseUrl + TEXT("/files");
	static const FString BalanceUrl = BaseUrl + TEXT("/account/balance");
	static const FString ConvertUrl = BaseUrl + TEXT("/models/convert");

	static FString NormalizeModelId(const FString& RequestedModel)
	{
		if (RequestedModel == TEXT("v3.0-20250812")) return TEXT("v3.0-20250812");
		if (RequestedModel == TEXT("v2.5-20250123")) return TEXT("v2.5-20250123");
		if (RequestedModel == TEXT("P1-20260311")) return TEXT("P1-20260311");
		return TEXT("v3.1-20260211");
	}

	static ETextureQuality NormalizeTextureQuality(const FString& RequestedQuality)
	{
		return RequestedQuality.Equals(TEXT("detailed"), ESearchCase::IgnoreCase)
			? ETextureQuality::Detailed
			: ETextureQuality::Standard;
	}

	static bool SupportsAdvanced(const FString& Model)
	{
		return Model == TEXT("v3.1-20260211") || Model == TEXT("v3.0-20250812");
	}

	static bool SupportsQuad(const FString& Model)
	{
		return SupportsAdvanced(Model) || Model == TEXT("v2.5-20250123");
	}

	static int32 MaxFaceLimit(const FString& Model, bool bQuad)
	{
		if (Model == TEXT("P1-20260311")) return 20000;
		if (bQuad && SupportsQuad(Model)) return 150000;
		if (Model == TEXT("v2.5-20250123")) return 500000;
		return 2000000;
	}

	static int32 ClampFaceLimit(const FString& Model, bool bQuad, int32 Value)
	{
		return FMath::Clamp(Value, 500, MaxFaceLimit(Model, bQuad));
	}

	static bool IsHttpSuccess(const FHttpResponsePtr& Response, bool bConnected)
	{
		return bConnected && Response.IsValid() && Response->GetResponseCode() >= 200 && Response->GetResponseCode() < 300;
	}

	static TSharedPtr<FJsonObject> ParseJson(const FHttpResponsePtr& Response)
	{
		if (!Response.IsValid()) return nullptr;
		TSharedPtr<FJsonObject> Json;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		return FJsonSerializer::Deserialize(Reader, Json) ? Json : nullptr;
	}

	static int32 GetCode(const TSharedPtr<FJsonObject>& Json)
	{
		if (!Json.IsValid()) return -1;
		double Code = 0.0;
		return Json->TryGetNumberField(TEXT("code"), Code) ? FMath::RoundToInt(Code) : 0;
	}

	static bool TryGetInt(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field, int32& OutValue)
	{
		double Number = 0.0;
		if (!Json.IsValid() || !Json->TryGetNumberField(Field, Number)) return false;
		OutValue = FMath::RoundToInt(Number);
		return true;
	}

	static FString GetErrorMessage(const TSharedPtr<FJsonObject>& Json)
	{
		if (!Json.IsValid()) return TEXT("Invalid JSON response");
		FString Message;
		if (!Json->TryGetStringField(TEXT("message"), Message)) Json->TryGetStringField(TEXT("error"), Message);
		FString Suggestion;
		if (Json->TryGetStringField(TEXT("suggestion"), Suggestion) && !Suggestion.IsEmpty() && Suggestion != Message)
		{
			if (!Message.IsEmpty()) Message += TEXT(" ");
			Message += Suggestion;
		}
		return Message.IsEmpty() ? TEXT("Tripo API request failed") : Message;
	}

	static void GetRequestError(const FHttpResponsePtr& Response, bool bConnected, const TSharedPtr<FJsonObject>& Json, int32& OutErrorCode, FString& OutErrorMessage, int32* OutRetryAfterSeconds = nullptr)
	{
		if (OutRetryAfterSeconds) *OutRetryAfterSeconds = 0;
		OutErrorCode = GetCode(Json);
		if (OutErrorCode <= 0 && Response.IsValid() && !IsHttpSuccess(Response, bConnected))
		{
			OutErrorCode = Response->GetResponseCode();
		}
		if (OutErrorCode == 0 && !bConnected) OutErrorCode = -1;

		if (Json.IsValid())
		{
			OutErrorMessage = GetErrorMessage(Json);
		}
		else if (!bConnected)
		{
			OutErrorMessage = TEXT("Network request failed");
		}
		else if (Response.IsValid())
		{
			OutErrorMessage = FString::Printf(TEXT("HTTP request failed with status %d"), Response->GetResponseCode());
		}
		else
		{
			OutErrorMessage = TEXT("No HTTP response received");
		}

		if (Response.IsValid() && Response->GetResponseCode() == 429)
		{
			const FString RetryAfter = Response->GetHeader(TEXT("Retry-After")).TrimStartAndEnd();
			if (!RetryAfter.IsEmpty())
			{
				if (OutRetryAfterSeconds) *OutRetryAfterSeconds = FMath::Max(0, FCString::Atoi(*RetryAfter));
				if (!OutErrorMessage.IsEmpty()) OutErrorMessage += TEXT(" ");
				OutErrorMessage += FString::Printf(TEXT("Retry after %s seconds."), *RetryAfter);
			}
		}
	}

	static FString GetString(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field)
	{
		FString Value;
		if (Json.IsValid()) Json->TryGetStringField(Field, Value);
		return Value;
	}

	static TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CreateRequest(const FString& URL, const FString& Verb, const FString& ApiKey, float TimeoutSeconds = 60.0f)
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(URL);
		Request->SetVerb(Verb);
		Request->SetTimeout(TimeoutSeconds);
		if (!ApiKey.IsEmpty())
		{
			Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
		}
		TrackRequest(Request);
		return Request;
	}

	static void AddCommonGenerationFields(const TSharedPtr<FJsonObject>& Body, const FString& Model, int32 FaceLimit, bool bUseFaceLimit, bool bTexture, bool bPbr, int32 TextureSeed, ETextureQuality TextureQuality, bool bAutoSize, bool bQuad)
	{
		if (bUseFaceLimit && FaceLimit > 0)
		{
			Body->SetNumberField(TEXT("face_limit"), ClampFaceLimit(Model, bQuad, FaceLimit));
		}
		Body->SetBoolField(TEXT("texture"), bTexture);
		Body->SetBoolField(TEXT("pbr"), bPbr);
		// texture_quality and auto_size are documented only for v3.0+.
		if (SupportsAdvanced(Model))
		{
			Body->SetStringField(TEXT("texture_quality"), TextureQuality == ETextureQuality::Detailed ? TEXT("detailed") : TEXT("standard"));
			Body->SetBoolField(TEXT("auto_size"), bAutoSize);
		}
		if (SupportsQuad(Model))
		{
			Body->SetBoolField(TEXT("quad"), bQuad);
		}
		// Texture seeds are not part of the current v3 endpoint contract.
		(void)TextureSeed;
	}

	static bool TryGetDataObject(const TSharedPtr<FJsonObject>& Json, TSharedPtr<FJsonObject>& OutData)
	{
		if (!Json.IsValid() || GetCode(Json) != 0) return false;
		const TSharedPtr<FJsonObject>* Field = nullptr;
		if (!Json->TryGetObjectField(TEXT("data"), Field) || !Field) return false;
		OutData = *Field;
		return OutData.IsValid();
	}

	static ETaskStatus ParseStatus(const FString& Status)
	{
		if (Status == TEXT("queued")) return ETaskStatus::Queued;
		if (Status == TEXT("running")) return ETaskStatus::Running;
		if (Status == TEXT("success")) return ETaskStatus::Success;
		if (Status == TEXT("banned")) return ETaskStatus::Banned;
		if (Status == TEXT("expired")) return ETaskStatus::Expired;
		if (Status == TEXT("cancelled")) return ETaskStatus::Cancelled;
		// The API contract requires unknown states to fail closed.
		return ETaskStatus::Failed;
	}

	static FTaskStatusData ParseTask(const TSharedPtr<FJsonObject>& Json)
	{
		FTaskStatusData Result;
		TSharedPtr<FJsonObject> Data;
		if (!TryGetDataObject(Json, Data))
		{
			Result.Status = ETaskStatus::Failed;
			Result.ErrorMessage = GetErrorMessage(Json);
			return Result;
		}

		Result.TaskID = GetString(Data, TEXT("task_id"));
		const FString StatusString = GetString(Data, TEXT("status"));
		Result.Status = ParseStatus(StatusString);
		Result.CreatedAt = GetString(Data, TEXT("created_at"));
		TryGetInt(Data, TEXT("progress"), Result.Progress);
		Result.Progress = FMath::Clamp(Result.Progress, 0, 100);
		TryGetInt(Data, TEXT("error_code"), Result.ErrorCode);
		Result.ErrorMessage = GetString(Data, TEXT("message"));
		if (Result.ErrorMessage.IsEmpty()) Result.ErrorMessage = GetString(Data, TEXT("error"));

		TSharedPtr<FJsonObject> Output;
		const TSharedPtr<FJsonObject>* OutputField = nullptr;
		if (Data->TryGetObjectField(TEXT("output"), OutputField) && OutputField)
		{
			Output = *OutputField;
			Result.ModelURL = GetString(Output, TEXT("model_url"));
			if (Result.ModelURL.IsEmpty()) Result.ModelURL = GetString(Output, TEXT("model"));
			Result.BaseModelURL = GetString(Output, TEXT("base_model"));
			Result.PBRModelURL = GetString(Output, TEXT("pbr_model"));
			// v3 returns one model URL. Existing Blueprint output pins receive it too.
			if (Result.BaseModelURL.IsEmpty()) Result.BaseModelURL = Result.ModelURL;
			if (Result.PBRModelURL.IsEmpty()) Result.PBRModelURL = Result.ModelURL;
			Result.RenderedImageURL = GetString(Output, TEXT("rendered_image_url"));
			if (Result.RenderedImageURL.IsEmpty()) Result.RenderedImageURL = GetString(Output, TEXT("rendered_image"));
		}
		if (Result.Status == ETaskStatus::Success && Result.ModelURL.IsEmpty())
		{
			Result.Status = ETaskStatus::Failed;
			Result.ErrorMessage = TEXT("Successful task returned no model_url");
		}
		return Result;
	}
}

UTripoBPLibrary::UTripoBPLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTripoBPLibrary::CancelAllRequests()
{
	TArray<TSharedRef<IHttpRequest, ESPMode::ThreadSafe>> Requests;
	{
		FScopeLock Lock(&TripoV3::RequestMutex);
		Requests = MoveTemp(TripoV3::ActiveRequests);
	}
	for (const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& Request : Requests)
	{
		if (Request->GetStatus() == EHttpRequestStatus::Processing)
		{
			Request->CancelRequest();
		}
	}
}

void UTripoBPLibrary::GetUserBalance(const FString& ApiKey, const FOnTripoRequestComplete& Callback)
{
	if (ApiKey.IsEmpty())
	{
		Callback.ExecuteIfBound(false, 0);
		return;
	}
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::BalanceUrl, TEXT("GET"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindLambda(
		[Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			TripoV3::UntrackRequest(Request);
			int32 Balance = 0;
			const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
			TSharedPtr<FJsonObject> Data;
			bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::GetCode(Json) == 0;
			if (bSuccess)
			{
				const TSharedPtr<FJsonObject>* DataField = nullptr;
				if (Json->TryGetObjectField(TEXT("data"), DataField) && DataField) Data = *DataField;
				else Data = Json;
				bSuccess = Data.IsValid() && Data->TryGetNumberField(TEXT("balance"), Balance);
			}
			Callback.ExecuteIfBound(bSuccess, Balance);
		});
	Request->ProcessRequest();
}

void UTripoBPLibrary::GetUserBalanceDetails(const FString& ApiKey, const FOnTripoBalanceReceived& Callback)
{
	if (ApiKey.IsEmpty())
	{
		Callback.ExecuteIfBound(false, 0, 0);
		return;
	}
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::BalanceUrl, TEXT("GET"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindLambda(
		[Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			TripoV3::UntrackRequest(Request);
			int32 Balance = 0;
			int32 Frozen = 0;
			const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
			TSharedPtr<FJsonObject> Data;
			bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::GetCode(Json) == 0;
			if (bSuccess)
			{
				const TSharedPtr<FJsonObject>* DataField = nullptr;
				if (Json->TryGetObjectField(TEXT("data"), DataField) && DataField) Data = *DataField;
				else Data = Json;
				bSuccess = Data.IsValid() && TripoV3::TryGetInt(Data, TEXT("balance"), Balance);
				if (Data.IsValid()) TripoV3::TryGetInt(Data, TEXT("frozen"), Frozen);
			}
			Callback.ExecuteIfBound(bSuccess, Balance, Frozen);
		});
	Request->ProcessRequest();
}

void UTripoBPLibrary::GetUserBalanceDetailsNative(const FString& ApiKey, TFunction<void(bool, int32, int32)> Callback)
{
	if (ApiKey.IsEmpty()) { Callback(false, 0, 0); return; }
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::BalanceUrl, TEXT("GET"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindLambda([Callback = MoveTemp(Callback)](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
	{
		TripoV3::UntrackRequest(Request);
		int32 Balance = 0, Frozen = 0;
		const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
		TSharedPtr<FJsonObject> Data;
		bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::GetCode(Json) == 0;
		if (bSuccess)
		{
			const TSharedPtr<FJsonObject>* DataField = nullptr;
			if (Json->TryGetObjectField(TEXT("data"), DataField) && DataField) Data = *DataField; else Data = Json;
			bSuccess = Data.IsValid() && TripoV3::TryGetInt(Data, TEXT("balance"), Balance);
			if (Data.IsValid()) TripoV3::TryGetInt(Data, TEXT("frozen"), Frozen);
		}
		Callback(bSuccess, Balance, Frozen);
	});
	Request->ProcessRequest();
}

void UTripoBPLibrary::CreateGenerationTask(const FString& ApiKey, const FTextGenerationParams& Params, const FOnTaskCreated& Callback)
{
	if (ApiKey.IsEmpty() || Params.Prompt.IsEmpty())
	{
		Callback.ExecuteIfBound(false, TEXT(""));
		return;
	}
	const FString Model = TripoV3::NormalizeModelId(Params.ModelVersion);
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("model"), Model);
	Body->SetStringField(TEXT("prompt"), Params.Prompt);
	if (Params.bUseNegativePrompt && !Params.NegativePrompt.IsEmpty()) Body->SetStringField(TEXT("negative_prompt"), Params.NegativePrompt);
	TripoV3::AddCommonGenerationFields(Body, Model, Params.FaceLimit, Params.bUseFaceLimit, Params.Texture, Params.PBR, Params.TextureSeed, TripoV3::NormalizeTextureQuality(Params.TextureQuality), Params.AutoSize, Params.Quad);
	FString Content;
	FJsonSerializer::Serialize(Body.ToSharedRef(), TJsonWriterFactory<>::Create(&Content));
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::TextToModelUrl, TEXT("POST"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetContentAsString(Content);
	Request->OnProcessRequestComplete().BindLambda(
		[Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			TripoV3::UntrackRequest(Request);
			FString TaskID;
			TSharedPtr<FJsonObject> Data;
			const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
			const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::TryGetDataObject(Json, Data) && Data->TryGetStringField(TEXT("task_id"), TaskID) && !TaskID.IsEmpty();
			Callback.ExecuteIfBound(bSuccess, TaskID);
		});
	Request->ProcessRequest();
}

void UTripoBPLibrary::CreateGenerationTaskNative(const FString& ApiKey, const FTextGenerationParams& Params, FOnTripoTaskCreatedNative Callback)
{
	if (ApiKey.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("API key is required")); return; }
	if (Params.Prompt.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("Prompt is required")); return; }
	const FString Model = TripoV3::NormalizeModelId(Params.ModelVersion);
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("model"), Model);
	Body->SetStringField(TEXT("prompt"), Params.Prompt);
	if (Params.bUseNegativePrompt && !Params.NegativePrompt.IsEmpty()) Body->SetStringField(TEXT("negative_prompt"), Params.NegativePrompt);
	TripoV3::AddCommonGenerationFields(Body, Model, Params.FaceLimit, Params.bUseFaceLimit, Params.Texture, Params.PBR, Params.TextureSeed, TripoV3::NormalizeTextureQuality(Params.TextureQuality), Params.AutoSize, Params.Quad);
	FString Content; FJsonSerializer::Serialize(Body.ToSharedRef(), TJsonWriterFactory<>::Create(&Content));
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::TextToModelUrl, TEXT("POST"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json")); Request->SetContentAsString(Content);
	Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
	{
		TripoV3::UntrackRequest(Request); FString TaskID; TSharedPtr<FJsonObject> Data; const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
		const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::TryGetDataObject(Json, Data) && Data->TryGetStringField(TEXT("task_id"), TaskID) && !TaskID.IsEmpty();
		int32 ErrorCode = 0; FString ErrorMessage; if (!bSuccess) TripoV3::GetRequestError(Response, bConnected, Json, ErrorCode, ErrorMessage);
		Callback.ExecuteIfBound(bSuccess, TaskID, ErrorCode, ErrorMessage);
	});
	Request->ProcessRequest();
}

void UTripoBPLibrary::CreateModelFromImage(const FString& ApiKey, const FImageGenerationParams& Params, const FOnImageTaskCreated& Callback)
{
	if (ApiKey.IsEmpty())
	{
		Callback.ExecuteIfBound(false, TEXT(""));
		return;
	}
	const FString Input = !Params.Input.IsEmpty() ? Params.Input : (!Params.FileToken.IsEmpty() ? Params.FileToken : Params.URL);
	if (Input.IsEmpty())
	{
		Callback.ExecuteIfBound(false, TEXT(""));
		return;
	}
	const FString Model = TripoV3::NormalizeModelId(Params.ModelVersion);
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("model"), Model);
	Body->SetStringField(TEXT("input"), Input);
	TripoV3::AddCommonGenerationFields(Body, Model, Params.FaceLimit, Params.bUseFaceLimit, Params.Texture, Params.PBR, Params.TextureSeed, TripoV3::NormalizeTextureQuality(Params.TextureQuality), Params.AutoSize, Params.Quad);
	FString Content;
	FJsonSerializer::Serialize(Body.ToSharedRef(), TJsonWriterFactory<>::Create(&Content));
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::ImageToModelUrl, TEXT("POST"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Content);
	Request->OnProcessRequestComplete().BindLambda(
		[Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			TripoV3::UntrackRequest(Request);
			FString TaskID;
			TSharedPtr<FJsonObject> Data;
			const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
			const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::TryGetDataObject(Json, Data) && Data->TryGetStringField(TEXT("task_id"), TaskID) && !TaskID.IsEmpty();
			Callback.ExecuteIfBound(bSuccess, TaskID);
		});
	Request->ProcessRequest();
}

void UTripoBPLibrary::CreateModelFromImageNative(const FString& ApiKey, const FImageGenerationParams& Params, FOnTripoImageTaskCreatedNative Callback)
{
	if (ApiKey.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("API key is required")); return; }
	const FString Input = !Params.Input.IsEmpty() ? Params.Input : (!Params.FileToken.IsEmpty() ? Params.FileToken : Params.URL);
	if (Input.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("Image input is required")); return; }
	const FString Model = TripoV3::NormalizeModelId(Params.ModelVersion); TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("model"), Model); Body->SetStringField(TEXT("input"), Input);
	TripoV3::AddCommonGenerationFields(Body, Model, Params.FaceLimit, Params.bUseFaceLimit, Params.Texture, Params.PBR, Params.TextureSeed, TripoV3::NormalizeTextureQuality(Params.TextureQuality), Params.AutoSize, Params.Quad);
	FString Content; FJsonSerializer::Serialize(Body.ToSharedRef(), TJsonWriterFactory<>::Create(&Content));
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::ImageToModelUrl, TEXT("POST"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json")); Request->SetContentAsString(Content);
	Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
	{
		TripoV3::UntrackRequest(Request); FString TaskID; TSharedPtr<FJsonObject> Data; const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
		const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::TryGetDataObject(Json, Data) && Data->TryGetStringField(TEXT("task_id"), TaskID) && !TaskID.IsEmpty();
		int32 ErrorCode = 0; FString ErrorMessage; if (!bSuccess) TripoV3::GetRequestError(Response, bConnected, Json, ErrorCode, ErrorMessage);
		Callback.ExecuteIfBound(bSuccess, TaskID, ErrorCode, ErrorMessage);
	});
	Request->ProcessRequest();
}

void UTripoBPLibrary::CreateModelFromMultiview(const FString&, const FMultiviewGenerationParams&, const FOnMultiviewTaskCreated& Callback)
{
	// The v3 contract has no supported multiview endpoint. Do not silently send a v2-shaped request.
	Callback.ExecuteIfBound(false, TEXT(""));
}

void UTripoBPLibrary::GetTaskStatus(const FString& ApiKey, const FString& TaskID, const FOnTaskStatusReceived& Callback)
{
	if (ApiKey.IsEmpty() || TaskID.IsEmpty())
	{
		Callback.ExecuteIfBound(false, FTaskStatusData());
		return;
	}
	const FString URL = TripoV3::BaseUrl + TEXT("/tasks/") + FGenericPlatformHttp::UrlEncode(TaskID);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(URL, TEXT("GET"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindLambda(
		[Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			TripoV3::UntrackRequest(Request);
			const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
			const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::GetCode(Json) == 0;
			FTaskStatusData Data = TripoV3::ParseTask(Json);
			if (!bSuccess) TripoV3::GetRequestError(Response, bConnected, Json, Data.ErrorCode, Data.ErrorMessage, &Data.RetryAfterSeconds);
			Callback.ExecuteIfBound(bSuccess, Data);
		});
	Request->ProcessRequest();
}

void UTripoBPLibrary::GetTaskStatusNative(const FString& ApiKey, const FString& TaskID, FOnTripoTaskStatusNative Callback)
{
	if (ApiKey.IsEmpty() || TaskID.IsEmpty()) { FTaskStatusData Empty; Callback.ExecuteIfBound(false, Empty); return; }
	const FString URL = TripoV3::BaseUrl + TEXT("/tasks/") + FGenericPlatformHttp::UrlEncode(TaskID);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(URL, TEXT("GET"), ApiKey); Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
	{
		TripoV3::UntrackRequest(Request); const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response); const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::GetCode(Json) == 0; FTaskStatusData Data = TripoV3::ParseTask(Json); if (!bSuccess) TripoV3::GetRequestError(Response, bConnected, Json, Data.ErrorCode, Data.ErrorMessage, &Data.RetryAfterSeconds); Callback.ExecuteIfBound(bSuccess, Data);
	}); Request->ProcessRequest();
}

void UTripoBPLibrary::PollTaskUntilComplete(const FString& ApiKey, const FString& TaskID, const FOnTaskStatusReceived& Callback)
{
	struct FPollState
	{
		FString ApiKey;
		FString TaskID;
		FOnTaskStatusReceived Callback;
		float Elapsed = 0.0f;
		float SinceLastPoll = 1.0f;
		float PollIntervalSeconds = 1.0f;
		int32 ConsecutiveFailures = 0;
		bool bRequestInFlight = false;
		bool bFinished = false;
	};
	const TSharedRef<FPollState, ESPMode::ThreadSafe> State = MakeShared<FPollState, ESPMode::ThreadSafe>();
	State->ApiKey = ApiKey;
	State->TaskID = TaskID;
	State->Callback = Callback;
	if (ApiKey.IsEmpty() || TaskID.IsEmpty())
	{
		Callback.ExecuteIfBound(false, FTaskStatusData());
		return;
	}

	TSharedPtr<TFunction<void()>> Poll;
	Poll = MakeShared<TFunction<void()>>();
	*Poll = [State]()
	{
		if (State->bFinished || State->bRequestInFlight) return;
		State->bRequestInFlight = true;
		const FString URL = TripoV3::BaseUrl + TEXT("/tasks/") + FGenericPlatformHttp::UrlEncode(State->TaskID);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(URL, TEXT("GET"), State->ApiKey);
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->OnProcessRequestComplete().BindLambda(
			[State](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
			{
				TripoV3::UntrackRequest(Request);
				State->bRequestInFlight = false;
				State->SinceLastPoll = 0.0f;
				const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
				const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::GetCode(Json) == 0;
				FTaskStatusData Status = TripoV3::ParseTask(Json);
				if (!bSuccess)
				{
					Status.Status = ETaskStatus::Failed;
					TripoV3::GetRequestError(Response, bConnected, Json, Status.ErrorCode, Status.ErrorMessage, &Status.RetryAfterSeconds);
					const bool bBackpressure = Status.ErrorCode == 429 || Status.ErrorCode == 1007 || Status.ErrorCode == 2000;
					if (bBackpressure)
					{
						State->ConsecutiveFailures = 0;
						State->PollIntervalSeconds = Status.RetryAfterSeconds > 0 ? static_cast<float>(Status.RetryAfterSeconds) : 5.0f;
						return;
					}
					++State->ConsecutiveFailures;
					State->PollIntervalSeconds = FMath::Min(4.0f, FMath::Pow(2.0f, static_cast<float>(State->ConsecutiveFailures)));
					if (State->ConsecutiveFailures >= 3)
					{
						State->bFinished = true;
						State->Callback.ExecuteIfBound(false, Status);
					}
					return;
				}
				State->ConsecutiveFailures = 0;
				State->PollIntervalSeconds = 1.0f;
				if (bSuccess && Status.Status != ETaskStatus::Success && Status.Status != ETaskStatus::Failed && Status.Status != ETaskStatus::Banned && Status.Status != ETaskStatus::Expired && Status.Status != ETaskStatus::Cancelled)
				{
					State->Callback.ExecuteIfBound(true, Status);
				}
				if (bSuccess && (Status.Status == ETaskStatus::Success || Status.Status == ETaskStatus::Failed || Status.Status == ETaskStatus::Banned || Status.Status == ETaskStatus::Expired || Status.Status == ETaskStatus::Cancelled))
				{
					State->bFinished = true;
					State->Callback.ExecuteIfBound(true, Status);
				}
			});
		Request->ProcessRequest();
	};
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[State, Poll](float DeltaSeconds)
		{
			if (State->bFinished) return false;
			State->Elapsed += DeltaSeconds;
			State->SinceLastPoll += DeltaSeconds;
			if (State->Elapsed >= 300.0f)
			{
				State->bFinished = true;
				FTaskStatusData Timeout;
				Timeout.Status = ETaskStatus::Failed;
				Timeout.ErrorMessage = TEXT("Task polling timed out after 300 seconds");
				State->Callback.ExecuteIfBound(false, Timeout);
				return false;
			}
			if (State->SinceLastPoll >= State->PollIntervalSeconds)
			{
				State->SinceLastPoll = 0.0f;
				(*Poll)();
			}
			return true;
		}), 0.0f);
}

void UTripoBPLibrary::ConvertModel(const FString& ApiKey, const FString& GenerationTaskID, const FOnTaskCreated& Callback)
{
	if (ApiKey.IsEmpty() || GenerationTaskID.IsEmpty())
	{
		Callback.ExecuteIfBound(false, TEXT(""));
		return;
	}
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("format"), TEXT("FBX"));
	Body->SetStringField(TEXT("input"), GenerationTaskID);
	FString Content;
	FJsonSerializer::Serialize(Body.ToSharedRef(), TJsonWriterFactory<>::Create(&Content));
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::ConvertUrl, TEXT("POST"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Content);
	Request->OnProcessRequestComplete().BindLambda(
		[Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			TripoV3::UntrackRequest(Request);
			FString TaskID;
			TSharedPtr<FJsonObject> Data;
			const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
			const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::TryGetDataObject(Json, Data) && Data->TryGetStringField(TEXT("task_id"), TaskID) && !TaskID.IsEmpty();
			Callback.ExecuteIfBound(bSuccess, TaskID);
		});
	Request->ProcessRequest();
}

void UTripoBPLibrary::ConvertModelNative(const FString& ApiKey, const FString& GenerationTaskID, FOnTripoTaskCreatedNative Callback)
{
	if (ApiKey.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("API key is required")); return; }
	if (GenerationTaskID.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("Generation task ID is required")); return; }
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>(); Body->SetStringField(TEXT("format"), TEXT("FBX")); Body->SetStringField(TEXT("input"), GenerationTaskID);
	FString Content; FJsonSerializer::Serialize(Body.ToSharedRef(), TJsonWriterFactory<>::Create(&Content)); TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::ConvertUrl, TEXT("POST"), ApiKey); Request->SetHeader(TEXT("Content-Type"), TEXT("application/json")); Request->SetContentAsString(Content);
	Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
	{
		TripoV3::UntrackRequest(Request); FString TaskID; TSharedPtr<FJsonObject> Data; const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response); const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::TryGetDataObject(Json, Data) && Data->TryGetStringField(TEXT("task_id"), TaskID) && !TaskID.IsEmpty();
		int32 ErrorCode = 0; FString ErrorMessage; if (!bSuccess) TripoV3::GetRequestError(Response, bConnected, Json, ErrorCode, ErrorMessage);
		Callback.ExecuteIfBound(bSuccess, TaskID, ErrorCode, ErrorMessage);
	}); Request->ProcessRequest();
}

void UTripoBPLibrary::UploadImage(const FString& ApiKey, const FString& ImagePath, const FOnImageUploaded& Callback)
{
	if (ApiKey.IsEmpty() || ImagePath.IsEmpty())
	{
		Callback.ExecuteIfBound(false, TEXT(""));
		return;
	}
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *ImagePath) || FileData.Num() == 0)
	{
		Callback.ExecuteIfBound(false, TEXT(""));
		return;
	}
	const FString Extension = FPaths::GetExtension(ImagePath).ToLower();
	if (Extension != TEXT("png") && Extension != TEXT("jpg") && Extension != TEXT("jpeg"))
	{
		Callback.ExecuteIfBound(false, TEXT(""));
		return;
	}
	const FString MimeType = Extension == TEXT("png") ? TEXT("image/png") : TEXT("image/jpeg");
	const FString Boundary = TEXT("TripoBoundary") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	TArray<uint8> Content;
	const FString Header = FString::Printf(TEXT("--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n"), *Boundary, *FPaths::GetCleanFilename(ImagePath), *MimeType);
	FTCHARToUTF8 HeaderUtf8(*Header);
	Content.Append(reinterpret_cast<const uint8*>(HeaderUtf8.Get()), HeaderUtf8.Length());
	Content.Append(FileData);
	const FString Footer = FString::Printf(TEXT("\r\n--%s--\r\n"), *Boundary);
	FTCHARToUTF8 FooterUtf8(*Footer);
	Content.Append(reinterpret_cast<const uint8*>(FooterUtf8.Get()), FooterUtf8.Length());

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::UploadUrl, TEXT("POST"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary));
	Request->SetContent(Content);
	Request->OnProcessRequestComplete().BindLambda(
		[Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			TripoV3::UntrackRequest(Request);
			FString Token;
			TSharedPtr<FJsonObject> Data;
			const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response);
			const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::TryGetDataObject(Json, Data) && Data->TryGetStringField(TEXT("file_token"), Token) && !Token.IsEmpty();
			Callback.ExecuteIfBound(bSuccess, Token);
		});
	Request->ProcessRequest();
}

void UTripoBPLibrary::UploadImageNative(const FString& ApiKey, const FString& ImagePath, FOnTripoImageUploadedNative Callback)
{
	if (ApiKey.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("API key is required")); return; }
	if (ImagePath.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("Image path is required")); return; }
	TArray<uint8> FileData; if (!FFileHelper::LoadFileToArray(FileData, *ImagePath) || FileData.Num() == 0) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("Image file could not be read")); return; }
	const FString Extension = FPaths::GetExtension(ImagePath).ToLower(); if (Extension != TEXT("png") && Extension != TEXT("jpg") && Extension != TEXT("jpeg")) { Callback.ExecuteIfBound(false, TEXT(""), -1, TEXT("Image must be PNG or JPEG")); return; }
	const FString MimeType = Extension == TEXT("png") ? TEXT("image/png") : TEXT("image/jpeg"); const FString Boundary = TEXT("TripoBoundary") + FGuid::NewGuid().ToString(EGuidFormats::Digits); TArray<uint8> Content;
	const FString Header = FString::Printf(TEXT("--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n"), *Boundary, *FPaths::GetCleanFilename(ImagePath), *MimeType); FTCHARToUTF8 HeaderUtf8(*Header); Content.Append(reinterpret_cast<const uint8*>(HeaderUtf8.Get()), HeaderUtf8.Length()); Content.Append(FileData); const FString Footer = FString::Printf(TEXT("\r\n--%s--\r\n"), *Boundary); FTCHARToUTF8 FooterUtf8(*Footer); Content.Append(reinterpret_cast<const uint8*>(FooterUtf8.Get()), FooterUtf8.Length());
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(TripoV3::UploadUrl, TEXT("POST"), ApiKey); Request->SetHeader(TEXT("Content-Type"), FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary)); Request->SetContent(Content);
	Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
	{
		TripoV3::UntrackRequest(Request); FString Token; TSharedPtr<FJsonObject> Data; const TSharedPtr<FJsonObject> Json = TripoV3::ParseJson(Response); const bool bSuccess = TripoV3::IsHttpSuccess(Response, bConnected) && TripoV3::TryGetDataObject(Json, Data) && Data->TryGetStringField(TEXT("file_token"), Token) && !Token.IsEmpty();
		int32 ErrorCode = 0; FString ErrorMessage; if (!bSuccess) TripoV3::GetRequestError(Response, bConnected, Json, ErrorCode, ErrorMessage);
		Callback.ExecuteIfBound(bSuccess, Token, ErrorCode, ErrorMessage);
	}); Request->ProcessRequest();
}

void UTripoBPLibrary::DownloadModel(const FString& ModelURL, const FOnModelDownloaded& Callback)
{
	if (ModelURL.IsEmpty())
	{
		Callback.ExecuteIfBound(false, TEXT(""));
		return;
	}
	// Editor downloads are temporary source files. Keep them out of Content so
	// Unreal does not create a second, empty-looking TripoModels asset folder.
	const FString BaseDir = TripoAssetPaths::StagingRoot();
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*BaseDir);
	FString PathPart;
	FString QueryPart;
	ModelURL.Split(TEXT("?"), &PathPart, &QueryPart);
	FString FileName = FPaths::GetCleanFilename(PathPart);
	if (FileName.IsEmpty()) FileName = TEXT("TripoModel.glb");
	if (FPaths::GetExtension(FileName).IsEmpty()) FileName += TEXT(".glb");
	for (TCHAR& Character : FileName)
	{
		if (FString(TEXT("\\/:*?\"<>|")).Contains(FString::Chr(Character))) Character = TEXT('_');
	}
	const FString FinalPath = BaseDir / FileName;
	const FString TempPath = FinalPath + TEXT(".part");
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(ModelURL, TEXT("GET"), TEXT(""), 300.0f);
	Request->SetHeader(TEXT("User-Agent"), TEXT("Unreal Engine"));
	Request->OnProcessRequestComplete().BindLambda(
		[FinalPath, TempPath, Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			TripoV3::UntrackRequest(Request);
			bool bSuccess = false;
			if (TripoV3::IsHttpSuccess(Response, bConnected) && Response->GetContent().Num() > 0 && FFileHelper::SaveArrayToFile(Response->GetContent(), *TempPath))
			{
				IFileManager::Get().Delete(*FinalPath, false, true);
				bSuccess = IFileManager::Get().Move(*FinalPath, *TempPath, true, true);
			}
			if (!bSuccess) IFileManager::Get().Delete(*TempPath, false, true);
			Callback.ExecuteIfBound(bSuccess, bSuccess ? FinalPath : TEXT(""));
		});
	Request->ProcessRequest();
}

void UTripoBPLibrary::DownloadModelNative(const FString& ModelURL, FOnTripoModelDownloadedNative Callback)
{
	DownloadModelNativeWithName(ModelURL, TEXT("TripoModel"), MoveTemp(Callback));
}

void UTripoBPLibrary::DownloadModelNativeWithName(const FString& ModelURL, const FString& DesiredBaseName, FOnTripoModelDownloadedNative Callback)
{
	if (ModelURL.IsEmpty()) { Callback.ExecuteIfBound(false, TEXT("")); return; }
	// Keep generated FBX source files outside Content. The importer creates the
	// actual Unreal assets under TripoAssetPaths::AssetImportRoot/TripoModel_N.
	const FString BaseDir = TripoAssetPaths::StagingRoot();
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*BaseDir);
	FString SafeBaseName = DesiredBaseName.IsEmpty() ? TEXT("TripoModel") : DesiredBaseName;
	for (TCHAR& Character : SafeBaseName)
	{
		if (!FChar::IsAlnum(Character) && Character != TEXT('_')) Character = TEXT('_');
	}
	if (SafeBaseName.IsEmpty()) SafeBaseName = TEXT("TripoModel");
	const FString ModelDir = BaseDir / SafeBaseName;
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ModelDir);
	const FString FinalPath = ModelDir / (SafeBaseName + TEXT(".fbx"));
	const FString TempPath = FinalPath + TEXT(".part");
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = TripoV3::CreateRequest(ModelURL, TEXT("GET"), TEXT(""), 300.0f); Request->SetHeader(TEXT("User-Agent"), TEXT("Unreal Engine"));
	Request->OnProcessRequestComplete().BindLambda([FinalPath, TempPath, Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
	{
		TripoV3::UntrackRequest(Request); bool bSuccess = false; if (TripoV3::IsHttpSuccess(Response, bConnected) && Response->GetContent().Num() > 0 && FFileHelper::SaveArrayToFile(Response->GetContent(), *TempPath)) { IFileManager::Get().Delete(*FinalPath, false, true); bSuccess = IFileManager::Get().Move(*FinalPath, *TempPath, true, true); } if (!bSuccess) IFileManager::Get().Delete(*TempPath, false, true); Callback.ExecuteIfBound(bSuccess, bSuccess ? FinalPath : TEXT(""));
	}); Request->ProcessRequest();
}

bool UTripoBPLibrary::LoadAPIKeyFromFile(FString& OutApiKey)
{
	const FString FilePath = FPaths::ProjectConfigDir() + TEXT("TripoAPIKey.txt");
	if (!FFileHelper::LoadFileToString(OutApiKey, *FilePath)) return false;
	OutApiKey = OutApiKey.TrimStartAndEnd();
	return !OutApiKey.IsEmpty();
}
