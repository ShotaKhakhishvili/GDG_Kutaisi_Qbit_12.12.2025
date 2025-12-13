#include "BPDT_Types.h"

FBPDT_Cell::FBPDT_Cell()
{
}

FBPDT_Cell::FBPDT_Cell(EBPDT_CellType InType, const void* InData, int32 InSize)
{
	check(InType != EBPDT_CellType::None);
	check(InData);
	check(InSize > 0);

	Type = InType;
	bIsNull = false;

	Data.SetNumUninitialized(InSize);
	FMemory::Memcpy(Data.GetData(), InData, InSize);
}

FBPDT_Cell FBPDT_Cell::MakeNull(EBPDT_CellType InType)
{
	check(InType != EBPDT_CellType::None);

	FBPDT_Cell C;
	C.Type = InType;
	C.bIsNull = true;
	return C;
}

int32 FBPDT_Cell::AsInt() const
{
	check(Type == EBPDT_CellType::Int && !bIsNull);
	check(Data.Num() == sizeof(int32));

	int32 V;
	FMemory::Memcpy(&V, Data.GetData(), sizeof(int32));
	return V;
}

float FBPDT_Cell::AsFloat() const
{
	check(Type == EBPDT_CellType::Float && !bIsNull);
	check(Data.Num() == sizeof(float));

	float V;
	FMemory::Memcpy(&V, Data.GetData(), sizeof(float));
	return V;
}

bool FBPDT_Cell::AsBool() const
{
	check(Type == EBPDT_CellType::Bool && !bIsNull);
	check(Data.Num() == sizeof(bool));

	bool V;
	FMemory::Memcpy(&V, Data.GetData(), sizeof(bool));
	return V;
}

FString FBPDT_Cell::AsString() const
{
	check(Type == EBPDT_CellType::String && !bIsNull);

	if (Data.Num() == 0)
	{
		return FString();
	}

	const ANSICHAR* Raw =
		reinterpret_cast<const ANSICHAR*>(Data.GetData());

	return FString(
		UTF8_TO_TCHAR(Raw),
		Data.Num()
	);
}


FVector FBPDT_Cell::AsVector3() const
{
	check(Type == EBPDT_CellType::Vector3 && !bIsNull);
	check(Data.Num() == sizeof(FVector));

	FVector V;
	FMemory::Memcpy(&V, Data.GetData(), sizeof(FVector));
	return V;
}
