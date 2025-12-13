#pragma once

#include "CoreMinimal.h"
#include "BPDT_Table.h"

class BPDT_RUNTIME_API FBPDT_FileManager
{
public:
	static bool WriteTable(
		const FString& TableName,
		const FBPDT_Table& Table
	);
	static bool ReadTable(
		const FString& TableName,
		FBPDT_Table& OutTable
	);

	static bool IsTableInRegistry(const FString& TableName);
	static bool ReadRegistry(TArray<FString>& OutTableNames);
private:
	static FString GetSaveDirectory();
	static bool AddTableToRegistry(const FString& TableName);
	static FString GetRegistryPath();

	static bool WriteSchemeFile(
		const FString& Path,
		const FString& TableName,
		const FBPDT_Table& Table
	);

	static bool WriteDataFile(
		const FString& Path,
		const FBPDT_Table& Table
	);

	static void WriteNullMask(
		FArchive& Ar,
		const FBPDT_Row& Row,
		int32 ColumnCount
	);
};
