// Pavel Penkov 2025 All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class MetaSoundFMODProxy : ModuleRules
{
	public MetaSoundFMODProxy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"Projects",
			"MetasoundEngine",
			"MetasoundFrontend",
			"MetasoundGraphCore",
			"MetasoundStandardNodes",
			"AudioExtensions",
			"SignalProcessing",
			"FMODStudio",
			"AudioMixer",
			"AudioPlatformConfiguration",
		});

        PublicIncludePathModuleNames.AddRange(
            new string[] {
            }
        );
		
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd",
				"ToolMenus",
				"EditorStyle",
				"EditorWidgets",
				"AudioEditor",
			});
		}
	}
}
