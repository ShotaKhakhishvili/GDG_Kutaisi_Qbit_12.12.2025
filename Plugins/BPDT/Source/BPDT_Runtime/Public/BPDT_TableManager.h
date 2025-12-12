#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPDT_Table.h"
#include "BPDT_TableManager.generated.h"

UCLASS()
class BPDT_RUNTIME_API UBPDT_TableManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/* ---- Table lifecycle ---- */

	UFUNCTION(BlueprintCallable, Category = "BPDT|Table")
	static bool CreateEmptyTable(
		const FString& TableName,
		bool bSerialPrimaryKey
	);


	UFUNCTION(BlueprintCallable, Category="BPDT|Table")
	static bool RemoveTable(const FString& TableName);

	/* ---- Schema ops ---- */

	UFUNCTION(BlueprintCallable, Category="BPDT|Table")
	static bool AddColumn(
		const FString& TableName,
		FName ColumnName,
		EBPDT_CellType Type,
		const TArray<uint8>& DefaultData
	);

	UFUNCTION(BlueprintCallable, Category="BPDT|Table")
	static bool ConvertTableToExplicitPK(
		const FString& TableName,
		FName NewPKColumnName,
		EBPDT_CellType Type,
		const TArray<uint8>& DefaultData
	);

	UFUNCTION(BlueprintCallable, Category="BPDT|Debug")
	static void PrintTable(const FString& TableName);

private:
	static TMap<FString, FBPDT_Table>& GetTables();
};
