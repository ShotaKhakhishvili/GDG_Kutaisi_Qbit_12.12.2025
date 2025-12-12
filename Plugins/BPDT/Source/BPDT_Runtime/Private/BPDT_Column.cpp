#include "BPDT_Column.h"

FBPDT_Column::FBPDT_Column(
	FName InName,
	EBPDT_CellType InType,
	const void* InDefaultData,
	int32 InByteSize
)
{
	check(InType != EBPDT_CellType::None);
	check(InByteSize > 0);
	check(InDefaultData);

	Name = InName;
	Type = InType;
	ByteSize = InByteSize;

	DefaultData.SetNumUninitialized(ByteSize);
	FMemory::Memcpy(DefaultData.GetData(), InDefaultData, ByteSize);
}
