#include "BPDT_Table.h"
#include "BPDT_TableManager.h"


FBPDT_Table::FBPDT_Table()
{
}

void FBPDT_Table::InitSerial()
{
	PKMode = EBPDT_PrimaryKeyMode::Serial;
	PKColumnName = FName(TEXT("PK"));
	Rows.Empty();
	NextSerialID = 1;

	Columns.Empty();
	Columns.Emplace(
		PKColumnName,
		EBPDT_CellType::Int,
		&NextSerialID,      // default irrelevant for PK
		sizeof(int32)
	);
}

FBPDT_Row& FBPDT_Table::InsertRowAsDefault()
{
	FBPDT_Row Row(Columns.Num());

	FBPDT_PrimaryKey Key;

	if (PKMode == EBPDT_PrimaryKeyMode::Serial)
	{
		// Generate serial PK
		const int32 NewID = NextSerialID++;

		// Write PK into cell[0]
		Row.SetCell(
			0,
			FBPDT_Cell(EBPDT_CellType::Int, &NewID, sizeof(int32))
		);

		Key = MakeSerialKey(NewID);
	}
	else
	{
		// Explicit PK: default value goes into cell[0]
		const FBPDT_Column& PKCol = Columns[0];
		Row.SetCell(
			0,
			FBPDT_Cell(PKCol.Type, PKCol.DefaultData.GetData(), PKCol.ByteSize)
		);

		Key = MakeExplicitKeyFromRow(Row);
	}

	// Fill remaining cells with defaults
	for (int32 i = 1; i < Columns.Num(); ++i)
	{
		const FBPDT_Column& Col = Columns[i];
		Row.SetCell(
			i,
			FBPDT_Cell(Col.Type, Col.DefaultData.GetData(), Col.ByteSize)
		);
	}

	check(!Rows.Contains(Key));
	return Rows.Add(Key, Row);
}

bool FBPDT_Table::InsertRow(const FBPDT_Row& InRow)
{
	check(InRow.Num() == Columns.Num());

	FBPDT_Row Row = InRow; // make a mutable copy
	FBPDT_PrimaryKey Key;

	if (PKMode == EBPDT_PrimaryKeyMode::Serial)
	{
		const int32 NewID = NextSerialID++;

		// Override PK cell
		Row.SetCell(
			0,
			FBPDT_Cell(EBPDT_CellType::Int, &NewID, sizeof(int32))
		);

		Key = MakeSerialKey(NewID);
	}
	else
	{
		Key = MakeExplicitKeyFromRow(Row);
	}

	check(!Rows.Contains(Key));
	Rows.Add(Key, Row);
	return true;
}

const FBPDT_Row* FBPDT_Table::FindRow(const FString& PKValue) const
{
	FBPDT_PrimaryKey Key = ParsePKFromString(PKValue);
	return Rows.Find(Key);
}

const FBPDT_Cell* FBPDT_Table::FindCellOnRow(const FString& PKValue, FName ColumnName) const
{
	const FBPDT_Row* Row = FindRow(PKValue);
	if (!Row)
	{
		return nullptr;
	}

	const int32 Index = ResolveColumnIndex(ColumnName);
	if (Index == INDEX_NONE)
	{
		return nullptr;
	}

	return &Row->GetCell(Index);
}

/* ---------------- Schema ---------------- */

bool FBPDT_Table::AddColumn(
	FName Name,
	EBPDT_CellType Type,
	const void* DefaultData,
	int32 DefaultSize
)
{
	const FString NameStr = Name.ToString();

	if (!UBPDT_TableManager::IsValidBPDTIdentifier(NameStr))
	{
		return false;
	}

	if (ResolveColumnIndex(Name) != INDEX_NONE)
	{
		return false;
	}

	Columns.Emplace(Name, Type, DefaultData, DefaultSize);

	for (auto& Pair : Rows)
	{
		Pair.Value.AddCell(
			FBPDT_Cell(Type, DefaultData, DefaultSize)
		);
	}

	return true;
}


bool FBPDT_Table::ConvertSerialToExplicit(
	FName NewPKColumnName,
	EBPDT_CellType Type,
	const void* DefaultData,
	int32 DefaultSize
)
{
	if (PKMode != EBPDT_PrimaryKeyMode::Serial)
	{
		return false;
	}

	if (!AddColumn(NewPKColumnName, Type, DefaultData, DefaultSize))
	{
		return false;
	}

	const int32 PKIndex = ResolveColumnIndex(NewPKColumnName);
	check(PKIndex != INDEX_NONE);

	for (auto& Pair : Rows)
	{
		const FBPDT_PrimaryKey& Key = Pair.Key;

		Pair.Value.SetCell(
			PKIndex,
			FBPDT_Cell(Key.Type, Key.Data.GetData(), Key.Data.Num())
		);
	}

	PKMode = EBPDT_PrimaryKeyMode::Explicit;
	PKColumnName = NewPKColumnName;
	return true;
}

bool FBPDT_Table::ConvertExplicitToSerial()
{
	if (PKMode != EBPDT_PrimaryKeyMode::Explicit)
	{
		return false;
	}

	const int32 PKIndex = ResolveColumnIndex(PKColumnName);
	check(PKIndex != INDEX_NONE);

	for (auto& Pair : Rows)
	{
		Pair.Value.RemoveCell(PKIndex);
	}

	Columns.RemoveAt(PKIndex);

	TMap<FBPDT_PrimaryKey, FBPDT_Row> NewRows;
	int32 NewID = 1;

	for (auto& Pair : Rows)
	{
		NewRows.Add(MakeSerialKey(NewID++), Pair.Value);
	}

	Rows = MoveTemp(NewRows);
	PKMode = EBPDT_PrimaryKeyMode::Serial;
	PKColumnName = NAME_None;
	NextSerialID = NewID;

	return true;
}

/* ---------------- Internals ---------------- */

FBPDT_PrimaryKey FBPDT_Table::MakeSerialKey(int32 Value) const
{
	return FBPDT_PrimaryKey(
		FBPDT_Cell(EBPDT_CellType::Int, &Value, sizeof(int32))
	);
}

FBPDT_PrimaryKey FBPDT_Table::MakeExplicitKeyFromRow(const FBPDT_Row& Row) const
{
	const int32 Index = ResolveColumnIndex(PKColumnName);
	check(Index != INDEX_NONE);

	const FBPDT_Cell& Cell = Row.GetCell(Index);
	check(!Cell.bIsNull);

	return FBPDT_PrimaryKey(Cell);
}

FBPDT_PrimaryKey FBPDT_Table::ParsePKFromString(const FString& PKValue) const
{
	if (PKMode == EBPDT_PrimaryKeyMode::Serial)
	{
		const int32 V = FCString::Atoi(*PKValue);
		return MakeSerialKey(V);
	}

	const int32 Index = ResolveColumnIndex(PKColumnName);
	check(Index != INDEX_NONE);

	const FBPDT_Column& Col = Columns[Index];

	if (Col.Type == EBPDT_CellType::Int)
	{
		const int32 V = FCString::Atoi(*PKValue);
		return MakeSerialKey(V);
	}

	if (Col.Type == EBPDT_CellType::String)
	{
		FTCHARToUTF8 Conv(*PKValue);
		return FBPDT_PrimaryKey(
			FBPDT_Cell(EBPDT_CellType::String, Conv.Get(), Conv.Length() + 1)
		);
	}

	checkNoEntry();
	return FBPDT_PrimaryKey();
}

int32 FBPDT_Table::ResolveColumnIndex(FName ColumnName) const
{
	for (int32 i = 0; i < Columns.Num(); ++i)
	{
		if (Columns[i].Name == ColumnName)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 FBPDT_Table::GetRowCount() const
{
	return Rows.Num();
}

int32 FBPDT_Table::GetColumnIndex(FName ColumnName) const
{
	return ResolveColumnIndex(ColumnName);
}

const TArray<FBPDT_Column>& FBPDT_Table::GetColumns() const
{
	return Columns;
}

void FBPDT_Table::ForEachRow(
	TFunctionRef<void(const FBPDT_PrimaryKey&, const FBPDT_Row&)> Func
) const
{
	for (const auto& Pair : Rows)
	{
		Func(Pair.Key, Pair.Value);
	}
}

const FBPDT_Column& FBPDT_Table::GetColumn(int32 Index) const
{
	check(Columns.IsValidIndex(Index));
	return Columns[Index];
}
