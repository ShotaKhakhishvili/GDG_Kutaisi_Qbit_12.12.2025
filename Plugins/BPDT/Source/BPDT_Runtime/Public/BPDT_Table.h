#pragma once

#include "CoreMinimal.h"
#include "BPDT_Row.h"
#include "BPDT_Column.h"
#include "BPDT_PrimaryKey.h"
#include "BPDT_Table.generated.h"

UENUM()
enum class EBPDT_PrimaryKeyMode : uint8
{
	Serial,
	Explicit
};

USTRUCT()
struct FBPDT_Table
{
	GENERATED_BODY()

public:
	EBPDT_PrimaryKeyMode PKMode = EBPDT_PrimaryKeyMode::Serial;

	FName PKColumnName = NAME_None;

	int32 NextSerialID = 1;

	TArray<FBPDT_Column> Columns;

private:
	TMap<FBPDT_PrimaryKey, FBPDT_Row> Rows;

public:
	FBPDT_Table();

	/* Init */
	void InitSerial();

	/* Row ops */
	FBPDT_Row& InsertRowAsDefault();
	bool InsertRow(const FBPDT_Row& Row);

	const FBPDT_Row* FindRow(const FString& PKValue) const;
	const FBPDT_Cell* FindCellOnRow(const FString& PKValue, FName ColumnName) const;

	/* Schema ops */
	bool AddColumn(
		FName Name,
		EBPDT_CellType Type,
		const void* DefaultData,
		int32 DefaultSize
	);

	bool ConvertSerialToExplicit(
		FName NewPKColumnName,
		EBPDT_CellType Type,
		const void* DefaultData,
		int32 DefaultSize
	);

	bool ConvertExplicitToSerial();
	int32 GetRowCount() const;
	int32 GetColumnIndex(FName ColumnName) const;

	const TArray<FBPDT_Column>& GetColumns() const;
	const FBPDT_Column& GetColumn(int32 Index) const;

	void ForEachRow(TFunctionRef<void(const FBPDT_PrimaryKey&, const FBPDT_Row&)> Func) const;

private:
	FBPDT_PrimaryKey MakeSerialKey(int32 Value) const;
	FBPDT_PrimaryKey MakeExplicitKeyFromRow(const FBPDT_Row& Row) const;
	FBPDT_PrimaryKey ParsePKFromString(const FString& PKValue) const;

	int32 ResolveColumnIndex(FName ColumnName) const;

	int32 GetPKColumnIndex() const;
	EBPDT_CellType GetPKType() const;
	bool TryParsePKFromString(const FString& In, FBPDT_PrimaryKey& OutKey) const;
};
