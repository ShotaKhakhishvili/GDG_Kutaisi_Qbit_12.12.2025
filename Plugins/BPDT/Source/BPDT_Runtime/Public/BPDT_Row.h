#pragma once

#include "CoreMinimal.h"
#include "BPDT_Types.h"
#include "BPDT_Row.generated.h"

USTRUCT()
struct FBPDT_Row
{
	GENERATED_BODY()

public:
	TArray<FBPDT_Cell> Cells;

public:
	FBPDT_Row() = default;

	explicit FBPDT_Row(int32 ColumnCount)
	{
		Cells.SetNum(ColumnCount);
	}

	int32 Num() const
	{
		return Cells.Num();
	}

	const FBPDT_Cell& GetCell(int32 Index) const
	{
		check(Cells.IsValidIndex(Index));
		return Cells[Index];
	}

	FBPDT_Cell& GetCellMutable(int32 Index)
	{
		check(Cells.IsValidIndex(Index));
		return Cells[Index];
	}

	void SetCell(int32 Index, const FBPDT_Cell& Cell)
	{
		check(Cells.IsValidIndex(Index));
		Cells[Index] = Cell;
	}

	void AddCell(const FBPDT_Cell& Cell)
	{
		Cells.Add(Cell);
	}

	void InsertCell(int32 Index, const FBPDT_Cell& Cell)
	{
		check(Index >= 0 && Index <= Cells.Num());
		Cells.Insert(Cell, Index);
	}

	void RemoveCell(int32 Index)
	{
		check(Cells.IsValidIndex(Index));
		Cells.RemoveAt(Index);
	}
};
