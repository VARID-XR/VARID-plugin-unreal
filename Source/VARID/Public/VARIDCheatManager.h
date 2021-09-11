// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "GameFramework/CheatManager.h"
#include "VARIDCheatManager.generated.h"

UCLASS()
class UVARIDCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:

	UFUNCTION(exec, Category = "VARID")
		void VARID_SetProfileRootPath(const FString& ProfileRootPath);

	UFUNCTION(exec, Category = "VARID")
		void VARID_SetProfileExtension(const FString& ProfileExtension);

	/** Obtain all files in a provided directory, with optional extension filter. All files are returned if Ext is left blank. Returns false if operation could not occur. */
	UFUNCTION(exec, Category = "VARID")
		void VARID_ListProfiles();

	UFUNCTION(exec, Category = "VARID")
		void VARID_SetActiveProfile(const int32 ID);

	UFUNCTION(exec, Category = "VARID")
		void VARID_ListFX();

	UFUNCTION(exec, Category = "VARID")
		void VARID_ToggleFX(const int32 ID);

	UFUNCTION(exec, Category = "VARID")
		void VARID_EnableAllFX();

	UFUNCTION(exec, Category = "VARID")
		void VARID_DisableAllFX();

	UFUNCTION(exec, Category = "VARID")
		void VARID_BeginRendering();

	UFUNCTION(exec, Category = "VARID")
		void VARID_EndRendering();

	UFUNCTION(exec, Category = "VARID")
		void VARID_GetDisplayFOV();

	UFUNCTION(exec, Category = "VARID")
		void VARID_SetDisplayFOV(const float HorizontalFOV, const float VerticalFOV);
};