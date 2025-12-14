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

	FString ToString() const
	{
		switch (Type)
		{
		case EBPDT_CellType::Int:
		{
			int32 V;
			FMemory::Memcpy(&V, Data.GetData(), sizeof(int32));
			return FString::FromInt(V);
		}

		case EBPDT_CellType::String:
		{
			return FString(
				UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(Data.GetData())),
				Data.Num()
			);
		}

		default:
			return TEXT("<PK>");
		}
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
