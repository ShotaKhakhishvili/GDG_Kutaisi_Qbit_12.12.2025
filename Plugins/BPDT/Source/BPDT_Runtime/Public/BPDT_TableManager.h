#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPDT_Table.h"
#include "BPDT_TableManager.generated.h"

UCLASS()
class BPDT_RUNTIME_API UBPDT_TableManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool IsValidBPDTIdentifier(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Table")
	static bool CreateTable(const FString& TableName);

	UFUNCTION(BlueprintCallable, Category="BPDT|Table")
	static bool RemoveTable(const FString& TableName);

	UFUNCTION(BlueprintCallable, Category="BPDT|Table")
	static bool ConvertTableToExplicitPK(
		const FString& TableName,
		FName NewPKColumnName,
		EBPDT_CellType Type,
		const TArray<uint8>& DefaultData
	);

	UFUNCTION(BlueprintCallable, Category="BPDT|Debug")
	static void PrintTable(const FString& TableName);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Debug")
	void PrintAllTables();

	UFUNCTION(BlueprintCallable, Category = "BPDT|Row")
	static bool AddDefaultRow(const FString& TableName);
	UFUNCTION(BlueprintCallable, Category = "BPDT|Save")
	static bool SaveTable(const FString& TableName);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Save")
	static bool SaveAllTables();

	UFUNCTION(BlueprintCallable, Category = "BPDT|Load")
	static bool LoadTable(const FString& TableName);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Load")
	static bool LoadAllTables();

	UFUNCTION(BlueprintCallable, Category = "BPDT|Table")
	static bool AddIntColumn(
		const FString& TableName,
		FName ColumnName,
		int32 DefaultValue
	);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Table")
	static bool AddFloatColumn(
		const FString& TableName,
		FName ColumnName,
		float DefaultValue
	);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Table")
	static bool AddBoolColumn(
		const FString& TableName,
		FName ColumnName,
		bool DefaultValue
	);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Table")
	static bool AddVector3Column(
		const FString& TableName,
		FName ColumnName,
		FVector DefaultValue
	);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Cell")
	static bool SetCellInt(
		const FString& TableName,
		const FString& PKValue,
		FName ColumnName,
		int32 Value
	);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Cell")
	static bool SetCellFloat(
		const FString& TableName,
		const FString& PKValue,
		FName ColumnName,
		float Value
	);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Cell")
	static bool SetCellBool(
		const FString& TableName,
		const FString& PKValue,
		FName ColumnName,
		bool Value
	);

	UFUNCTION(BlueprintCallable, Category = "BPDT|Cell")
	static bool SetCellVector3(
		const FString& TableName,
		const FString& PKValue,
		FName ColumnName,
		FVector Value
	);

private:
	static TMap<FString, FBPDT_Table>& GetTables();

	template<typename T>
	static bool AddColumn_Typed(
		const FString& TableName,
		FName ColumnName,
		EBPDT_CellType ExpectedType,
		const T& DefaultValue
	);

	template<typename T>
	static bool SetCellTyped(
		FBPDT_Table* Table,
		const FString& PKValue,
		FName ColumnName,
		EBPDT_CellType ExpectedType,
		const T& Value
	);

};



template<typename T>
inline bool UBPDT_TableManager::AddColumn_Typed(const FString& TableName, FName ColumnName, EBPDT_CellType ExpectedType, const T& DefaultValue)
{
	FBPDT_Table* Table = GetTables().Find(TableName);
	if (!Table)
	{
		return false;
	}

	if (ExpectedType == EBPDT_CellType::None)
	{
		return false;
	}

	const int32 ByteSize = sizeof(T);

	// Validate size against cell type
	switch (ExpectedType)
	{
	case EBPDT_CellType::Int:
		check(ByteSize == sizeof(int32));
		break;

	case EBPDT_CellType::Float:
		check(ByteSize == sizeof(float));
		break;

	case EBPDT_CellType::Vector3:
		check(ByteSize == sizeof(FVector));
		break;

	case EBPDT_CellType::Bool:
		check(ByteSize == sizeof(bool));
		break;

	default:
		return false;
	}

	return Table->AddColumn(
		ColumnName,
		ExpectedType,
		&DefaultValue,
		ByteSize
	);
}

template<typename T>
bool UBPDT_TableManager::SetCellTyped(
	FBPDT_Table* Table,
	const FString& PKValue,
	FName ColumnName,
	EBPDT_CellType ExpectedType,
	const T& Value
)
{
	const FBPDT_Row* RowConst = Table->FindRow(PKValue);
	if (!RowConst)
	{
		return false;
	}

	FBPDT_Row* Row = const_cast<FBPDT_Row*>(RowConst);

	const int32 ColIndex = Table->GetColumnIndex(ColumnName);
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}

	const FBPDT_Column& Col = Table->GetColumn(ColIndex);
	if (Col.Type != ExpectedType)
	{
		return false;
	}

	Row->SetCell(
		ColIndex,
		FBPDT_Cell(ExpectedType, &Value, sizeof(T))
	);

	return true;
}