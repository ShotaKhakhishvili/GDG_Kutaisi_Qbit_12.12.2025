#pragma once

#include "Modules/ModuleManager.h"

class FBPDT_RuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

static bool IsValidBPDTIdentifier(const FString& Name)
{
	if (Name.IsEmpty())
	{
		return false;
	}

	for (TCHAR C : Name)
	{
		const bool bValid =
			(C >= 'A' && C <= 'Z') ||
			(C >= 'a' && C <= 'z') ||
			(C >= '0' && C <= '9') ||
			(C == '_');

		if (!bValid)
		{
			return false;
		}
	}

	return true;
}