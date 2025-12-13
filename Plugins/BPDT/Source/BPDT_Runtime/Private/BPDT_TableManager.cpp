#include "BPDT_TableManager.h"

static TMap<FString, FBPDT_Table> G_BPDT_Tables;

TMap<FString, FBPDT_Table>& UBPDT_TableManager::GetTables()
{
	return G_BPDT_Tables;
}

bool UBPDT_TableManager::CreateTable(const FString& TableName)
{
	if (!IsValidBPDTIdentifier(TableName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BPDT] Invalid table name '%s'. Only [A-Za-z0-9_] allowed."),
			*TableName
		);
		return false;
	}

	if (GetTables().Contains(TableName))
	{
		return false;
	}

	FBPDT_Table Table;
	Table.InitSerial();

	GetTables().Add(TableName, MoveTemp(Table));
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
	UE_LOG(LogTemp, Warning, TEXT("==== TABLE: %s ===="), *TableName);

	// ---- Header ----
	FString Header;
	for (const FBPDT_Column& Col : Columns)
	{
		Header += FString::Printf(
			TEXT("[%s | %s | %d] "),
			*Col.Name.ToString(),
			*StaticEnum<EBPDT_CellType>()
			->GetNameStringByValue((int64)Col.Type),
			Col.ByteSize
		);
	}
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Header);

	// ---- Rows ----
	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey&, const FBPDT_Row& Row)
		{
			FString Line;
			for (int32 i = 0; i < Columns.Num(); ++i)
			{
				const FBPDT_Cell& Cell = Row.GetCell(i);

				FString ValueStr = TEXT("null");

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
					case EBPDT_CellType::Bool:
					{
						bool V;
						FMemory::Memcpy(&V, Cell.Data.GetData(), sizeof(bool));
						ValueStr = V ? TEXT("true") : TEXT("false");
						break;
					}
					case EBPDT_CellType::Float:
					{
						float V;
						FMemory::Memcpy(&V, Cell.Data.GetData(), sizeof(float));
						ValueStr = FString::SanitizeFloat(V);
						break;
					}
					case EBPDT_CellType::Vector3:
					{
						FVector V;
						FMemory::Memcpy(&V, Cell.Data.GetData(), sizeof(FVector));
						ValueStr = V.ToString();
						break;
					}
					default:
						break;
					}
				}

				Line += FString::Printf(TEXT("[%s] "), *ValueStr);
			}

			UE_LOG(LogTemp, Warning, TEXT("%s"), *Line);
		}
	);

	UE_LOG(LogTemp, Warning, TEXT("==== END TABLE ===="));
	UE_LOG(LogTemp, Warning, TEXT(""));
}

void UBPDT_TableManager::PrintAllTables()
{
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("==== BPDT: ALL TABLES ===="));

	for (const auto& Pair : GetTables())
	{
		PrintTable(Pair.Key);
	}

	UE_LOG(LogTemp, Warning, TEXT("==== END ALL TABLES ===="));
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

bool UBPDT_TableManager::SaveTable(const FString& TableName)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BPDT] SaveTable failed. Table '%s' not found."), *TableName);
		return false;
	}

	if (!FBPDT_FileManager::WriteTable(TableName, *Table))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BPDT] SaveTable failed for table '%s'."), *TableName);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[BPDT] Table '%s' saved successfully."), *TableName);
	return true;
}

bool UBPDT_TableManager::SaveAllTables()
{
	TMap<FString, FBPDT_Table>& Tables = GetTables();

	if (Tables.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BPDT] SaveAllTables called but no tables exist."));
		return false;
	}

	bool bAllSucceeded = true;

	for (const auto& Pair : Tables)
	{
		const FString& TableName = Pair.Key;
		const FBPDT_Table& Table = Pair.Value;

		if (!FBPDT_FileManager::WriteTable(TableName, Table))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BPDT] Failed to save table '%s'."),
				*TableName
			);
			bAllSucceeded = false;
		}
		else
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BPDT] Saved table '%s'."),
				*TableName
			);
		}
	}

	return bAllSucceeded;
}

bool UBPDT_TableManager::LoadTable(const FString& TableName)
{
	if (!FBPDT_FileManager::IsTableInRegistry(TableName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BPDT] Table '%s' not found in registry."),
			*TableName
		);
		return false;
	}

	FBPDT_Table Loaded;
	if (!FBPDT_FileManager::ReadTable(TableName, Loaded))
	{
		return false;
	}

	GetTables().Add(TableName, MoveTemp(Loaded));
	return true;
}

bool UBPDT_TableManager::LoadAllTables()
{
	TArray<FString> Tables;
	if (!FBPDT_FileManager::ReadRegistry(Tables))
	{
		return false;
	}

	bool bSuccess = true;

	for (const FString& Name : Tables)
	{
		if (!LoadTable(Name))
		{
			bSuccess = false;
		}
	}

	return bSuccess;
}
