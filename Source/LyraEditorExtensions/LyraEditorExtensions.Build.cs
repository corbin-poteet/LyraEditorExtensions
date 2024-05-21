// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LyraEditorExtensions : ModuleRules
{
	public LyraEditorExtensions(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"ApplicationCore",
				"AssetTools",
				"Slate",
				"SlateCore",
				"Engine",
				"InputCore",
				"ImageCore",
				"EditorFramework",
				"UnrealEd", // for AssetEditorSubsystem
				"KismetWidgets",
				"Kismet",  // for FWorkflowCentricApplication
				"PropertyEditor",
				"RenderCore",
				"ContentBrowser",
				"WorkspaceMenuStructure",	
				"MeshPaint",
				"EditorWidgets",
				"Projects",
				"NavigationSystem",
				"ToolMenus",
			});

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Json",
			});

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Settings",
				"LevelEditor"
			});
	}
}
