#include "BPDT_FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"

FString UBPDT_FileManager::LoadedData;

void UBPDT_FileManager::LoadFile(const FString& FileName)
{
    // Find plugin folder
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("BPDT");
    if (!Plugin.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[BPDT] Plugin not found!"));
        return;
    }

    // Content/Data path
    FString FullPath = Plugin->GetContentDir() / TEXT("Data") / FileName;

    if (!FPaths::FileExists(FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[BPDT] File does not exist: %s"), *FullPath);
        return;
    }

    if (!FFileHelper::LoadFileToString(LoadedData, *FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[BPDT] Failed to read file: %s"), *FullPath);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[BPDT] Loaded Data: %s"), *LoadedData);
}

void UBPDT_FileManager::PrintLoadedData()
{
    UE_LOG(LogTemp, Warning, TEXT("[BPDT] LoadedData = %s"), *LoadedData);
}
