// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "VARIDBlueprintFunctionLibrary.h"
#include "VARIDModule.h"
#include "VARIDProfile.h"
#include "VARIDEyeTracking.h"
#include "EngineMinimal.h"

// When working with blueprint functions, accessing logic using singletons is the simplest approach. 
// Unreal modules (including VARID module) are singleton objects.
// This is a good way to expose 3rd party library functions to Blueprints or to interact with C++ classes that don't have UObject support. 


void UVARIDBlueprintFunctionLibrary::SetProfileRootPath(const FString& ProfileRootPath)
{
	FVARIDModule::Get().SetProfileRootPath(ProfileRootPath);
}

void UVARIDBlueprintFunctionLibrary::SetProfileExtension(const FString& ProfileExtension)
{
	FVARIDModule::Get().SetProfileExtension(ProfileExtension);
}

void UVARIDBlueprintFunctionLibrary::ListProfiles(TArray<FString>& OutFiles)
{
	FVARIDModule::Get().ListProfiles(OutFiles);
}

bool UVARIDBlueprintFunctionLibrary::LoadProfile(const FString& ProfileFullPath, FVARIDProfile& OutProfile)
{
	return FVARIDModule::Get().LoadProfile(ProfileFullPath, OutProfile);
}

FVARIDProfile& UVARIDBlueprintFunctionLibrary::GetActiveProfile()
{
	return FVARIDModule::Get().GetActiveProfile();
}

void UVARIDBlueprintFunctionLibrary::SetActiveProfile(const FVARIDProfile& Profile)
{
	FVARIDModule::Get().SetActiveProfile(Profile);
}

void UVARIDBlueprintFunctionLibrary::ListFX(TArray<FString>& OutFXDetails)
{
	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	TArray<FVARIDFX*> FXArray = Profile.GetFX();
	OutFXDetails.Empty();
	OutFXDetails.SetNum(FXArray.Num());
	for (int32 i = 0; i < FXArray.Num(); i++)
	{
		FVARIDFX* FX = FXArray[i];
		FString Details = FString::Printf(TEXT("%d - %s"), FX->ID, *FX->Name);
		OutFXDetails.Add(Details);
	}
}

void UVARIDBlueprintFunctionLibrary::ToggleFX(const int32 ID)
{
	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	Profile.ToggleFX(ID);
}

void UVARIDBlueprintFunctionLibrary::EnableAllFX()
{
	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	Profile.EnableAllFX();
}

void UVARIDBlueprintFunctionLibrary::DisableAllFX()
{
	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	Profile.DisableAllFX();
}

void UVARIDBlueprintFunctionLibrary::BeginRendering()
{
	FVARIDModule::Get().BeginRendering();
}

void UVARIDBlueprintFunctionLibrary::EndRendering()
{
	FVARIDModule::Get().EndRendering();
}

FVARIDEyeTracking& UVARIDBlueprintFunctionLibrary::GetEyeTracking()
{
	return FVARIDModule::Get().GetEyeTracking();
}

void UVARIDBlueprintFunctionLibrary::SetEyeTracking(const FVARIDEyeTracking& EyeTracking)
{
	FVARIDModule::Get().SetEyeTracking(EyeTracking);
}

const FVector2D& UVARIDBlueprintFunctionLibrary::GetDisplayFOV()
{
	return FVARIDModule::Get().GetDisplayFOV();
}

void UVARIDBlueprintFunctionLibrary::SetDisplayFOV(const FVector2D& DisplayFOV)
{
	FVARIDModule::Get().SetDisplayFOV(DisplayFOV);
}