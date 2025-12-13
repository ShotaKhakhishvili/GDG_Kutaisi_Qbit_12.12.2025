#pragma once

#include "CoreMinimal.h"
#include "BPDT_Types.h"
#include "BPDT_TableViewTypes.generated.h"

USTRUCT(BlueprintType)
struct FBPDT_ColumnView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName ColumnName;

	UPROPERTY(BlueprintReadOnly)
	EBPDT_CellType Type;

	// One value per row (column-major)
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Values;

	// Default value of this column, stringified
	UPROPERTY(BlueprintReadOnly)
	FString DefaultValue;

	// Max string length in this column (0 if not a string column)
	UPROPERTY(BlueprintReadOnly)
	int32 MaxStringLength = 0;
};

USTRUCT(BlueprintType)
struct FBPDT_TableSchemaView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString TableName;

	UPROPERTY(BlueprintReadOnly)
	TArray<FName> ColumnNames;

	UPROPERTY(BlueprintReadOnly)
	TArray<EBPDT_CellType> ColumnTypes;
};
