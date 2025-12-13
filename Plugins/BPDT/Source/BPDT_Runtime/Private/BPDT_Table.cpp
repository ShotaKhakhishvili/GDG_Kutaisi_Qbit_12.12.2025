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

	// 1) Add the new PK column.
	if (!AddColumn(NewPKColumnName, Type, DefaultData, DefaultSize))
	{
		return false;
	}

	const int32 NewPKIndex = ResolveColumnIndex(NewPKColumnName);
	if (NewPKIndex == INDEX_NONE)
	{
		return false;
	}

	// 2) Write the old serial PK into that new column for all rows.
	for (auto& Pair : Rows)
	{
		const FBPDT_PrimaryKey& OldKey = Pair.Key;

		// OldKey is Int in serial mode; enforce here.
		if (OldKey.Type != EBPDT_CellType::Int || OldKey.Data.Num() != sizeof(int32))
		{
			return false;
		}

		Pair.Value.SetCell(
			NewPKIndex,
			FBPDT_Cell(EBPDT_CellType::Int, OldKey.Data.GetData(), OldKey.Data.Num())
		);
	}

	// 3) Rebuild map using explicit keys computed from rows.
	TMap<FBPDT_PrimaryKey, FBPDT_Row> NewRows;
	NewRows.Reserve(Rows.Num());

	// Temporarily set PK metadata so MakeExplicitKeyFromRow uses the right column.
	const EBPDT_PrimaryKeyMode OldMode = PKMode;
	const FName OldName = PKColumnName;

	PKMode = EBPDT_PrimaryKeyMode::Explicit;
	PKColumnName = NewPKColumnName;

	for (const auto& Pair : Rows)
	{
		const FBPDT_Row& Row = Pair.Value;

		FBPDT_PrimaryKey NewKey = MakeExplicitKeyFromRow(Row);

		// Uniqueness validation (crucial!)
		if (NewRows.Contains(NewKey))
		{
			// rollback metadata, leave table unchanged (except the added column; if you want full rollback,
			// you'd also remove the column, but you said you're okay not changing things now)
			PKMode = OldMode;
			PKColumnName = OldName;
			return false;
		}

		NewRows.Add(NewKey, Row);
	}

	Rows = MoveTemp(NewRows);
	return true;
}


bool FBPDT_Table::ConvertExplicitToSerial()
{
	if (PKMode != EBPDT_PrimaryKeyMode::Explicit)
	{
		return false;
	}

	const int32 PKIndex = ResolveColumnIndex(PKColumnName);
	if (PKIndex == INDEX_NONE)
	{
		return false;
	}

	// 1) Remove PK column from rows and schema.
	for (auto& Pair : Rows)
	{
		Pair.Value.RemoveCell(PKIndex);
	}
	Columns.RemoveAt(PKIndex);

	// 2) Ensure serial PK column exists at index 0.
	// If your design requires PK always at [0] and named "PK", restore it.
	// (You already do this in InitSerial normally, but here we’re converting in-place.)
	PKMode = EBPDT_PrimaryKeyMode::Serial;
	PKColumnName = FName(TEXT("PK"));

	// If column 0 isn't the serial PK column right now, you should enforce it.
	// Minimal version: assume column 0 is still the old serial PK column slot.
	// Better: explicitly set/replace column 0 as int PK.
	if (Columns.Num() == 0 || Columns[0].Type != EBPDT_CellType::Int)
	{
		Columns.Insert(
			FBPDT_Column(PKColumnName, EBPDT_CellType::Int, nullptr, sizeof(int32)),
			0
		);

		// Insert a placeholder PK cell into each row at index 0
		for (auto& Pair : Rows)
		{
			Pair.Value.InsertCell(0, FBPDT_Cell::MakeNull(EBPDT_CellType::Int));
		}
	}

	// 3) Rebuild map with new IDs and write them into cell[0].
	TMap<FBPDT_PrimaryKey, FBPDT_Row> NewRows;
	NewRows.Reserve(Rows.Num());

	int32 NewID = 1;

	for (auto& Pair : Rows)
	{
		FBPDT_Row Row = Pair.Value;

		Row.SetCell(
			0,
			FBPDT_Cell(EBPDT_CellType::Int, &NewID, sizeof(int32))
		);

		NewRows.Add(MakeSerialKey(NewID), MoveTemp(Row));
		++NewID;
	}

	Rows = MoveTemp(NewRows);
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

int32 FBPDT_Table::GetPKColumnIndex() const
{
	// In serial mode PK is always column 0 in your current design.
	// In explicit mode: PKColumnName tells where PK is.
	if (PKMode == EBPDT_PrimaryKeyMode::Serial)
	{
		return 0;
	}
	return ResolveColumnIndex(PKColumnName);
}

EBPDT_CellType FBPDT_Table::GetPKType() const
{
	const int32 PKIndex = GetPKColumnIndex();
	if (PKIndex == INDEX_NONE)
	{
		return EBPDT_CellType::None;
	}
	return Columns[PKIndex].Type;
}

static bool TryParseInt32Strict(const FString& S, int32& Out)
{
	// FCString::Atoi returns 0 on failure which is ambiguous.
	// This rejects "abc" and "12abc".
	if (S.IsEmpty()) return false;

	TCHAR* End = nullptr;
	const long V = FCString::Strtoi(*S, &End, 10);
	if (End == *S || *End != '\0') return false;

	Out = (int32)V;
	return true;
}

static bool TryParseFloatStrict(const FString& S, float& Out)
{
	if (S.IsEmpty()) return false;

	const double V = FCString::Atod(*S);

	Out = (float)V;
	return true;
}

static bool TryParseBoolStrict(const FString& S, bool& Out)
{
	if (S.Equals(TEXT("true"), ESearchCase::IgnoreCase)) { Out = true;  return true; }
	if (S.Equals(TEXT("false"), ESearchCase::IgnoreCase)) { Out = false; return true; }
	if (S == TEXT("1")) { Out = true;  return true; }
	if (S == TEXT("0")) { Out = false; return true; }
	return false;
}

bool FBPDT_Table::TryParsePKFromString(const FString& In, FBPDT_PrimaryKey& OutKey) const
{
	const int32 PKIndex = GetPKColumnIndex();
	if (PKIndex == INDEX_NONE)
	{
		return false;
	}

	const EBPDT_CellType PKType = Columns[PKIndex].Type;

	// Serial mode is always int PK in your current design.
	if (PKMode == EBPDT_PrimaryKeyMode::Serial)
	{
		int32 V = 0;
		if (!TryParseInt32Strict(In, V))
		{
			return false;
		}
		OutKey = MakeSerialKey(V);
		return true;
	}

	// Explicit PK: parse according to PK column type.
	switch (PKType)
	{
	case EBPDT_CellType::Int:
	{
		int32 V = 0;
		if (!TryParseInt32Strict(In, V))
		{
			return false;
		}
		OutKey = MakeSerialKey(V); // still correct for int PK: same binary layout
		return true;
	}

	case EBPDT_CellType::String:
	{
		// Canonical string encoding: UTF-8 bytes WITHOUT null terminator.
		FTCHARToUTF8 Conv(*In);
		const uint8* Ptr = (const uint8*)Conv.Get();
		const int32 Len = Conv.Length();

		if (Len <= 0)
		{
			return false; // reject empty PK unless you explicitly want it
		}

		FBPDT_Cell Cell(EBPDT_CellType::String, Ptr, Len);
		OutKey = FBPDT_PrimaryKey(Cell);
		return true;
	}

	case EBPDT_CellType::Float:
	{
		float V = 0.f;
		if (!TryParseFloatStrict(In, V))
		{
			return false;
		}
		FBPDT_Cell Cell(EBPDT_CellType::Float, &V, sizeof(float));
		OutKey = FBPDT_PrimaryKey(Cell);
		return true;
	}

	case EBPDT_CellType::Bool:
	{
		bool V = false;
		if (!TryParseBoolStrict(In, V))
		{
			return false;
		}
		FBPDT_Cell Cell(EBPDT_CellType::Bool, &V, sizeof(bool));
		OutKey = FBPDT_PrimaryKey(Cell);
		return true;
	}

	case EBPDT_CellType::Vector3:
	{
		// Accept "(X=1,Y=2,Z=3)" or "1 2 3" depending on what you want.
		// Simple strict: parse "X Y Z"
		TArray<FString> Parts;
		In.ParseIntoArrayWS(Parts);
		if (Parts.Num() != 3)
		{
			return false;
		}
		float X, Y, Z;
		if (!TryParseFloatStrict(Parts[0], X) ||
			!TryParseFloatStrict(Parts[1], Y) ||
			!TryParseFloatStrict(Parts[2], Z))
		{
			return false;
		}

		const FVector V(X, Y, Z);
		FBPDT_Cell Cell(EBPDT_CellType::Vector3, &V, sizeof(FVector));
		OutKey = FBPDT_PrimaryKey(Cell);
		return true;
	}

	default:
		return false;
	}
}

const FBPDT_Row* FBPDT_Table::FindRow(const FString& PKValue) const
{
	FBPDT_PrimaryKey Key;
	if (!TryParsePKFromString(PKValue, Key))
	{
		return nullptr;
	}
	return Rows.Find(Key);
}
