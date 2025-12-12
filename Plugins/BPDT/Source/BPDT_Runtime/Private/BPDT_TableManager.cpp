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

	const TArray<FBPDT_Column>& Columns = Table->GetColumns();

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("========== BPDT TABLE: %s =========="), *TableName);

	// ---- Schema ----
	UE_LOG(LogTemp, Warning, TEXT("Columns (%d):"), Columns.Num());
	for (int32 i = 0; i < Columns.Num(); ++i)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("  [%d] %s | Type=%d | Size=%d"),
			i,
			*Columns[i].Name.ToString(),
			(int32)Columns[i].Type,
			Columns[i].ByteSize
		);
	}

	UE_LOG(LogTemp, Warning, TEXT(""));

	// ---- Rows ----
	int32 RowCounter = 0;

	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey& PK, const FBPDT_Row& Row)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Row %d | PK = %s"),
				RowCounter++,
				*PK.ToString()
			);

			for (int32 ColIndex = 0; ColIndex < Columns.Num(); ++ColIndex)
			{
				const FBPDT_Cell& Cell = Row.GetCell(ColIndex);
				const FBPDT_Column& Col = Columns[ColIndex];

				FString ValueStr = TEXT("<null>");

				if (!Cell.bIsNull)
				{
					switch (Cell.Type)
					{
					case EBPDT_CellType::Int:
					{
						int32 V;
						FMemory::Memcpy(&V, Cell.Data.GetData(), sizeof(int32));
						ValueStr = FString::FromInt(V);
						break;
					}
					case EBPDT_CellType::Float:
					{
						float V;
						FMemory::Memcpy(&V, Cell.Data.GetData(), sizeof(float));
						ValueStr = FString::SanitizeFloat(V);
						break;
					}
					case EBPDT_CellType::Bool:
					{
						bool V;
						FMemory::Memcpy(&V, Cell.Data.GetData(), sizeof(bool));
						ValueStr = V ? TEXT("true") : TEXT("false");
						break;
					}
					case EBPDT_CellType::Vector3:
					{
						FVector V;
						FMemory::Memcpy(&V, Cell.Data.GetData(), sizeof(FVector));
						ValueStr = V.ToString();
						break;
					}
					case EBPDT_CellType::String:
					{
						ValueStr = FString(UTF8_TO_TCHAR(Cell.Data.GetData()));
						break;
					}
					default:
						ValueStr = TEXT("<unknown>");
						break;
					}
				}

				UE_LOG(
					LogTemp,
					Warning,
					TEXT("    %s = %s"),
					*Col.Name.ToString(),
					*ValueStr
				);
			}
		}
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("========== END TABLE (%d rows) =========="),
		Table->GetRowCount()
	);
	UE_LOG(LogTemp, Warning, TEXT(""));
}

bool UBPDT_TableManager::AddIntColumn(
	const FString& TableName,
	FName ColumnName,
	int32 DefaultValue
)
{
	return AddColumn_Typed(
		TableName,
		ColumnName,
		EBPDT_CellType::Int,
		DefaultValue
	);
}

bool UBPDT_TableManager::AddFloatColumn(
	const FString& TableName,
	FName ColumnName,
	float DefaultValue
)
{
	return AddColumn_Typed(
		TableName,
		ColumnName,
		EBPDT_CellType::Float,
		DefaultValue
	);
}

bool UBPDT_TableManager::AddBoolColumn(
	const FString& TableName,
	FName ColumnName,
	bool DefaultValue
)
{
	return AddColumn_Typed(
		TableName,
		ColumnName,
		EBPDT_CellType::Bool,
		DefaultValue
	);
}

bool UBPDT_TableManager::AddVector3Column(
	const FString& TableName,
	FName ColumnName,
	FVector DefaultValue
)
{
	return AddColumn_Typed(
		TableName,
		ColumnName,
		EBPDT_CellType::Vector3,
		DefaultValue
	);
}

bool UBPDT_TableManager::AddDefaultRow(const FString& TableName)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	Table->InsertRowAsDefault();
	return true;
}

bool UBPDT_TableManager::SetCellInt(
	const FString& TableName,
	const FString& PKValue,
	FName ColumnName,
	int32 Value
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	return SetCellTyped(
		Table,
		PKValue,
		ColumnName,
		EBPDT_CellType::Int,
		Value
	);
}

bool UBPDT_TableManager::SetCellFloat(
	const FString& TableName,
	const FString& PKValue,
	FName ColumnName,
	float Value
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	return SetCellTyped(
		Table,
		PKValue,
		ColumnName,
		EBPDT_CellType::Float,
		Value
	);
}

bool UBPDT_TableManager::SetCellBool(
	const FString& TableName,
	const FString& PKValue,
	FName ColumnName,
	bool Value
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	return SetCellTyped(
		Table,
		PKValue,
		ColumnName,
		EBPDT_CellType::Bool,
		Value
	);
}

bool UBPDT_TableManager::SetCellVector3(
	const FString& TableName,
	const FString& PKValue,
	FName ColumnName,
	FVector Value
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	return SetCellTyped(
		Table,
		PKValue,
		ColumnName,
		EBPDT_CellType::Vector3,
		Value
	);
}

