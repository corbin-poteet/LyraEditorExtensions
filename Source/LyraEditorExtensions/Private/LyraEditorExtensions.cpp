// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraEditorExtensions.h"

#include "LyraContentBrowserExtensions.h"
#include "LyraEditorExtensionsLog.h"
#include "LyraLevelEditorExtensions.h"

DEFINE_LOG_CATEGORY(LogLyraEditorExtensions);

#define LOCTEXT_NAMESPACE "LyraEditorExtensions"

//////////////////////////////////////////////////////////////////////////
// FLyraEditorExtensions

class FLyraEditorExtensions : public ILyraEditorExtensionsModule
{
public:
	
	virtual void StartupModule() override
	{
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FLyraEditorExtensions::OnPostEngineInit);
	}


	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnPostEngineInit.RemoveAll(this);

		if (UObjectInitialized())
		{
			UnregisterSettings();

			FLyraLevelEditorExtensions::RemoveHooks();
			FLyraContentBrowserExtensions::RemoveHooks();
		}
	}

private:
	
	void OnPostEngineInit()
	{
		// Integrate actions into existing editor context menus
		if (!IsRunningCommandlet())
		{
			FLyraContentBrowserExtensions::InstallHooks();
			FLyraLevelEditorExtensions::InstallHooks();
		}

		RegisterSettings();
	}

	void RegisterSettings()
	{
		// @TODO: Register settings
		// see: FPaper2DEditor::RegisterSettings()
	}

	void UnregisterSettings()
	{
		// @TODO: Unregister settings
		// see: FPaper2DEditor::UnregisterSettings()
	}
};
	//////////////////////////////////////////////////////////////////////////

	IMPLEMENT_MODULE(FLyraEditorExtensions, LyraEditorExtensions);

	//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
