#pragma once

#include "Modules/ModuleManager.h"

class FBPDT_RuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
