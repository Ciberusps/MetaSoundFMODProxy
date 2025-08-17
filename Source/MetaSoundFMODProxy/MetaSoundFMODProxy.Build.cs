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
		});

        PublicIncludePathModuleNames.AddRange(
            new string[] {
            }
        );
		
		PrivateDependencyModuleNames.AddRange(new string[] {
		});
	}
}
