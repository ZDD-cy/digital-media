#include "TripoBPEditorLibrary.h"
#include "TripoAssetPaths.h"
#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "AssetImportTask.h"
#include "IDesktopPlatform.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/FbxTextureImportData.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"

UAssetImportTask* UTripoBPEditorLibrary::CreateImportTask(FString SourcePath, FString DestinationPath, bool& bOutSuccess, FString& OutInfoMessage)
{
    // 创建导入任务
    UAssetImportTask* RetTask = NewObject<UAssetImportTask>();
    if (!RetTask)
    {
        bOutSuccess = false;
        OutInfoMessage = FString::Printf(TEXT("Failed to create import task for: %s"), *SourcePath);
        return nullptr;
    }

    FString PackagePath = DestinationPath;
    if (!PackagePath.StartsWith(TEXT("/Game/")))
    {
        if (PackagePath.StartsWith(TEXT("Content/")))
        {
            PackagePath = TEXT("/Game/") + PackagePath.RightChop(8);
        }
        else
        {
            PackagePath = FString(TEXT("/Game/")) + PackagePath;
        }
    }

    PackagePath.RemoveFromEnd(TEXT("/"));
    RetTask->Filename = SourcePath;
    RetTask->DestinationPath = PackagePath;
    // Keep the primary asset name aligned with the task's destination folder.
    // The downloaded file is already named after the reserved TripoModel_N id,
    // but using the explicit destination name also protects callers that pass a
    // differently named source file.
    RetTask->DestinationName = FPaths::GetBaseFilename(SourcePath);

    // 设置基本选项（不变）
    RetTask->bSave = true;
    RetTask->bAutomated = true;
    RetTask->bAsync = false;
    RetTask->bReplaceExisting = false;
    RetTask->bReplaceExistingSettings = false;

    // Tripo FBX is imported into UE's Z-up, centimetre space by the engine
    // importer; avoid applying a second manual scale in the plugin.
    UFbxImportUI* ImportUI = NewObject<UFbxImportUI>(RetTask);
    if (ImportUI)
    {
        // The queue only accepts converted FBX geometry. Do not let FBX's
        // automatic detector switch this task back to skeletal/animation mode.
        ImportUI->bAutomatedImportShouldDetectType = false;
        ImportUI->bImportMesh = true;
        ImportUI->MeshTypeToImport = FBXIT_StaticMesh;
        ImportUI->bImportAsSkeletal = false;
        ImportUI->bImportMaterials = true;
        ImportUI->bImportTextures = true;
        ImportUI->bImportAnimations = false;
        if (ImportUI->StaticMeshImportData)
        {
            ImportUI->StaticMeshImportData->bConvertScene = true;
            ImportUI->StaticMeshImportData->bConvertSceneUnit = true;
            ImportUI->StaticMeshImportData->ImportUniformScale = 1.0f;
            ImportUI->StaticMeshImportData->bCombineMeshes = false;
            // Preserve the source model's root-node scale and orientation. FBX
            // conversions can store the unit scale on that node, so skipping
            // it changes the imported mesh dimensions.
            ImportUI->StaticMeshImportData->bTransformVertexToAbsolute = true;
            // Keep the authored pivot instead of baking it into the vertices.
            ImportUI->StaticMeshImportData->bBakePivotInVertex = false;
        }
        if (ImportUI->TextureImportData)
        {
            // Search only inside this model's folder. Concurrent imports must not
            // resolve a same-named material from another task or from /Game.
            ImportUI->TextureImportData->MaterialSearchLocation = EMaterialSearchLocation::Local;
        }
        RetTask->Options = ImportUI;
    }

    bOutSuccess = true;
    OutInfoMessage = FString::Printf(TEXT("Import task created for: %s"), *SourcePath);
    return RetTask;
}

UObject* UTripoBPEditorLibrary::ProcessImportTask(UAssetImportTask* ImportTask, bool& bOutSuccess, FString& OutInfoMessage)
{
    if (!ImportTask)
    {
        bOutSuccess = false;
        OutInfoMessage = TEXT("Invalid import task");
        return nullptr;
    }

    // 获取AssetTools模块
    FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");
    if (!AssetToolsModule)
    {
        bOutSuccess = false;
        OutInfoMessage = FString::Printf(TEXT("AssetTools module missing for: %s"), *ImportTask->Filename);
        return nullptr;
    }

    // 执行导入
    AssetToolsModule->Get().ImportAssetTasks({ ImportTask });

    // 验证导入结果
    if (ImportTask->GetObjects().Num() == 0)
    {
        bOutSuccess = false;
        OutInfoMessage = FString::Printf(TEXT("No assets imported for: %s"), *ImportTask->Filename);
        return nullptr;
    }

    UObject* ImportedAsset = nullptr;
    for (UObject* Object : ImportTask->GetObjects())
    {
        if (Object && Object->IsA<UStaticMesh>())
        {
            ImportedAsset = Object;
            break;
        }
    }
    if (!ImportedAsset)
    {
        ImportedAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *(ImportTask->DestinationPath / ImportTask->DestinationName));
    }

    bOutSuccess = true;
    OutInfoMessage = FString::Printf(TEXT("Imported asset: %s"), *ImportTask->Filename);
    return ImportedAsset;
}

UObject* UTripoBPEditorLibrary::ImportModelAsset(FString SourcePath, FString DestinationPath, bool& bOutSuccess, FString& OutInfoMessage)
{
    UAssetImportTask* Task = CreateImportTask(SourcePath, DestinationPath, bOutSuccess, OutInfoMessage);
    if (!bOutSuccess)
    {
        return nullptr;
    }

    // Import the asset
    UObject* RetAsset = ProcessImportTask(Task, bOutSuccess, OutInfoMessage);
    if (!bOutSuccess)
    {
        return nullptr;
    }

    bOutSuccess = true;
    OutInfoMessage = FString::Printf(TEXT("Import Asset Succeeded - '%s'"), *DestinationPath);
    return RetAsset;
}

void UTripoBPEditorLibrary::ImportModelToEditor(const FString& FilePath, const FOnAssetImported& OnComplete)
{
    // 创建导入任务
    bool bSuccess;
    FString InfoMessage;
    UObject* ImportedAsset = ImportModelAsset(FilePath, TripoAssetPaths::AssetImportRoot, bSuccess, InfoMessage);

    OnComplete.ExecuteIfBound(bSuccess, ImportedAsset);
}

void UTripoBPEditorLibrary::ShowFileDialog(bool bSave, const FString& Title, const FString& DefaultPath, 
    const FString& DefaultFile, const FString& FileTypes, const FOnFileSelected& OnFileSelected)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        OnFileSelected.ExecuteIfBound(false, TEXT(""));
        return;
    }

    TArray<FString> OutFilenames;
    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

    bool bResult;
    if (bSave)
    {
        bResult = DesktopPlatform->SaveFileDialog(
            ParentWindowHandle,
            Title,
            DefaultPath,
            DefaultFile,
            FileTypes,
            EFileDialogFlags::None,
            OutFilenames
        );
    }
    else
    {
        bResult = DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            Title,
            DefaultPath,
            DefaultFile,
            FileTypes,
            EFileDialogFlags::None,
            OutFilenames
        );
    }

    if (bResult && OutFilenames.Num() > 0)
    {
        OnFileSelected.ExecuteIfBound(true, OutFilenames[0]);
    }
    else
    {
        OnFileSelected.ExecuteIfBound(false, TEXT(""));
    }
}

bool UTripoBPEditorLibrary::OpenImagePicker(FString& OutFilePath, UTexture2D*& OutTexture, 
    const FString& DialogTitle, const FString& FileTypes)
{
    TArray<FString> OutFiles;
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

    if (DesktopPlatform)
    {
        const bool bOpened = DesktopPlatform->OpenFileDialog(
            FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
            DialogTitle,
            FPaths::ProjectContentDir(),
            TEXT(""),
            FileTypes,
            EFileDialogFlags::None,
            OutFiles
        );

        if (bOpened && OutFiles.Num() > 0)
        {
            OutFilePath = OutFiles[0];
            OutTexture = FImageUtils::ImportFileAsTexture2D(OutFilePath);
            return true;
        }
    }

    OutFilePath = TEXT("");
    OutTexture = nullptr;
    return false;
}

bool UTripoBPEditorLibrary::SaveAPIKeyToFile(const FString& ApiKey)
{
    if (ApiKey.IsEmpty()) return false;

    // 获取配置文件路径
    const FString FilePath = FPaths::ProjectConfigDir() + TEXT("TripoAPIKey.txt");

    // 使用FFileHelper将字符串保存到文件
    return FFileHelper::SaveStringToFile(
        ApiKey,
        *FilePath,
        FFileHelper::EEncodingOptions::ForceUTF8
    );
}

void UTripoBPEditorLibrary::ShowFileDialogNative(bool bSave, const FString& Title, const FString& DefaultPath,
    const FString& DefaultFile, const FString& FileTypes, FOnTripoFileSelectedNative OnFileSelected)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform) { OnFileSelected.ExecuteIfBound(false, TEXT("")); return; }
    TArray<FString> OutFilenames;
    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
    const bool bResult = bSave
        ? DesktopPlatform->SaveFileDialog(ParentWindowHandle, Title, DefaultPath, DefaultFile, FileTypes, EFileDialogFlags::None, OutFilenames)
        : DesktopPlatform->OpenFileDialog(ParentWindowHandle, Title, DefaultPath, DefaultFile, FileTypes, EFileDialogFlags::None, OutFilenames);
    OnFileSelected.ExecuteIfBound(bResult && OutFilenames.Num() > 0, bResult && OutFilenames.Num() > 0 ? OutFilenames[0] : TEXT(""));
}

void UTripoBPEditorLibrary::ImportModelToEditorNative(const FString& FilePath, FOnTripoAssetImportedNative OnComplete)
{
    ImportModelToEditorNativeNamed(FilePath, FPaths::GetBaseFilename(FilePath), MoveTemp(OnComplete));
}

void UTripoBPEditorLibrary::ImportModelToEditorNativeNamed(const FString& FilePath, const FString& ModelBaseName, FOnTripoAssetImportedNative OnComplete)
{
    bool bSuccess = false;
    FString InfoMessage;
    FString SafeName = ModelBaseName.IsEmpty() ? TEXT("TripoModel") : ModelBaseName;
    for (TCHAR& Character : SafeName)
    {
        if (!FChar::IsAlnum(Character) && Character != TEXT('_')) Character = TEXT('_');
    }
    const FString AssetFolder = TripoAssetPaths::AssetImportRoot / SafeName;
    // The file and destination are both scoped to this task. This prevents the
    // FBX importer from resolving a same-named material from another queued job.
    UAssetImportTask* ImportTask = CreateImportTask(FilePath, AssetFolder, bSuccess, InfoMessage);
    if (ImportTask)
    {
        // Do not let the remote conversion endpoint's name (historically
        // "convert") leak into the Unreal asset name. For a single-mesh FBX,
        // the FBX factory honors bOverrideFullName and uses this exact name.
        ImportTask->DestinationName = SafeName;
        if (UFbxImportUI* ImportUI = Cast<UFbxImportUI>(ImportTask->Options))
        {
            ImportUI->bOverrideFullName = true;
        }
    }
    UObject* ImportedAsset = ProcessImportTask(ImportTask, bSuccess, InfoMessage);

    // Unity only imports the generated FBX and previews the resulting asset in
    // its editor window. It does not instantiate a model in the active scene,
    // so keep the Unreal behavior aligned and preserve the FBX pivot as-is.
    OnComplete.ExecuteIfBound(bSuccess, ImportedAsset);
}

FString UTripoBPEditorLibrary::GetNextTripoModelBaseName()
{
    const FString DownloadDir = TripoAssetPaths::StagingRoot();
    const FString AssetDir = TripoAssetPaths::AssetImportRootOnDisk();
    int32 Index = 1;
    while (true)
    {
        const FString Candidate = FString::Printf(TEXT("TripoModel_%d"), Index++);
        const FString MeshPackage = TripoAssetPaths::AssetImportRoot / Candidate / Candidate;
        if (!IFileManager::Get().DirectoryExists(*(DownloadDir / Candidate)) &&
            !IFileManager::Get().DirectoryExists(*(AssetDir / Candidate)) &&
            !FPackageName::DoesPackageExist(MeshPackage))
        {
            // Reserve the source folder now, matching Unity's Directory.Exists
            // allocation and preventing concurrent jobs from claiming one id.
            IFileManager::Get().MakeDirectory(*(DownloadDir / Candidate), true);
            return Candidate;
        }
    }
}
