#include "TripoEditor.h"

#include "Framework/Docking/TabManager.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenus.h"
#include "TripoVersion.h"
#include "Widgets/Docking/SDockTab.h"
#include "STripoEditorWidget.h"

#define LOCTEXT_NAMESPACE "FTripoEditorModule"

TSharedPtr<FSlateStyleSet> FTripoEditorStyle::StyleInstance;

FName FTripoEditorStyle::GetStyleSetName()
{
    static const FName StyleSetName(TEXT("TripoEditorStyle"));
    return StyleSetName;
}

void FTripoEditorStyle::Initialize()
{
    if (StyleInstance.IsValid()) return;

    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Tripo3D"));
    if (!Plugin.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Unable to locate the Tripo3D plugin resources."));
        return;
    }

    StyleInstance = MakeShared<FSlateStyleSet>(GetStyleSetName());
    StyleInstance->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
    StyleInstance->Set(TEXT("Tripo.Icon.Logo"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("Icon128"), TEXT(".png")), FVector2D(20.0f, 20.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.Key"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("tripo-icon-key"), TEXT(".png")), FVector2D(16.0f, 16.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.Enter"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("tripo-icon-enter"), TEXT(".png")), FVector2D(16.0f, 16.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.Check"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("tripo-icon-check"), TEXT(".png")), FVector2D(16.0f, 16.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.Text"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("tripo-icon-text"), TEXT(".png")), FVector2D(18.0f, 18.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.Image"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("tripo-icon-image"), TEXT(".png")), FVector2D(18.0f, 18.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.ImagePlus"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("tripo-icon-image-plus"), TEXT(".png")), FVector2D(42.0f, 42.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.Preview"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("tripo-icon-preview"), TEXT(".png")), FVector2D(16.0f, 16.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.Box"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("tripo-icon-box"), TEXT(".png")), FVector2D(64.0f, 64.0f)));
    StyleInstance->Set(TEXT("Tripo.Icon.Credits"), new FSlateImageBrush(StyleInstance->RootToContentDir(TEXT("coin"), TEXT(".png")), FVector2D(16.0f, 16.0f)));
    FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

void FTripoEditorStyle::Shutdown()
{
    if (!StyleInstance.IsValid()) return;
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    StyleInstance.Reset();
}

const ISlateStyle& FTripoEditorStyle::Get()
{
    return *StyleInstance;
}

void FTripoEditorModule::StartupModule()
{
    if (!UTripoVersionLibrary::IsVersionSupported(5, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Tripo 3D Plugin requires Unreal Engine 5.0 or later."));
        return;
    }

    FTripoEditorStyle::Initialize();

    // Register the panel as a nomad tab so it can be invoked and docked, but
    // hide it from the Window menu: the single user-facing entry point lives
    // under Tools -> Tripo (see RegisterMenus). Leaving the spawner visible here
    // would produce a duplicate "Tripo API" item in the Window menu.
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TripoTabName,
        FOnSpawnTab::CreateRaw(this, &FTripoEditorModule::SpawnTripoTab))
        .SetDisplayName(LOCTEXT("TripoTabTitle", "Tripo API"))
        .SetTooltipText(LOCTEXT("TripoTabTooltip", "Generate Tripo models from text or images"))
        .SetIcon(FSlateIcon(FTripoEditorStyle::GetStyleSetName(), "Tripo.Icon.Logo"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    ToolMenusStartupHandle = UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTripoEditorModule::RegisterMenus));
}

void FTripoEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TripoTabName);
    }
    FTripoEditorStyle::Shutdown();
}

void FTripoEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));

    // Add a dedicated, labelled "Tripo" section pinned to the top of the Tools
    // menu. Without an explicit label and insert position the entry is rendered
    // adjacent to the engine's Data Validation group, which makes it look like
    // it belongs to that submenu.
    FToolMenuSection& Section = Menu->FindOrAddSection(
        TEXT("Tripo"),
        LOCTEXT("TripoSectionLabel", "Tripo"),
        FToolMenuInsert(NAME_None, EToolMenuInsertType::First));

    FToolUIAction Action;
    Action.ExecuteAction = FToolMenuExecuteAction::CreateRaw(this, &FTripoEditorModule::OpenTripoWindow);
    Section.AddMenuEntry(
        TEXT("TripoApiWindow"),
        LOCTEXT("OpenTripoApi", "Tripo API"),
        LOCTEXT("OpenTripoApiTooltip", "Open the Tripo API generation window"),
        FSlateIcon(FTripoEditorStyle::GetStyleSetName(), "Tripo.Icon.Logo"),
        Action);
}

void FTripoEditorModule::OpenTripoWindow(const FToolMenuContext&)
{
    FGlobalTabmanager::Get()->TryInvokeTab(TripoTabName);
}

TSharedRef<SDockTab> FTripoEditorModule::SpawnTripoTab(const FSpawnTabArgs&)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(STripoEditorWidget)
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTripoEditorModule, TripoEditor)
