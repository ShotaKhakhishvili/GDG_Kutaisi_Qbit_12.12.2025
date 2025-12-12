#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPDT_FileManager.generated.h"

UCLASS()
class BPDT_RUNTIME_API UBPDT_FileManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Load file into static memory */
    static void LoadFile(const FString& FileName = TEXT("test.txt"));

    /** Returns data loaded from file */
    UFUNCTION(BlueprintCallable, Category = "BPDT")
    static void PrintLoadedData();

private:

    /** The loaded text from the file */
    static FString LoadedData;
};
