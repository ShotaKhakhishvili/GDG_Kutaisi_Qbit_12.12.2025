#include "BPDT_Editor.h"

#include "ToolMenus.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilitySubsystem.h"
#include "Editor.h"
#include "TickableEditorObject.h"

IMPLEMENT_MODULE(FBPDT_EditorModule, BPDT_Editor)

void FBPDT_EditorModule::StartupModule()
{
#if WITH_EDITOR
    UE_LOG(LogTemp, Warning, TEXT("[BPDT_Editor] StartupModule() called."));
    FCoreDelegates::OnPostEngineInit.AddRaw(this, &FBPDT_EditorModule::InitializeToolbarLater);
#endif
}

void FBPDT_EditorModule::ShutdownModule()
{
#if WITH_EDITOR
    UE_LOG(LogTemp, Warning, TEXT("[BPDT_Editor] ShutdownModule() called."));
    FCoreDelegates::OnPostEngineInit.RemoveAll(this);
#endif
}

void FBPDT_EditorModule::InitializeToolbarLater()
{
    UE_LOG(LogTemp, Warning, TEXT("[BPDT_Editor] InitializeToolbarLater() — starting ticker."));
    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateRaw(this, &FBPDT_EditorModule::TickInitializeToolbar)
    );
}

bool FBPDT_EditorModule::TickInitializeToolbar(float DeltaTime)
{
    static int32 TickCount = 0;
    TickCount++;

    if (UToolMenus::Get() == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Tick %d] ToolMenus NOT ready yet — waiting."), TickCount);
        return true; // keep ticking
    }

    UToolMenus* TM = UToolMenus::Get();

    UToolMenu* Menu = TM->ExtendMenu("LevelEditor.LevelEditorToolBar");
    if (!Menu)
    {
        UE_LOG(LogTemp, Error, TEXT("[Tick %d] FAILED to access LevelEditor.LevelEditorToolBar menu!"), TickCount);
        return true; // maybe next tick
    }

    if (!Menu->FindSection("User"))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Tick %d] 'User' section does NOT exist — creating it."), TickCount);
        Menu->AddSection("User", FText::FromString("User Tools"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Tick %d] 'User' section ALREADY exists."), TickCount);
    }

    const FString Path = TEXT("/BPDT/UI/ToolbarStuff/EUB_Toolbar.EUB_Toolbar");

    UEditorUtilityBlueprint* BP = LoadObject<UEditorUtilityBlueprint>(nullptr, *Path);

    if (!BP)
    {
        UE_LOG(LogTemp, Error, TEXT("[Tick %d] FAILED to load EUB_Toolbar ! PATH INVALID."), TickCount);
        return true; // keep ticking so you can see repeated fails
    }

    if (!BP->GeneratedClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Tick %d] Blueprint has NO GeneratedClass! Cannot run."), TickCount);
        return false;
    }

    UObject* CDO = BP->GeneratedClass->GetDefaultObject();
    UFunction* RunFunc = CDO->FindFunction(TEXT("Run"));

    if (!RunFunc)
    {
        UE_LOG(LogTemp, Error, TEXT("[Tick %d] ERROR: Blueprint DOES NOT CONTAIN 'Run' FUNCTION!"), TickCount);
        return false;
    }

    CDO->ProcessEvent(RunFunc, nullptr);
    TM->RefreshAllWidgets();

    UE_LOG(LogTemp, Warning, TEXT("[BPDT_Editor] Toolbar initialization COMPLETE."));

    return false; // Stop ticking forever
}
