#pragma once

#include "CoreMinimal.h"
#include "BPDT_Types.h"
#include "BPDT_Column.generated.h"

USTRUCT()
struct FBPDT_Column
{
	GENERATED_BODY()

public:
	FName Name;
	EBPDT_CellType Type = EBPDT_CellType::None;
	int32 ByteSize = 0;
	TArray<uint8> DefaultData;

	// ---------- FK METADATA ----------
	bool bIsForeignKey = false;

	// Referenced table name (PK is implicit)
	FName ReferencedTableName = NAME_None;

public:
	FBPDT_Column() = default;

	FBPDT_Column(
		FName InName,
		EBPDT_CellType InType,
		const void* InDefaultData,
		int32 InByteSize,
		bool bInIsForeignKey = false,
		FName InReferencedTable = NAME_None
	);
};

