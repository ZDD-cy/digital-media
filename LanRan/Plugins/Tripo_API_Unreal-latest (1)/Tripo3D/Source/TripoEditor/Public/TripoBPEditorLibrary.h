#pragma once

#include "CoreMinimal.h"
#include "TripoBPLibrary.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TripoBPEditorLibrary.generated.h"

UCLASS()
class TRIPOEDITOR_API UTripoBPEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    DECLARE_DELEGATE_TwoParams(FOnTripoFileSelectedNative, bool, const FString&);
    // 资产导入相关函数
    UFUNCTION(BlueprintCallable, Category = "Tripo|Editor")
    static UAssetImportTask* CreateImportTask(FString SourcePath, FString DestinationPath, bool& bOutSuccess, FString& OutInfoMessage);

    UFUNCTION(BlueprintCallable, Category = "Tripo|Editor")
    static UObject* ProcessImportTask(UAssetImportTask* ImportTask, bool& bOutSuccess, FString& OutInfoMessage);

    UFUNCTION(BlueprintCallable, Category = "Tripo|Editor")
    static UObject* ImportModelAsset(FString SourcePath, FString DestinationPath, bool& bOutSuccess, FString& OutInfoMessage);

    UFUNCTION(BlueprintCallable, Category = "Tripo|Editor")
    static void ImportModelToEditor(const FString& FilePath, const FOnAssetImported& OnComplete);
    static void ImportModelToEditorNative(const FString& FilePath, FOnTripoAssetImportedNative OnComplete);
    static void ImportModelToEditorNativeNamed(const FString& FilePath, const FString& ModelBaseName, FOnTripoAssetImportedNative OnComplete);
    static FString GetNextTripoModelBaseName();

    // 文件对话框
    UFUNCTION(BlueprintCallable, Category = "Tripo|Editor")
    static void ShowFileDialog(bool bSave, const FString& Title, const FString& DefaultPath, 
        const FString& DefaultFile, const FString& FileTypes, const FOnFileSelected& OnFileSelected);
    static void ShowFileDialogNative(bool bSave, const FString& Title, const FString& DefaultPath,
        const FString& DefaultFile, const FString& FileTypes, FOnTripoFileSelectedNative OnFileSelected);

    UFUNCTION(BlueprintCallable, Category = "Tripo|Editor")
    static bool OpenImagePicker(FString& OutFilePath, UTexture2D*& OutTexture, 
        const FString& DialogTitle = TEXT("Select Image"), 
        const FString& FileTypes = TEXT("Image Files|*.png;*.jpg;*.jpeg"));

    // API密钥管理
    UFUNCTION(BlueprintCallable, Category = "Tripo|Editor")
    static bool SaveAPIKeyToFile(const FString& ApiKey);

    // 添加其他编辑器特定的函数...
};
