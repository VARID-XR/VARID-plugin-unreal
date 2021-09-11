// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "VARIDProfile.h"
#include "VARIDEyeTracking.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VARIDBlueprintFunctionLibrary.generated.h"

UCLASS(BlueprintType)
class UVARIDBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "VARID")
		static void SetProfileRootPath(const FString& ProfileRootPath);

	UFUNCTION(BlueprintCallable, Category = "VARID")
		static void SetProfileExtension(const FString& ProfileExtension);

	/** Obtain all files in a provided directory, with optional extension filter. All files are returned if Ext is left blank. Returns false if operation could not occur. */
	UFUNCTION(BlueprintCallable, Category = "VARID")
		static void ListProfiles(TArray<FString>& OutFiles);

	UFUNCTION(BlueprintCallable, category = "VARID")
		static bool LoadProfile(const FString& ProfileFullPath, FVARIDProfile& Profile);

	UFUNCTION(BlueprintCallable, category = "VARID")
		static FVARIDProfile& GetActiveProfile();

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void SetActiveProfile(const FVARIDProfile& Profile);

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void ListFX(TArray<FString>& OutFXDetails);

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void ToggleFX(const int32 ID);

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void EnableAllFX();

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void DisableAllFX();

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void BeginRendering();

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void EndRendering();

	UFUNCTION(BlueprintCallable, category = "VARID")
		static FVARIDEyeTracking& GetEyeTracking();

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void SetEyeTracking(const FVARIDEyeTracking& EyeTracking);

	UFUNCTION(BlueprintCallable, category = "VARID")
		static const FVector2D& GetDisplayFOV();

	UFUNCTION(BlueprintCallable, category = "VARID")
		static void SetDisplayFOV(const FVector2D& DisplayFOV);
};