#include "BPDT_TableManager.h"

static TMap<FString, FBPDT_Table> G_BPDT_Tables;

TMap<FString, FBPDT_Table>& UBPDT_TableManager::GetTables()
{
	return G_BPDT_Tables;
}

bool UBPDT_TableManager::CreateEmptyTable(
	const FString& TableName,
	bool bSerialPrimaryKey
)
{
	if (TableName.IsEmpty() || GetTables().Contains(TableName))
	{
		return false;
	}

	FBPDT_Table Table;

	if (bSerialPrimaryKey)
	{
		Table.InitSerial({});
	}
	else
	{
		Table.InitSerial({});
	}

	GetTables().Add(TableName, Table);
	return true;
}


bool UBPDT_TableManager::RemoveTable(const FString& TableName)
{
	return GetTables().Remove(TableName) > 0;
}

/* ---------------- Schema ops ---------------- */

bool UBPDT_TableManager::AddColumn(
	const FString& TableName,
	FName ColumnName,
	EBPDT_CellType Type,
	const TArray<uint8>& DefaultData
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table || DefaultData.Num() == 0)
	{
		return false;
	}

	return Table->AddColumn(
		ColumnName,
		Type,
		DefaultData.GetData(),
		DefaultData.Num()
	);
}

bool UBPDT_TableManager::ConvertTableToExplicitPK(
	const FString& TableName,
	FName NewPKColumnName,
	EBPDT_CellType Type,
	const TArray<uint8>& DefaultData
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table || DefaultData.Num() == 0)
	{
		return false;
	}

	return Table->ConvertSerialToExplicit(
		NewPKColumnName,
		Type,
		DefaultData.GetData(),
		DefaultData.Num()
	);
}

/* ---------------- Debug ---------------- */

void UBPDT_TableManager::PrintTable(const FString& TableName)
{
	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BPDT] Table '%s' not found"), *TableName);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("==== BPDT TABLE: %s ===="), *TableName);

	for (const FBPDT_Column& Col : Table->Columns)
	{
		UE_LOG(LogTemp, Warning, TEXT("Column: %s"), *Col.Name.ToString());
	}

	UE_LOG(LogTemp, Warning, TEXT("Rows: %d"), Table->GetRowCount());
}
