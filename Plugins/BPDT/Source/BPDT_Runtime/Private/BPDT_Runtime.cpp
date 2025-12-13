#include "BPDT_Runtime.h"
#include "CoreMinimal.h"
#include "BPDT_TableManager.h"

IMPLEMENT_MODULE(FBPDT_RuntimeModule, BPDT_Runtime)

void FBPDT_RuntimeModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("[BPDT_Runtime] StartupModule"));

	// Load all saved tables on editor / standalone / packaged startup
	UBPDT_TableManager::LoadAllTables();

	UE_LOG(LogTemp, Log, TEXT("[BPDT_Runtime] All tables loaded"));
}

void FBPDT_RuntimeModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("[BPDT_Runtime] ShutdownModule"));
}
