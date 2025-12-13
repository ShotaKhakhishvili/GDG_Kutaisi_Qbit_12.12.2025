#include "BPDT_RowBlueprintLibrary.h"

/* ==================== INT ==================== */

bool UBPDT_RowBlueprintLibrary::GetCellAsInt(
    const FBPDT_RowView& Row,
    FName ColumnName,
    int32& OutValue
)
{
    const int32* IndexPtr = Row.ColumnIndexMap.Find(ColumnName);
    if (!IndexPtr)
        return false;

    const int32 Index = *IndexPtr;
    if (!Row.Cells.IsValidIndex(Index))
        return false;

    const FBPDT_Cell& Cell = Row.Cells[Index];
    if (Cell.bIsNull || Cell.Type != EBPDT_CellType::Int)
        return false;

    OutValue = Cell.AsInt();
    return true;
}

/* ==================== FLOAT ==================== */

bool UBPDT_RowBlueprintLibrary::GetCellAsFloat(
    const FBPDT_RowView& Row,
    FName ColumnName,
    float& OutValue
)
{
    const int32* IndexPtr = Row.ColumnIndexMap.Find(ColumnName);
    if (!IndexPtr)
        return false;

    const int32 Index = *IndexPtr;
    if (!Row.Cells.IsValidIndex(Index))
        return false;

    const FBPDT_Cell& Cell = Row.Cells[Index];
    if (Cell.bIsNull || Cell.Type != EBPDT_CellType::Float)
        return false;

    OutValue = Cell.AsFloat();
    return true;
}

/* ==================== BOOL ==================== */

bool UBPDT_RowBlueprintLibrary::GetCellAsBool(
    const FBPDT_RowView& Row,
    FName ColumnName,
    bool& OutValue
)
{
    const int32* IndexPtr = Row.ColumnIndexMap.Find(ColumnName);
    if (!IndexPtr)
        return false;

    const int32 Index = *IndexPtr;
    if (!Row.Cells.IsValidIndex(Index))
        return false;

    const FBPDT_Cell& Cell = Row.Cells[Index];
    if (Cell.bIsNull || Cell.Type != EBPDT_CellType::Bool)
        return false;

    OutValue = Cell.AsBool();
    return true;
}

/* ==================== STRING ==================== */

bool UBPDT_RowBlueprintLibrary::GetCellAsString(
    const FBPDT_RowView& Row,
    FName ColumnName,
    FString& OutValue
)
{
    const int32* IndexPtr = Row.ColumnIndexMap.Find(ColumnName);
    if (!IndexPtr)
        return false;

    const int32 Index = *IndexPtr;
    if (!Row.Cells.IsValidIndex(Index))
        return false;

    const FBPDT_Cell& Cell = Row.Cells[Index];
    if (Cell.bIsNull || Cell.Type != EBPDT_CellType::String)
        return false;

    OutValue = Cell.AsString();
    return true;
}

/* ==================== VECTOR3 ==================== */

bool UBPDT_RowBlueprintLibrary::GetCellAsVector3(
    const FBPDT_RowView& Row,
    FName ColumnName,
    FVector& OutValue
)
{
    const int32* IndexPtr = Row.ColumnIndexMap.Find(ColumnName);
    if (!IndexPtr)
        return false;

    const int32 Index = *IndexPtr;
    if (!Row.Cells.IsValidIndex(Index))
        return false;

    const FBPDT_Cell& Cell = Row.Cells[Index];
    if (Cell.bIsNull || Cell.Type != EBPDT_CellType::Vector3)
        return false;

    OutValue = Cell.AsVector3();
    return true;
}
