// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "VARIDCheatManager.h"
#include "VARIDModule.h"
#include "GameFramework/CheatManager.h"
#include "GameFramework/PlayerController.h"

void UVARIDCheatManager::VARID_SetProfileRootPath(const FString& ProfileRootPath)
{
	FVARIDModule::Get().SetProfileRootPath(ProfileRootPath);
}

void UVARIDCheatManager::VARID_SetProfileExtension(const FString& ProfileExtension)
{
	FVARIDModule::Get().SetProfileExtension(ProfileExtension);
}

void UVARIDCheatManager::VARID_ListProfiles()
{
	TArray<FString> Files;
	FVARIDModule::Get().ListProfiles(Files);

	for (int32 i = 0; i < Files.Num(); ++i)
	{
		FString FilePath = Files[i];
		FString OutputString = FString::Printf(TEXT("%d - %s"), i, *FilePath);
		UE_LOG(LogTemp, Display, TEXT("%s"), *OutputString);
		GetOuterAPlayerController()->ClientMessage(OutputString);
	}
}

void UVARIDCheatManager::VARID_SetActiveProfile(const int32 ID)
{
	TArray<FString> Files;
	FVARIDModule::Get().ListProfiles(Files);

	// HACK: we are relying on the file system not changing between listing the profiles and loading a profile
	
	if (Files.IsValidIndex(ID))
	{
		FVARIDProfile Profile;
		if (FVARIDModule::Get().LoadProfile(Files[ID], Profile))
		{
			FVARIDModule::Get().SetActiveProfile(Profile);
		}
		else
		{
			FString Message = "Failed to load profile. Check log for details";
			UE_LOG(LogTemp, Error, TEXT("%s"), *Message);
			GetOuterAPlayerController()->ClientMessage(Message);
		}
	}
	else
	{
		FString Message = "Invalid Profile ID";
		UE_LOG(LogTemp, Error, TEXT("%s"), *Message);
		GetOuterAPlayerController()->ClientMessage(Message);
	}
}

void UVARIDCheatManager::VARID_ListFX()
{
	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	TArray<FVARIDFX*> FXArray = Profile.GetFX();

	for (int32 i = 0; i < FXArray.Num(); i++)
	{
		FVARIDFX* FX = FXArray[i];
		FString OutputString = FString::Printf(TEXT("%d - %s"), FX->ID, *FX->Name);
		UE_LOG(LogTemp, Display, TEXT("%s"), *OutputString);
		GetOuterAPlayerController()->ClientMessage(OutputString);
	}
}

void UVARIDCheatManager::VARID_ToggleFX(const int32 ID)
{
	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	Profile.ToggleFX(ID);
}

void UVARIDCheatManager::VARID_EnableAllFX()
{
	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	Profile.EnableAllFX();
}

void UVARIDCheatManager::VARID_DisableAllFX()
{
	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	Profile.DisableAllFX();
}

void UVARIDCheatManager::VARID_BeginRendering()
{
	FVARIDModule::Get().BeginRendering();
}

void UVARIDCheatManager::VARID_EndRendering()
{
	FVARIDModule::Get().EndRendering();
}

void UVARIDCheatManager::VARID_GetDisplayFOV()
{
	const FVector2D& DisplayFOV = FVARIDModule::Get().GetDisplayFOV();
	FString Message = FString::Printf(TEXT("Display FOV: Horizontal=%f, Vertical=%f"), DisplayFOV.X, DisplayFOV.Y);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Message);
	GetOuterAPlayerController()->ClientMessage(Message);
}

void UVARIDCheatManager::VARID_SetDisplayFOV(const float HorizontalFOV, const float VerticalFOV)
{
	FVARIDModule::Get().SetDisplayFOV(FVector2D(HorizontalFOV, VerticalFOV));
}