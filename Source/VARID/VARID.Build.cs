// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

using UnrealBuildTool;
using System.IO;

public class VARID : ModuleRules
{
	public VARID(ReadOnlyTargetRules Target) : base(Target)
	{
		//PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;    // relates to IWYU - include what you use
		////bEnforceIWYU = true;
		//PrivatePCHHeaderFile = "Private/VARID_PCH.h";		// still useful to have a PCH
		//MinFilesUsingPrecompiledHeaderOverride = 1;
		////bUseUnity = false;
		///bFasterWithoutUnity = true;	//'bFasterWithoutUnity has been deprecated in favor of setting 'bUseUnity'
		bUseUnity = false;

		PublicIncludePaths.AddRange(
			new string[] {
				
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"),
				Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private/PostProcess")
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"RenderCore", // Needed for AddShaderSourceDirectoryMapping & ResetAllShaderSourceDirectoryMappings
				"RHI",
				"Projects", // Needed for IPluginManager
				"Renderer",
				"CoreUObject",
				"Engine",
				// ... add other public dependencies that you statically link with here ...
			}
			);
         
            
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "RenderCore", // Needed for AddShaderSourceDirectoryMapping & ResetAllShaderSourceDirectoryMappings
				"RHI",
                "Projects", // Needed for IPluginManager
				"Renderer",
                "CoreUObject",
                "Engine",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
