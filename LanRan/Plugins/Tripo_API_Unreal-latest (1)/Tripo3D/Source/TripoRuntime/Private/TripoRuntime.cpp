#include "TripoRuntime.h"
#include "TripoVersion.h"
#include "TripoBPLibrary.h"

#define LOCTEXT_NAMESPACE "FTripoRuntimeModule"

void FTripoRuntimeModule::StartupModule()
{
    // 检查引擎版本是否支持
    if (!UTripoVersionLibrary::IsVersionSupported(5, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Tripo 3D Plugin requires Unreal Engine 5.0 or later."));
        return;
    }
}

void FTripoRuntimeModule::ShutdownModule()
{
	// v3 has no public cancel endpoint; cancel local HTTP work before unloading.
	UTripoBPLibrary::CancelAllRequests();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTripoRuntimeModule, TripoRuntime)
