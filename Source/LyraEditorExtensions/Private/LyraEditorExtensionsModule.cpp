// Copyright Epic Games, Inc. All Rights Reserved.

#include "ContentBrowser/LyraContentBrowserExtensions.h"
#include "LevelEditor/LyraLevelEditorExtensions.h"

/**
 * Implements the FLyraEditorExtensionsModule module.
 */
class FLyraEditorExtensionsModule : public IModuleInterface
{
public:
	FLyraEditorExtensionsModule();
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void OnPostEngineInit();
	void RegisterSettings();
	void UnregisterSettings();
};

FLyraEditorExtensionsModule::FLyraEditorExtensionsModule()
{
}

void FLyraEditorExtensionsModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FLyraEditorExtensionsModule::OnPostEngineInit);
}

void FLyraEditorExtensionsModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	if (UObjectInitialized())
	{
		UnregisterSettings();

		FLyraLevelEditorExtensions::RemoveHooks();
		FLyraContentBrowserExtensions::RemoveHooks();
	}
}

void FLyraEditorExtensionsModule::OnPostEngineInit()
{
	// Integrate actions into existing editor context menus
	if (!IsRunningCommandlet())
	{
		FLyraContentBrowserExtensions::InstallHooks();
		FLyraLevelEditorExtensions::InstallHooks();
	}

	RegisterSettings();
}

void FLyraEditorExtensionsModule::RegisterSettings()
{
	// @TODO: Register custom editor settings here
}

void FLyraEditorExtensionsModule::UnregisterSettings()
{
	// @TODO: Unregister cuatom editor settings here
}

IMPLEMENT_MODULE(FLyraEditorExtensionsModule, LyraEditorExtensions);
