#include "BPDT_TableManager.h"
#include "BPDT_FileManager.h"


static TMap<FString, FBPDT_Table> G_BPDT_Tables;
TArray<FBPDT_ForeignKeyConstraint> UBPDT_TableManager::ForeignKeys;

static bool ParseBool(const FString& Str, bool& OutValue)
{
	if (Str.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Str == TEXT("1"))
	{
		OutValue = true;
		return true;
	}
	if (Str.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Str == TEXT("0"))
	{
		OutValue = false;
		return true;
	}
	return false;
}

static bool ParseVector3(const FString& Str, FVector& OutValue)
{
	FString Clean = Str;
	Clean.ReplaceInline(TEXT(","), TEXT(" "));
	return OutValue.InitFromString(Clean);
}


bool UBPDT_TableManager::IsValidBPDTIdentifier(const FString& Name)
{
	if (Name.IsEmpty())
	{
		return false;
	}

	for (TCHAR C : Name)
	{
		const bool bValid =
			(C >= 'A' && C <= 'Z') ||
			(C >= 'a' && C <= 'z') ||
			(C >= '0' && C <= '9') ||
			(C == '_');

		if (!bValid)
		{
			return false;
		}
	}

	return true;
}

TMap<FString, FBPDT_Table>& UBPDT_TableManager::GetTables()
{
	return G_BPDT_Tables;
}

bool UBPDT_TableManager::CreateTable(const FString& TableName)
{
	if (!UBPDT_TableManager::IsValidBPDTIdentifier(TableName))
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
	return AddColumn_Typed<int32>(
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

bool UBPDT_TableManager::AddDefaultRow(
	const FString& TableName,
	int32& OutPrimaryKey
)
{
	OutPrimaryKey = -1;

	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	// This function ONLY applies to serial PK tables
	if (Table->PKMode != EBPDT_PrimaryKeyMode::Serial)
	{
		return false;
	}

	// Capture the PK that will be used
	const int32 NewPKValue = Table->NextSerialID;

	// Let the table handle row + PK creation
	Table->InsertRowAsDefault();

	OutPrimaryKey = NewPKValue;
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

bool UBPDT_TableManager::AddStringColumn(
	const FString& TableName,
	FName ColumnName,
	const FString& DefaultValue
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	// Convert FString ? UTF-8 bytes
	FTCHARToUTF8 Conv(*DefaultValue);

	const int32 ByteSize = Conv.Length(); // no null terminator
	if (ByteSize <= 0)
	{
		// Allow empty strings if you want; otherwise reject
		return Table->AddColumn(
			ColumnName,
			EBPDT_CellType::String,
			nullptr,
			0
		);
	}

	return Table->AddColumn(
		ColumnName,
		EBPDT_CellType::String,
		Conv.Get(),
		ByteSize
	);
}

bool UBPDT_TableManager::SetCellString(
	const FString& TableName,
	const FString& PKValue,
	FName ColumnName,
	const FString& Value
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const FBPDT_Row* RowConst = Table->FindRow(PKValue);
	if (!RowConst)
	{
		return false;
	}

	const int32 ColIndex = Table->GetColumnIndex(ColumnName);
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}

	const FBPDT_Column& Col = Table->GetColumn(ColIndex);
	if (Col.Type != EBPDT_CellType::String)
	{
		return false;
	}

	// Convert FString ? UTF-8
	FTCHARToUTF8 Conv(*Value);
	const int32 ByteSize = Conv.Length();

	FBPDT_Cell NewCell;

	if (ByteSize > 0)
	{
		NewCell = FBPDT_Cell(
			EBPDT_CellType::String,
			Conv.Get(),
			ByteSize
		);
	}
	else
	{
		// empty string = non-null but zero-length
		NewCell.Type = EBPDT_CellType::String;
		NewCell.bIsNull = false;
	}

	FBPDT_Row* Row = const_cast<FBPDT_Row*>(RowConst);
	Row->SetCell(ColIndex, MoveTemp(NewCell));

	return true;
}

void UBPDT_TableManager::GetAllTableNames(TArray<FString>& OutTableNames)
{
	OutTableNames.Reset();
	GetTables().GetKeys(OutTableNames);
	OutTableNames.Sort();
}

bool UBPDT_TableManager::GetColumnInfo(
	const FString& TableName,
	TArray<FName>& OutColumnNames,
	TArray<EBPDT_CellType>& OutColumnTypes
)
{
	OutColumnNames.Reset();
	OutColumnTypes.Reset();

	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const TArray<FBPDT_Column>& Columns = Table->GetColumns();

	for (const FBPDT_Column& Col : Columns)
	{
		OutColumnNames.Add(Col.Name);
		OutColumnTypes.Add(Col.Type);
	}

	return true;
}

bool UBPDT_TableManager::GetIntColumnData(
	const FString& TableName,
	FName ColumnName,
	TArray<int32>& OutValues
)
{
	OutValues.Reset();

	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
		return false;

	const bool bIsPK = (ColumnName == Table->PKColumnName);
	int32 ColIndex = INDEX_NONE;

	if (!bIsPK)
	{
		ColIndex = Table->GetColumnIndex(ColumnName);
		if (ColIndex == INDEX_NONE)
			return false;

		if (Table->GetColumn(ColIndex).Type != EBPDT_CellType::Int)
			return false;
	}

	bool bAllValid = true;

	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey& PK, const FBPDT_Row& Row)
		{
			int32 Value = 0;

			if (bIsPK)
			{
				if (!LexTryParseString(Value, *PK.ToString()))
					bAllValid = false;
			}
			else
			{
				const FBPDT_Cell& Cell = Row.GetCell(ColIndex);
				Value = Cell.bIsNull ? 0 : Cell.AsInt();
			}

			OutValues.Add(Value);
		}
	);

	return bAllValid;
}


bool UBPDT_TableManager::GetFloatColumnData(
	const FString& TableName,
	FName ColumnName,
	TArray<float>& OutValues
)
{
	OutValues.Reset();

	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
		return false;

	const bool bIsPK = (ColumnName == Table->PKColumnName);
	int32 ColIndex = INDEX_NONE;

	if (!bIsPK)
	{
		ColIndex = Table->GetColumnIndex(ColumnName);
		if (ColIndex == INDEX_NONE)
			return false;

		if (Table->GetColumn(ColIndex).Type != EBPDT_CellType::Float)
			return false;
	}

	bool bAllValid = true;

	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey& PK, const FBPDT_Row& Row)
		{
			float Value = 0.f;

			if (bIsPK)
			{
				if (!LexTryParseString(Value, *PK.ToString()))
					bAllValid = false;
			}
			else
			{
				const FBPDT_Cell& Cell = Row.GetCell(ColIndex);
				Value = Cell.bIsNull ? 0.f : Cell.AsFloat();
			}

			OutValues.Add(Value);
		}
	);

	return bAllValid;
}


bool UBPDT_TableManager::GetBoolColumnData(
	const FString& TableName,
	FName ColumnName,
	TArray<bool>& OutValues
)
{
	OutValues.Reset();

	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
		return false;

	const bool bIsPK = (ColumnName == Table->PKColumnName);
	int32 ColIndex = INDEX_NONE;

	if (!bIsPK)
	{
		ColIndex = Table->GetColumnIndex(ColumnName);
		if (ColIndex == INDEX_NONE)
			return false;

		if (Table->GetColumn(ColIndex).Type != EBPDT_CellType::Bool)
			return false;
	}

	bool bAllValid = true;

	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey& PK, const FBPDT_Row& Row)
		{
			bool Value = false;

			if (bIsPK)
			{
				if (!ParseBool(PK.ToString(), Value))
					bAllValid = false;
			}
			else
			{
				const FBPDT_Cell& Cell = Row.GetCell(ColIndex);
				Value = Cell.bIsNull ? false : Cell.AsBool();
			}

			OutValues.Add(Value);
		}
	);

	return bAllValid;
}


bool UBPDT_TableManager::GetStringColumnData(
	const FString& TableName,
	FName ColumnName,
	TArray<FString>& OutValues
)
{
	OutValues.Reset();

	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
		return false;

	const bool bIsPK = (ColumnName == Table->PKColumnName);
	int32 ColIndex = INDEX_NONE;

	if (!bIsPK)
	{
		ColIndex = Table->GetColumnIndex(ColumnName);
		if (ColIndex == INDEX_NONE)
			return false;

		if (Table->GetColumn(ColIndex).Type != EBPDT_CellType::String)
			return false;
	}

	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey& PK, const FBPDT_Row& Row)
		{
			if (bIsPK)
			{
				OutValues.Add(PK.ToString());
			}
			else
			{
				const FBPDT_Cell& Cell = Row.GetCell(ColIndex);
				OutValues.Add(Cell.bIsNull ? FString() : Cell.AsString());
			}
		}
	);

	return true;
}


bool UBPDT_TableManager::GetVector3ColumnData(
	const FString& TableName,
	FName ColumnName,
	TArray<FVector>& OutValues
)
{
	OutValues.Reset();

	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const int32 ColIndex = Table->GetColumnIndex(ColumnName);
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}

	if (Table->GetColumn(ColIndex).Type != EBPDT_CellType::Vector3)
	{
		return false;
	}

	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey&, const FBPDT_Row& Row)
		{
			const FBPDT_Cell& Cell = Row.GetCell(ColIndex);
			OutValues.Add(
				Cell.bIsNull ? FVector::ZeroVector : Cell.AsVector3()
			);
		}
	);

	return true;
}

bool UBPDT_TableManager::GetIntColumnDefault(
	const FString& TableName,
	FName ColumnName,
	int32& OutDefaultValue
)
{
	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const int32 ColIndex = Table->GetColumnIndex(ColumnName);
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}

	const FBPDT_Column& Col = Table->GetColumn(ColIndex);
	if (Col.Type != EBPDT_CellType::Int || Col.DefaultData.Num() != sizeof(int32))
	{
		return false;
	}

	FMemory::Memcpy(&OutDefaultValue, Col.DefaultData.GetData(), sizeof(int32));
	return true;
}

bool UBPDT_TableManager::GetFloatColumnDefault(
	const FString& TableName,
	FName ColumnName,
	float& OutDefaultValue
)
{
	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const int32 ColIndex = Table->GetColumnIndex(ColumnName);
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}

	const FBPDT_Column& Col = Table->GetColumn(ColIndex);
	if (Col.Type != EBPDT_CellType::Float || Col.DefaultData.Num() != sizeof(float))
	{
		return false;
	}

	FMemory::Memcpy(&OutDefaultValue, Col.DefaultData.GetData(), sizeof(float));
	return true;
}

bool UBPDT_TableManager::GetBoolColumnDefault(
	const FString& TableName,
	FName ColumnName,
	bool& OutDefaultValue
)
{
	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const int32 ColIndex = Table->GetColumnIndex(ColumnName);
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}

	const FBPDT_Column& Col = Table->GetColumn(ColIndex);
	if (Col.Type != EBPDT_CellType::Bool || Col.DefaultData.Num() != sizeof(bool))
	{
		return false;
	}

	FMemory::Memcpy(&OutDefaultValue, Col.DefaultData.GetData(), sizeof(bool));
	return true;
}

bool UBPDT_TableManager::GetVector3ColumnDefault(
	const FString& TableName,
	FName ColumnName,
	FVector& OutDefaultValue
)
{
	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const int32 ColIndex = Table->GetColumnIndex(ColumnName);
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}

	const FBPDT_Column& Col = Table->GetColumn(ColIndex);
	if (Col.Type != EBPDT_CellType::Vector3 || Col.DefaultData.Num() != sizeof(FVector))
	{
		return false;
	}

	FMemory::Memcpy(&OutDefaultValue, Col.DefaultData.GetData(), sizeof(FVector));
	return true;
}

bool UBPDT_TableManager::GetStringColumnDefaultAndMaxLength(
	const FString& TableName,
	FName ColumnName,
	FString& OutDefaultValue,
	int32& OutMaxLength
)
{
	OutDefaultValue.Reset();
	OutMaxLength = 0;

	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const int32 ColIndex = Table->GetColumnIndex(ColumnName);
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}

	const FBPDT_Column& Col = Table->GetColumn(ColIndex);
	if (Col.Type != EBPDT_CellType::String)
	{
		return false;
	}

	// ---- Default value (schema) ----
	if (Col.DefaultData.Num() > 0)
	{
		const ANSICHAR* Raw =
			reinterpret_cast<const ANSICHAR*>(Col.DefaultData.GetData());

		OutDefaultValue = FString(
			UTF8_TO_TCHAR(Raw),
			Col.DefaultData.Num()
		);

		OutMaxLength = OutDefaultValue.Len();
	}
	else
	{
		OutDefaultValue = FString();
		OutMaxLength = 0;
	}

	// ---- Max length from row data ----
	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey&, const FBPDT_Row& Row)
		{
			const FBPDT_Cell& Cell = Row.GetCell(ColIndex);
			if (!Cell.bIsNull)
			{
				const FString Value = Cell.AsString();
				OutMaxLength = FMath::Max(
					OutMaxLength,
					Value.Len()
				);
			}
		}
	);

	return true;
}

bool UBPDT_TableManager::GetPKName(
	const FString& TableName,
	FName& OutPKColumnName
)
{
	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	if (Table->PKMode == EBPDT_PrimaryKeyMode::Serial)
	{
		const TArray<FBPDT_Column>& Columns = Table->GetColumns();
		OutPKColumnName = Columns.Num() > 0 ? Columns[0].Name : NAME_None;
	}
	else
	{
		OutPKColumnName = Table->PKColumnName;
	}

	return true;
}

bool UBPDT_TableManager::GetPKInfo(
	const FString& TableName,
	FName& OutPKColumnName,
	EBPDT_CellType& OutPKType,
	bool& bIsSerial
)
{
	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	bIsSerial = (Table->PKMode == EBPDT_PrimaryKeyMode::Serial);

	if (bIsSerial)
	{
		const TArray<FBPDT_Column>& Columns = Table->GetColumns();
		if (Columns.Num() == 0)
		{
			return false;
		}

		OutPKColumnName = Columns[0].Name;
		OutPKType = Columns[0].Type; // will be Int
	}
	else
	{
		const int32 PKIndex = Table->GetColumnIndex(Table->PKColumnName);
		if (PKIndex == INDEX_NONE)
		{
			return false;
		}

		const FBPDT_Column& Col = Table->GetColumn(PKIndex);
		OutPKColumnName = Col.Name;
		OutPKType = Col.Type;
	}

	return true;
}

bool UBPDT_TableManager::GetTableRow(
	const FString& TableName,
	const FString& PKValue,
	FBPDT_RowView& OutRow
)
{
	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
		return false;

	const FBPDT_Row* Row = Table->FindRow(PKValue);
	if (!Row)
		return false;

	OutRow.Cells = Row->Cells;
	OutRow.ColumnIndexMap.Empty();

	const TArray<FBPDT_Column>& Columns = Table->GetColumns();
	for (int32 i = 0; i < Columns.Num(); ++i)
	{
		OutRow.ColumnIndexMap.Add(Columns[i].Name, i);
	}

	return true;
}

void UBPDT_TableManager::GetAllTableSchemas(
	TArray<FBPDT_TableSchemaView>& OutSchemas
)
{
	OutSchemas.Reset();

	const TMap<FString, FBPDT_Table>& Tables = GetTables();

	for (const auto& Pair : Tables)
	{
		const FString& TableName = Pair.Key;
		const FBPDT_Table& Table = Pair.Value;

		FBPDT_TableSchemaView View;
		View.TableName = TableName;

		const TArray<FBPDT_Column>& Columns = Table.GetColumns();
		View.ColumnNames.Reserve(Columns.Num());
		View.ColumnTypes.Reserve(Columns.Num());

		for (const FBPDT_Column& Col : Columns)
		{
			View.ColumnNames.Add(Col.Name);
			View.ColumnTypes.Add(Col.Type);
		}

		OutSchemas.Add(MoveTemp(View));
	}
}

bool UBPDT_TableManager::GetAllRowPKValues(
	const FString& TableName,
	TArray<FString>& OutPKValues
)
{
	OutPKValues.Reset();

	const FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	Table->ForEachRow(
		[&](const FBPDT_PrimaryKey& PK, const FBPDT_Row&)
		{
			OutPKValues.Add(PK.ToString());
		}
	);

	return true;
}

bool UBPDT_TableManager::ChangePrimaryKey(
	const FString& TableName,
	const FString& OldPKValue,
	const FString& NewPKValue
)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	const bool bChanged =
		Table->ChangePrimaryKey(OldPKValue, NewPKValue);

	if (!bChanged)
	{
		return false;
	}

	// ---- CASCADE ----
	CascadePrimaryKeyChange(
		TableName,
		OldPKValue,
		NewPKValue
	);

	return true;
}

void UBPDT_TableManager::CascadePrimaryKeyChange(
	const FString& ReferencedTableName,
	const FString& OldPKValue,
	const FString& NewPKValue
)
{
	int32 OldID = 0;
	int32 NewID = 0;

	if (!LexTryParseString(OldID, *OldPKValue))
		return;

	if (!LexTryParseString(NewID, *NewPKValue))
		return;

	struct FCascadeOp
	{
		FBPDT_Table* Table;
		FBPDT_PrimaryKey RowKey;
		int32 ColumnIndex;
	};

	TArray<FCascadeOp> Ops;
	const FName RefTableName(*ReferencedTableName);

	// ---------- PHASE 1: COLLECT ----------
	for (auto& TablePair : GetTables())
	{
		FBPDT_Table& FKTable = TablePair.Value;
		const TArray<FBPDT_Column>& Columns = FKTable.GetColumns();

		for (int32 ColIndex = 0; ColIndex < Columns.Num(); ++ColIndex)
		{
			const FBPDT_Column& Col = Columns[ColIndex];

			if (!Col.bIsForeignKey)
				continue;

			if (Col.ReferencedTableName != RefTableName)
				continue;

			// INT FK ONLY (for now)
			if (Col.Type != EBPDT_CellType::Int)
				continue;

			FKTable.ForEachRow(
				[&](const FBPDT_PrimaryKey& RowPK, const FBPDT_Row& Row)
				{
					const FBPDT_Cell& Cell = Row.GetCell(ColIndex);
					if (Cell.bIsNull)
						return;

					int32 FKValue = 0;
					FMemory::Memcpy(&FKValue, Cell.Data.GetData(), sizeof(int32));

					if (FKValue == OldID)
					{
						Ops.Add({ &FKTable, RowPK, ColIndex });
					}
				}
			);
		}
	}

	// ---------- PHASE 2: APPLY ----------
	for (const FCascadeOp& Op : Ops)
	{
		FBPDT_Row* Row = Op.Table->FindRowMutable(Op.RowKey);
		if (!Row)
			continue;

		Row->SetCell(
			Op.ColumnIndex,
			FBPDT_Cell(EBPDT_CellType::Int, &NewID, sizeof(int32))
		);
	}
}


bool UBPDT_TableManager::AddForeignKeyConstraint(
	const FString& FKTableName,
	FName FKColumnName,
	const FString& ReferencedTableName
)
{
	// ---- lookup tables ----
	FBPDT_Table* FKTable = GetTables().Find(FKTableName);
	if (!FKTable)
	{
		return false;
	}

	FBPDT_Table* ReferencedTable = GetTables().Find(ReferencedTableName);
	if (!ReferencedTable)
	{
		return false;
	}

	// ---- column must exist ----
	const int32 FKColIndex = FKTable->GetColumnIndex(FKColumnName);
	if (FKColIndex == INDEX_NONE)
	{
		return false;
	}

	FBPDT_Column& FKColumn =
		const_cast<FBPDT_Column&>(FKTable->GetColumn(FKColIndex));

	// ---- cannot FK the PK column ----
	if (FKColumnName == FKTable->GetPKColumnName())
	{
		return false;
	}

	// ---- already an FK? ----
	if (FKColumn.bIsForeignKey)
	{
		return false;
	}

	// ---- PK info of referenced table ----
	FName RefPKName;
	EBPDT_CellType RefPKType;
	bool bIsSerial;

	if (!GetPKInfo(
		ReferencedTableName,
		RefPKName,
		RefPKType,
		bIsSerial
	))
	{
		return false;
	}

	// ---- type compatibility check ----
	if (FKColumn.Type != RefPKType)
	{
		return false;
	}

	// ---- declare FK constraint ----
	FKColumn.bIsForeignKey = true;
	FKColumn.ReferencedTableName = FName(*ReferencedTableName);

	return true;
}

bool UBPDT_TableManager::IsValidFKConstraintOrdered(
	const FString& PKTableName,
	FName PKColumnName,
	const FString& FKTableName,
	FName FKColumnName
)
{
	// ---- find tables ----
	FBPDT_Table* PKTable = GetTables().Find(PKTableName);
	FBPDT_Table* FKTable = GetTables().Find(FKTableName);

	if (!PKTable || !FKTable)
	{
		return false;
	}

	// ---- PK column must match actual PK column ----
	if (PKTable->GetPKColumnName() != PKColumnName)
	{
		return false;
	}

	// ---- FK column must exist ----
	const int32 FKColIndex = FKTable->GetColumnIndex(FKColumnName);
	if (FKColIndex == INDEX_NONE)
	{
		return false;
	}

	const FBPDT_Column& FKColumn = FKTable->GetColumn(FKColIndex);

	// ---- must be marked as FK ----
	if (!FKColumn.bIsForeignKey)
	{
		return false;
	}

	// ---- must reference the PK table ----
	if (FKColumn.ReferencedTableName != FName(*PKTableName))
	{
		return false;
	}

	// ---- get referenced PK type via public API ----
	FName OutPKName = NAME_None;
	EBPDT_CellType OutPKType = EBPDT_CellType::None;
	bool bIsSerial = false;

	if (!UBPDT_TableManager::GetPKInfo(PKTableName, OutPKName, OutPKType, bIsSerial))
	{
		return false;
	}

	// Optional extra safety: ensure PK column name matches what GetPKInfo says
	if (OutPKName != PKColumnName)
	{
		return false;
	}

	// ---- type compatibility ----
	if (FKColumn.Type != OutPKType)
	{
		return false;
	}

	return true;
}

bool UBPDT_TableManager::AddExistingForeignKeyConstraint(
	const FString& FKTable,
	FName FKColumn,
	const FString& PKTable,
	FName PKColumn
)
{
	// Basic validation
	if (FKTable.IsEmpty() || PKTable.IsEmpty())
	{
		return false;
	}

	if (FKColumn.IsNone() || PKColumn.IsNone())
	{
		return false;
	}

	// Prevent duplicates
	for (const FBPDT_ForeignKeyConstraint& FK : ForeignKeys)
	{
		if (FK.FKTable == FKTable &&
			FK.FKColumn == FKColumn &&
			FK.PKTable == PKTable &&
			FK.PKColumn == PKColumn)
		{
			return false;
		}
	}

	FBPDT_ForeignKeyConstraint NewFK;
	NewFK.FKTable  = FKTable;
	NewFK.FKColumn = FKColumn;
	NewFK.PKTable  = PKTable;
	NewFK.PKColumn = PKColumn;

	ForeignKeys.Add(NewFK);
	return true;
}

bool UBPDT_TableManager::SaveForeignKeys()
{
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("BPDT");
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString FilePath = Dir / TEXT("ForeignKeys.txt");

	FString Output;

	for (const FBPDT_ForeignKeyConstraint& FK : ForeignKeys)
	{
		Output += FString::Printf(
			TEXT("%s|%s|%s|%s\n"),
			*FK.FKTable,
			*FK.FKColumn.ToString(),
			*FK.PKTable,
			*FK.PKColumn.ToString()
		);
	}

	return FFileHelper::SaveStringToFile(Output, *FilePath);
}

bool UBPDT_TableManager::LoadForeignKeys()
{
	const FString FilePath =
		FPaths::ProjectSavedDir() / TEXT("BPDT/ForeignKeys.txt");

	if (!FPaths::FileExists(FilePath))
	{
		return true; // No constraints is valid
	}

	FString Input;
	if (!FFileHelper::LoadFileToString(Input, *FilePath))
	{
		return false;
	}

	ForeignKeys.Empty();

	TArray<FString> Lines;
	Input.ParseIntoArrayLines(Lines);

	for (const FString& Line : Lines)
	{
		if (Line.IsEmpty())
		{
			continue;
		}

		TArray<FString> Parts;
		Line.ParseIntoArray(Parts, TEXT("|"), true);

		if (Parts.Num() != 4)
		{
			continue; // malformed line
		}

		FBPDT_ForeignKeyConstraint FK;
		FK.FKTable  = Parts[0];
		FK.FKColumn = FName(*Parts[1]);
		FK.PKTable  = Parts[2];
		FK.PKColumn = FName(*Parts[3]);

		ForeignKeys.Add(FK);
	}

	return true;
}

void UBPDT_TableManager::ApplyForeignKeysToTables()
{
	for (const FBPDT_ForeignKeyConstraint& FK : ForeignKeys)
	{
		FBPDT_Table* FKTable = GetTables().Find(FK.FKTable);
		if (!FKTable)
			continue;

		const int32 ColIndex = FKTable->GetColumnIndex(FK.FKColumn);
		if (ColIndex == INDEX_NONE)
			continue;

		FBPDT_Column& Col =
			const_cast<FBPDT_Column&>(FKTable->GetColumn(ColIndex));

		Col.bIsForeignKey = true;
		Col.ReferencedTableName = FName(*FK.PKTable);
	}
}
