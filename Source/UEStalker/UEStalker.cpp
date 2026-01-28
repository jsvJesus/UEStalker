// Fill out your copyright notice in the Description page of Project Settings.

#include "UEStalker.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Editor/StalkerAnimJitterFixer.h"
#endif

class FUEStalkerModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
#if WITH_EDITOR
		FStalkerAnimJitterFixer::Startup();
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		FStalkerAnimJitterFixer::Shutdown();
#endif
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, UEStalker, "UEStalker" );
