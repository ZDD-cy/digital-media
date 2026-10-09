#include "TripoVersion.h"

FTripoVersionInfo UTripoVersionLibrary::GetEngineVersion()
{
    return FTripoVersionInfo();
}

bool UTripoVersionLibrary::IsVersionSupported(int32 MinMajorVersion, int32 MinMinorVersion)
{
    FTripoVersionInfo VersionInfo = GetEngineVersion();
    
    // 检查主版本号
    if (VersionInfo.MajorVersion < MinMajorVersion)
    {
        return false;
    }
    
    // 如果主版本号相同，检查次版本号
    if (VersionInfo.MajorVersion == MinMajorVersion && VersionInfo.MinorVersion < MinMinorVersion)
    {
        return false;
    }
    
    return true;
} 