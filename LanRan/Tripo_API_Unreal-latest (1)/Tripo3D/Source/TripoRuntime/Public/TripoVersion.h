#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/Launch/Resources/Version.h"
#include "TripoVersion.generated.h"

USTRUCT(BlueprintType)
struct TRIPORUNTIME_API FTripoVersionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Tripo|Version")
    int32 MajorVersion = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tripo|Version")
    int32 MinorVersion = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tripo|Version")
    int32 PatchVersion = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tripo|Version")
    FString VersionString;

    FTripoVersionInfo()
    {
        MajorVersion = ENGINE_MAJOR_VERSION;
        MinorVersion = ENGINE_MINOR_VERSION;
        PatchVersion = ENGINE_PATCH_VERSION;
        VersionString = FString::Printf(TEXT("%d.%d.%d"), MajorVersion, MinorVersion, PatchVersion);
    }
};

UCLASS(meta=(ScriptName="TripoVersion"))
class TRIPORUNTIME_API UTripoVersionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Tripo|Version", meta=(DisplayName="Get Engine Version"))
    static FTripoVersionInfo GetEngineVersion();

    UFUNCTION(BlueprintCallable, Category = "Tripo|Version", meta=(DisplayName="Is Version Supported"))
    static bool IsVersionSupported(int32 MinMajorVersion = 5, int32 MinMinorVersion = 0);
}; 