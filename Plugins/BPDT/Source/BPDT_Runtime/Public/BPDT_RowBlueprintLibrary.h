#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "BPDT_Row.h"
#include "BPDT_Types.h"

#include "BPDT_RowBlueprintLibrary.generated.h"

/**
 * Blueprint helper functions for reading data from FBPDT_RowView
 * NOTE:
 *  - RowView is pure data
 *  - All logic lives here (UE rule: no UFUNCTION in USTRUCT)
 */
UCLASS()
class BPDT_RUNTIME_API UBPDT_RowBlueprintLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "BPDT|Row")
    static bool GetCellAsInt(
        const FBPDT_RowView& Row,
        FName ColumnName,
        int32& OutValue
    );

    UFUNCTION(BlueprintCallable, Category = "BPDT|Row")
    static bool GetCellAsFloat(
        const FBPDT_RowView& Row,
        FName ColumnName,
        float& OutValue
    );

    UFUNCTION(BlueprintCallable, Category = "BPDT|Row")
    static bool GetCellAsBool(
        const FBPDT_RowView& Row,
        FName ColumnName,
        bool& OutValue
    );

    UFUNCTION(BlueprintCallable, Category = "BPDT|Row")
    static bool GetCellAsString(
        const FBPDT_RowView& Row,
        FName ColumnName,
        FString& OutValue
    );

    UFUNCTION(BlueprintCallable, Category = "BPDT|Row")
    static bool GetCellAsVector3(
        const FBPDT_RowView& Row,
        FName ColumnName,
        FVector& OutValue
    );
};
