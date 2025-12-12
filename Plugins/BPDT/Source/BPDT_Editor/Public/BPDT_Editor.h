#pragma once

#include "Modules/ModuleInterface.h"

class FBPDT_EditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void InitializeToolbarLater();
    bool TickInitializeToolbar(float DeltaTime);
};
