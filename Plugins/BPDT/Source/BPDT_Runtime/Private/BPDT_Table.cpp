#include "BPDT_Table.h"

FBPDT_Table::FBPDT_Table()
{
}

void FBPDT_Table::InitSerial(const TArray<FBPDT_Column>& InColumns)
{
	check(InColumns.Num() > 0);

	PKMode = EBPDT_PrimaryKeyMode::Serial;
	PKColumnName = NAME_None;
	Columns = InColumns;
	Rows.Empty();
	NextSerialID = 1;
}

FBPDT_Row& FBPDT_Table::InsertRowAsDefault()
{
	check(PKMode == EBPDT_PrimaryKeyMode::Serial);

	FBPDT_Row Row(Columns.Num());

	for (const FBPDT_Column& Col : Columns)
	{
		Row.AddCell(
			FBPDT_Cell(Col.Type, Col.DefaultData.GetData(), Col.ByteSize)
		);
	}

	const int32 NewID = NextSerialID++;
	FBPDT_PrimaryKey Key = MakeSerialKey(NewID);

	return Rows.Add(Key, Row);
}

bool FBPDT_Table::InsertRow(const FBPDT_Row& Row)
{
	check(Row.Num() == Columns.Num());

	FBPDT_PrimaryKey Key =
		(PKMode == EBPDT_PrimaryKeyMode::Serial)
		? MakeSerialKey(NextSerialID++)
		: MakeExplicitKeyFromRow(Row);

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

	AddColumn(NewPKColumnName, Type, DefaultData, DefaultSize);
	const int32 PKIndex = ResolveColumnIndex(NewPKColumnName);

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
