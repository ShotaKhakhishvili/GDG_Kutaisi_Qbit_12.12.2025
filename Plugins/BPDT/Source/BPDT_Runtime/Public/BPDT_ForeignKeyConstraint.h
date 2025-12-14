#pragma once

#include "CoreMinimal.h"
#include "BPDT_ForeignKeyConstraint.generated.h"

USTRUCT()
struct FBPDT_ForeignKeyConstraint
{
	GENERATED_BODY()

	UPROPERTY()
	FString FKTable;

	UPROPERTY()
	FName FKColumn;

	UPROPERTY()
	FString PKTable;

	UPROPERTY()
	FName PKColumn;
};
