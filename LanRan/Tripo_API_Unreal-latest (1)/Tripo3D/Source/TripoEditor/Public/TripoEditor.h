#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"

/** Shared Slate style and Unity-sourced artwork for the Tripo editor UI. */
class TRIPOEDITOR_API FTripoEditorStyle
{
public:
    static void Initialize();
    static void Shutdown();
    static FName GetStyleSetName();
    static const ISlateStyle& Get();

private:
    static TSharedPtr<class FSlateStyleSet> StyleInstance;
};

class FTripoEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void OpenTripoWindow(const struct FToolMenuContext& Context);
    TSharedRef<class SDockTab> SpawnTripoTab(const class FSpawnTabArgs& Args);
    FDelegateHandle ToolMenusStartupHandle;
    FName TripoTabName = FName(TEXT("TripoApiEditor"));
};
