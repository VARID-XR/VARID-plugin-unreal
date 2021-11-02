// This source code is provided "as is" without warranty of any kind, either express or implied. Use at your own risk.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

using UnrealBuildTool;
using System.IO;

public class VARID : ModuleRules
{
	public VARID(ReadOnlyTargetRules Target) : base(Target)
	{
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

		// Where you would place the dependency would depend on whether it was needed in the Public or Private folder
        // (most dependencies are Private since the Public folder typically contains only interfaces).
		// try to minimize public dependencies whenever we can.
        // One common way to eliminate indirect dependencies from modules is to use forward-declarations in their public header files and only use those types and functions internally inside private files.

		PublicDependencyModuleNames.AddRange(

			// Public includes/ modules will be re - exported from your module.
			// You are basically indicating that, in order to use your module, users will also require dependencies from these other headers and modules.
			// This generally happens if your module's public header files expose and/or use types or functions that are declared in another module.
			// This mechanism allows users of your module to add a dependency to your module without also explicitly having to add all the other indirect dependencies.
			// In effect, the header file and linker dependencies are passed through from your module to the user's module.

			new string[]
			{
				// ... add other public dependencies that you statically link with here ...
			}
			);
         
            
		PrivateDependencyModuleNames.AddRange(

			// Private includes/ modules indicate that these are internal dependencies of your module.
            // They will not be visible to the outside world.
            // This generally happens if your internal, private .cpp and.h files consume types or functions that are declared in another module.

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
