#include "BPDT_Runtime.h"
#include "CoreMinimal.h"
#include "BPDT_FileManager.h"

IMPLEMENT_MODULE(FBPDT_RuntimeModule, BPDT_Runtime)

void FBPDT_RuntimeModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("BPDT Runtime Module Loaded")); 
    UBPDT_FileManager::LoadFile(TEXT("test.txt"));

}

void FBPDT_RuntimeModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("BPDT Runtime Module Unloaded"));
}
