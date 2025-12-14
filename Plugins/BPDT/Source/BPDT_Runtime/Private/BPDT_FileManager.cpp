#include "BPDT_FileManager.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static constexpr uint32 BPDT_DATA_MAGIC = 0x42504454; // 'BPDT'
static constexpr uint32 BPDT_DATA_VERSION = 1;

/* ===================== PUBLIC ===================== */

bool FBPDT_FileManager::WriteTable(
	const FString& TableName,
	const FBPDT_Table& Table
)
{
	const FString Directory = GetSaveDirectory();

	IPlatformFile& PF =
		FPlatformFileManager::Get().GetPlatformFile();

	if (!PF.DirectoryExists(*Directory))
	{
		PF.CreateDirectoryTree(*Directory);
	}

	const FString SchemePath =
		Directory / (TableName + TEXT("_Scheme.txt"));

	const FString DataPath =
		Directory / (TableName + TEXT("_Data.bin"));

	if (!WriteSchemeFile(SchemePath, TableName, Table))
	{
		return false;
	}

	if (!WriteDataFile(DataPath, Table))
	{
		return false;
	}

	// ---- REGISTER TABLE (NEW) ----
	if (!AddTableToRegistry(TableName))
	{
		return false;
	}

	return true;
}

bool FBPDT_FileManager::ReadRegistry(TArray<FString>& OutTableNames)
{
	const FString Path = GetRegistryPath();

	if (!FPaths::FileExists(Path))
	{
		return true; // empty registry is valid
	}

	return FFileHelper::LoadFileToStringArray(OutTableNames, *Path);
}

bool FBPDT_FileManager::IsTableInRegistry(const FString& TableName)
{
	TArray<FString> Tables;
	if (!ReadRegistry(Tables))
	{
		return false;
	}

	return Tables.Contains(TableName);
}

/* ===================== INTERNAL ===================== */

FString FBPDT_FileManager::GetSaveDirectory()
{
	return FPaths::ProjectSavedDir() / TEXT("Plugins/BPDT");
}

FString FBPDT_FileManager::GetRegistryPath()
{
	return FBPDT_FileManager::GetSaveDirectory() /
		TEXT("__BPDT_TABLE_REGISTRY.txt");
}

bool FBPDT_FileManager::AddTableToRegistry(const FString& TableName)
{
	TArray<FString> Tables;
	ReadRegistry(Tables);

	if (Tables.Contains(TableName))
	{
		return true; // already registered
	}

	Tables.Add(TableName);

	const FString Path = GetRegistryPath();
	return FFileHelper::SaveStringArrayToFile(Tables, *Path);
}


/* ===================== SCHEME ===================== */

bool FBPDT_FileManager::WriteSchemeFile(
	const FString& Path,
	const FString& TableName,
	const FBPDT_Table& Table
)
{
	FString Out;

	const TArray<FBPDT_Column>& Columns = Table.GetColumns();
	const int32 ColumnCount = Columns.Num();
	const int32 RowCount = Table.GetRowCount();

	const int32 NullMaskBytes = (ColumnCount + 7) / 8;

	int32 DataBytesPerRow = 0;
	for (const FBPDT_Column& Col : Columns)
	{
		DataBytesPerRow += Col.ByteSize;
	}

	// ---- Header ----
	Out += TEXT("Table=") + TableName + TEXT("\n");
	Out += TEXT("RowCount=") + FString::FromInt(RowCount) + TEXT("\n");
	Out += TEXT("ColumnCount=") + FString::FromInt(ColumnCount) + TEXT("\n");
	Out += TEXT("NullMaskBytes=") + FString::FromInt(NullMaskBytes) + TEXT("\n");
	Out += TEXT("BytesPerRow=") +
		FString::FromInt(DataBytesPerRow + NullMaskBytes) + TEXT("\n\n");

	// ---- Columns ----
	Out += TEXT("Name,Size,Type\n");

	for (int32 i = 0; i < Columns.Num(); ++i)
	{
		const FBPDT_Column& Col = Columns[i];

		FString TypeStr;

		if (i == 0)
		{
			// PK is always first and serial
			TypeStr = TEXT("SERIAL");
		}
		else
		{
			TypeStr =
				StaticEnum<EBPDT_CellType>()
				->GetNameStringByValue((int64)Col.Type);
		}

		Out += FString::Printf(
			TEXT("%s,%d,%s\n"),
			*Col.Name.ToString(),
			Col.ByteSize,
			*TypeStr
		);
	}

	return FFileHelper::SaveStringToFile(Out, *Path);
}


/* ===================== DATA ===================== */

bool FBPDT_FileManager::WriteDataFile(
	const FString& Path,
	const FBPDT_Table& Table
)
{
	TUniquePtr<FArchive> Ar(
		IFileManager::Get().CreateFileWriter(*Path)
	);

	if (!Ar)
	{
		return false;
	}

	const TArray<FBPDT_Column>& Columns = Table.GetColumns();
	const int32 ColumnCount = Columns.Num();
	const int32 NullMaskBytes = (ColumnCount + 7) / 8;

	int32 DataBytesPerRow = 0;
	for (const FBPDT_Column& Col : Columns)
	{
		DataBytesPerRow += Col.ByteSize;
	}

	uint32 Magic = BPDT_DATA_MAGIC;
	uint32 Version = BPDT_DATA_VERSION;
	uint32 RowCount = (uint32)Table.GetRowCount();
	uint32 ColCount = (uint32)ColumnCount;
	uint32 BytesPerRow = (uint32)(DataBytesPerRow + NullMaskBytes);

	(*Ar) << Magic;
	(*Ar) << Version;
	(*Ar) << RowCount;
	(*Ar) << ColCount;
	(*Ar) << BytesPerRow;

	Table.ForEachRow(
		[&](const FBPDT_PrimaryKey&, const FBPDT_Row& Row)
		{
			WriteNullMask(*Ar, Row, ColumnCount);

			for (int32 i = 0; i < ColumnCount; ++i)
			{
				const FBPDT_Cell& Cell = Row.GetCell(i);
				const int32 ByteSize = Columns[i].ByteSize;

				if (!Cell.bIsNull && Cell.Data.Num() == ByteSize)
				{
					Ar->Serialize(
						(void*)Cell.Data.GetData(),
						ByteSize
					);
				}
				else
				{
					TArray<uint8> Zero;
					Zero.SetNumZeroed(ByteSize);
					Ar->Serialize(Zero.GetData(), ByteSize);
				}
			}
		}
	);

	Ar->Close();
	return true;
}

/* ===================== NULL MASK ===================== */

void FBPDT_FileManager::WriteNullMask(
	FArchive& Ar,
	const FBPDT_Row& Row,
	int32 ColumnCount
)
{
	const int32 Bytes = (ColumnCount + 7) / 8;
	TArray<uint8> Mask;
	Mask.SetNumZeroed(Bytes);

	for (int32 i = 0; i < ColumnCount; ++i)
	{
		if (Row.GetCell(i).bIsNull)
		{
			Mask[i / 8] |= (1 << (i % 8));
		}
	}

	Ar.Serialize(Mask.GetData(), Mask.Num());
}

bool FBPDT_FileManager::ReadTable(
	const FString& TableName,
	FBPDT_Table& OutTable
)
{
	const FString Directory = GetSaveDirectory();

	const FString SchemePath =
		Directory / (TableName + TEXT("_Scheme.txt"));

	const FString DataPath =
		Directory / (TableName + TEXT("_Data.bin"));

	if (!FPaths::FileExists(SchemePath) ||
		!FPaths::FileExists(DataPath))
	{
		return false;
	}

	/* ---------- READ SCHEME ---------- */

	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *SchemePath))
	{
		return false;
	}

	int32 RowCount = 0;
	int32 ColumnCount = 0;
	int32 NullMaskBytes = 0;

	TArray<FBPDT_Column> Columns;

	int32 LineIndex = 0;

	// Header
	for (; LineIndex < Lines.Num(); ++LineIndex)
	{
		const FString& Line = Lines[LineIndex];
		if (Line.IsEmpty())
		{
			LineIndex++;
			break;
		}

		FString Key, Value;
		if (!Line.Split(TEXT("="), &Key, &Value))
		{
			continue;
		}

		if (Key == TEXT("RowCount"))
		{
			RowCount = FCString::Atoi(*Value);
		}
		else if (Key == TEXT("ColumnCount"))
		{
			ColumnCount = FCString::Atoi(*Value);
		}
		else if (Key == TEXT("NullMaskBytes"))
		{
			NullMaskBytes = FCString::Atoi(*Value);
		}
	}

	// Skip CSV header
	if (LineIndex < Lines.Num() &&
		Lines[LineIndex].StartsWith(TEXT("Name")))
	{
		LineIndex++;
	}

	// Columns
	for (; LineIndex < Lines.Num(); ++LineIndex)
	{
		const FString& Line = Lines[LineIndex];
		if (Line.IsEmpty())
		{
			continue;
		}

		TArray<FString> Parts;
		Line.ParseIntoArray(Parts, TEXT(","), true);
		if (Parts.Num() != 3)
		{
			return false;
		}

		const FString& Name = Parts[0];
		const int32 Size = FCString::Atoi(*Parts[1]);
		const FString& TypeStr = Parts[2];

		EBPDT_CellType Type =
			(TypeStr == TEXT("SERIAL"))
			? EBPDT_CellType::Int
			: (EBPDT_CellType)StaticEnum<EBPDT_CellType>()
			->GetValueByNameString(TypeStr);

		TArray<uint8> DefaultData;
		DefaultData.SetNumZeroed(Size);

		Columns.Emplace(
			FName(*Name),
			Type,
			DefaultData.GetData(),
			Size
		);
	}

	if (Columns.Num() != ColumnCount)
	{
		return false;
	}

	/* ---------- INIT TABLE ---------- */

	OutTable = FBPDT_Table();
	OutTable.InitSerial();

	for (int32 i = 1; i < Columns.Num(); ++i)
	{
		const FBPDT_Column& Col = Columns[i];
		OutTable.AddColumn(
			Col.Name,
			Col.Type,
			Col.DefaultData.GetData(),
			Col.ByteSize
		);
	}

	/* ---------- READ DATA ---------- */

	TUniquePtr<FArchive> Ar(
		IFileManager::Get().CreateFileReader(*DataPath)
	);
	if (!Ar)
	{
		return false;
	}

	// Read binary header written by WriteDataFile
	uint32 Magic = 0, Version = 0, BinRowCount = 0, BinColCount = 0, BinBytesPerRow = 0;
	(*Ar) << Magic;
	(*Ar) << Version;
	(*Ar) << BinRowCount;
	(*Ar) << BinColCount;
	(*Ar) << BinBytesPerRow;

	if (Magic != BPDT_DATA_MAGIC || Version != BPDT_DATA_VERSION)
	{
		return false;
	}

	if ((int32)BinRowCount != RowCount || (int32)BinColCount != ColumnCount)
	{
		return false;
	}

	for (int32 RowIdx = 0; RowIdx < RowCount; ++RowIdx)
	{
		TArray<uint8> NullMask;
		NullMask.SetNumZeroed(NullMaskBytes);
		Ar->Serialize(NullMask.GetData(), NullMaskBytes);

		FBPDT_Row Row(ColumnCount);

		for (int32 ColIdx = 0; ColIdx < ColumnCount; ++ColIdx)
		{
			const bool bIsNull =
				(NullMask[ColIdx / 8] & (1 << (ColIdx % 8))) != 0;

			const FBPDT_Column& Col = Columns[ColIdx];

			if (bIsNull)
			{
				Row.SetCell(
					ColIdx,
					FBPDT_Cell::MakeNull(Col.Type)
				);
			}

			else
			{
				TArray<uint8> Data;
				Data.SetNumUninitialized(Col.ByteSize);
				Ar->Serialize(Data.GetData(), Col.ByteSize);

				Row.SetCell(
					ColIdx,
					FBPDT_Cell(Col.Type, Data.GetData(), Col.ByteSize)
				);
			}
		}

		OutTable.InsertRow(Row);
	}

	return true;

}