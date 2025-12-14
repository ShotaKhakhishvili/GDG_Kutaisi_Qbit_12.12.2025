#include "BPDT_Column.h"

FBPDT_Column::FBPDT_Column(
	FName InName,
	EBPDT_CellType InType,
	const void* InDefaultData,
	int32 InByteSize,
	bool bInIsForeignKey,
	FName InReferencedTable
)
{
	check(InType != EBPDT_CellType::None);
	check(InByteSize > 0);
	check(InDefaultData);

	// FK sanity
	if (bInIsForeignKey)
	{
		check(InReferencedTable != NAME_None);
	}

	Name = InName;
	Type = InType;
	ByteSize = InByteSize;
	bIsForeignKey = bInIsForeignKey;
	ReferencedTableName = InReferencedTable;

	DefaultData.SetNumUninitialized(ByteSize);
	FMemory::Memcpy(DefaultData.GetData(), InDefaultData, ByteSize);
}
