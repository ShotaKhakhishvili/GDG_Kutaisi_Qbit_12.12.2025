#pragma once

#include "CoreMinimal.h"
#include "BPDT_Types.generated.h"

UENUM(BlueprintType)
enum class EBPDT_CellType : uint8
{
	None,
	Int,
	Float,
	Bool,
	String,
	Vector3
};

USTRUCT(BlueprintType)
struct FBPDT_Cell
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBPDT_CellType Type = EBPDT_CellType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsNull = true;

	UPROPERTY()
	TArray<uint8> Data;

	FBPDT_Cell();

	FBPDT_Cell(EBPDT_CellType InType, const void* InData, int32 InSize);

	static FBPDT_Cell MakeNull(EBPDT_CellType InType);

	int32   AsInt() const;
	float   AsFloat() const;
	bool    AsBool() const;
	FString AsString() const;
	FVector AsVector3() const;
};
