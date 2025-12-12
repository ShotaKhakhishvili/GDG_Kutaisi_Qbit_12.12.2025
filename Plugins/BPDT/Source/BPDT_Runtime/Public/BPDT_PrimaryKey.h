#pragma once

#include "CoreMinimal.h"
#include "BPDT_Types.h"
#include "BPDT_PrimaryKey.generated.h"

USTRUCT()
struct FBPDT_PrimaryKey
{
	GENERATED_BODY()

public:
	EBPDT_CellType Type = EBPDT_CellType::None;
	TArray<uint8> Data;

public:
	FBPDT_PrimaryKey() = default;

	explicit FBPDT_PrimaryKey(const FBPDT_Cell& Cell)
	{
		check(!Cell.bIsNull);
		Type = Cell.Type;
		Data = Cell.Data;
	}

	bool operator==(const FBPDT_PrimaryKey& Other) const
	{
		return Type == Other.Type && Data == Other.Data;
	}
};

FORCEINLINE uint32 GetTypeHash(const FBPDT_PrimaryKey& Key)
{
	uint32 Hash = ::GetTypeHash(static_cast<uint8>(Key.Type));
	Hash = HashCombine(Hash, ::GetTypeHash(Key.Data.Num()));
	for (uint8 B : Key.Data)
	{
		Hash = HashCombine(Hash, B);
	}
	return Hash;
}
