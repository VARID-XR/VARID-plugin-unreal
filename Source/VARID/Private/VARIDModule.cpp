// This source code is provided "as is" without warranty of any kind, either express or implied. Use at your own risk.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "VARIDModule.h"
#include "VARIDProfile.h"
#include <json.hpp>
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "VARIDRendering.h"
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "EngineMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"

using json = nlohmann::json;
using nlohmann::json_pointer;

#define LOCTEXT_NAMESPACE "FVARIDModule"

static bool CheckFOV(FVector2D FOV)
{
	if (FOV.IsZero() || FOV.X < 0.0f || FOV.Y < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("VARID: DisplayFOV is zero"));
		return false;
	}

	return true;
}

void FVARIDModule::StartupModule()
{
	UE_LOG(LogTemp, Display, TEXT("VARID: FVARIDModule_StartupModule"));

	check(ENGINE_MINOR_VERSION >= 26);

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// map shader dir
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("VARID"))->GetBaseDir(), TEXT("Shaders"));
	UE_LOG(LogTemp, Display, TEXT("VARID: PluginShaderDir: %s"), *PluginShaderDir);
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/VARID"), PluginShaderDir);
}

void FVARIDModule::ShutdownModule()
{
	UE_LOG(LogTemp, Display, TEXT("VARID: FVARIDModule_ShutdownModule"));

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Cleanup the virtual source directory mapping.
	ResetAllShaderSourceDirectoryMappings();

	EndRendering();	// Module could be shutdown before we explicitly end rendering. Ensure cleanup.
}

void FVARIDModule::BeginRendering()
{
	if (!SceneViewExtension)
		SceneViewExtension = FSceneViewExtensions::NewExtension<FVARIDSceneViewExtension>();
}

void FVARIDModule::EndRendering()
{
	if (SceneViewExtension)
	{
		SceneViewExtension.Reset();
		SceneViewExtension = nullptr;
	}
}

void FVARIDModule::SetProfileRootPath(const FString ProfileRootPath)
{
	DefaultProfileRootPath = ProfileRootPath;
}

void FVARIDModule::SetProfileExtension(const FString ProfileExtension)
{
	DefaultProfileExtension = ProfileExtension;
}

FVARIDProfile& FVARIDModule::GetActiveProfile()
{
	return Profile;
}

void FVARIDModule::SetActiveProfile(const FVARIDProfile& InProfile)
{
	if (InProfile.IsValid)
	{
		Profile = FVARIDProfile(InProfile);
	}
}

bool FVARIDModule::ListProfiles(FString RootFolderFullPath, FString Ext, TArray<FString>& Files)
{
	if (RootFolderFullPath.IsEmpty())
	{
		UE_LOG(LogTemp, Display, TEXT("VARID: RootFolderFullPath is empty. Fall back to plugin content dir: %s"), *RootFolderFullPath);
		FString PluginContentFolderFullPath = IPluginManager::Get().FindPlugin(TEXT("VARID"))->GetContentDir();
		RootFolderFullPath = FPaths::Combine(PluginContentFolderFullPath, "Profiles");
	}

	if (!FPaths::DirectoryExists(RootFolderFullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: Directory does not exist: %s"), *RootFolderFullPath);
		return false;
	}

	FPaths::NormalizeDirectoryName(RootFolderFullPath);

	IFileManager& FileManager = IFileManager::Get();

	if (Ext == "")
	{
		Ext = "*.json";
	}
	else
	{
		Ext = (Ext.Left(1) == ".") ? "*" + Ext : "*." + Ext;
	}

	FString FinalPath = FPaths::Combine(RootFolderFullPath, Ext);
	FileManager.FindFiles(Files, *FinalPath, true, false);

	int32 NumProfilesWarningThreshold = 100;

	if (Files.Num() > NumProfilesWarningThreshold)
	{
		UE_LOG(LogTemp, Warning, TEXT("VARID: The directory '%s' contains a large number of profile files (> %d). This may impact performance when listing and loading VARID profiles. Consider using directories to organise the profiles"), *RootFolderFullPath, NumProfilesWarningThreshold);
	}

	for (int32 i = 0; i < Files.Num(); ++i)
	{
		Files[i] = FPaths::Combine(RootFolderFullPath, Files[i]);
	}

	return true;
}

bool FVARIDModule::ListProfiles(TArray<FString>& Files)
{
	return ListProfiles(DefaultProfileRootPath, DefaultProfileExtension, Files);
}

static bool CheckValue(float Value, float Min, float Max)
{
	if (Min == 0.0f && Max == 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: Min and Max are both zero."));
		return false;
	}
	else if (Max <= Min)
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: Max (%f) is less than or equal to Min (%f)."), Max, Min);
		return false;
	}
	else if (Value < Min)
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: Value below expected minumum: %f"), Value);
		return false;
	}
	else if (Value > Max)
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: Value above expected maximum: %f"), Value);
		return false;
	}
	else if (Min < 0 && abs(Min) != Max)
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: If Min (%f) is less than zero, its value must mirror Max (%f) .e.g -10 & 10, -50 & 50: %f"), Min, Max);
		return false;
	}

	return true;
}

static bool ParseVFMap(json& jsonObject, FString JsonPath, const FVector2D& DisplayFOV, FVARIDVFMap& OutVFMap)
{
	if (!CheckFOV(DisplayFOV))
	{
		return false;
	}
	
	std::string JsonPathStdString = std::string(TCHAR_TO_UTF8(*JsonPath));

	json::json_pointer rootJsonPtr(JsonPathStdString);
	if (!jsonObject.contains(rootJsonPtr))
	{
		UE_LOG(LogTemp, Warning, TEXT("VARID: VF Map Path does not exist: %s. ignore"), *JsonPath);
		return true;
	}

	json::json_pointer dataJsonPtr(JsonPathStdString + "/data");
	if (!jsonObject.contains(dataJsonPtr))
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: VF Map @ %s does not have field: 'data'"), *JsonPath);
		return false;
	}

	std::vector<float> InRawFloatArray = jsonObject.at(dataJsonPtr).get<std::vector<float>>();
	OutVFMap.Data.Empty();	// ensure

	if (InRawFloatArray.size() == 3)
	{
		OutVFMap.FullField = true;

		float RawValue = InRawFloatArray[0];
		float Min = InRawFloatArray[1];
		float Max = InRawFloatArray[2];

		if (!CheckValue(RawValue, Min, Max))
		{
			return false;
		}

		float NormValue = ((RawValue - Min) / (Max - Min));

		if (NormValue < 0.0f || NormValue > 1.0f)
		{
			UE_LOG(LogTemp, Error, TEXT("VARID: Normalised Value is out of expected 0...1 range: %f"), NormValue);
			return false;
		}

		// do One Minus to invert the value.
		// internally low numbers are good vision, high numbers are bad vision, which is the opposite of how they are defined in the profile. 
		NormValue = 1.0f - NormValue;

		// Correct for an origin offset
		if (Min < 0 && Max > 0)
		{
			NormValue -= 0.5f;
		}

		FVARIDVFMapPoint MapPoint(0.0f, 0.0f, RawValue, Min, Max, 0.0f, 0.0f, NormValue);
		OutVFMap.Data.Add(MapPoint);
	}
	else
	{
		OutVFMap.FullField = false;

		int32 DataStride = 5;

		json::json_pointer sizeJsonPtr(JsonPathStdString + "/expected_num_data_points");
		if (jsonObject.contains(sizeJsonPtr))
		{
			OutVFMap.ExpectedNumDataPoints = jsonObject.at(sizeJsonPtr).get<int32>();

			if (InRawFloatArray.size() != (OutVFMap.ExpectedNumDataPoints * DataStride))
			{
				UE_LOG(LogTemp, Error, TEXT("VARID: profile has unexpected number of map points. Expected %d VF Map point tuples each with %d parts"), OutVFMap.ExpectedNumDataPoints, DataStride);
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("VARID: VF Map @ %s does not have field: 'expected_num_data_points'. Will be implied by the data alone."), *JsonPath);
			OutVFMap.ExpectedNumDataPoints = InRawFloatArray.size() / DataStride;
		}

		if (InRawFloatArray.size() % DataStride > 0)
		{
			UE_LOG(LogTemp, Error, TEXT("VARID: profile has unexpected number of map points. Data should be specified in tuples each with %d parts"), DataStride);
			return false;
		}		

		for (int32 i = 0; i < InRawFloatArray.size(); i += DataStride)
		{
			float RawX = InRawFloatArray[i];
			float RawY = InRawFloatArray[i + 1];
			float RawValue = InRawFloatArray[i + 2];
			float Min = InRawFloatArray[i + 3];
			float Max = InRawFloatArray[i + 4];

			if (!CheckValue(RawValue, Min, Max))
			{
				return false;
			}

			float NormX = ((RawX / (DisplayFOV.X / 2.0f)) / 2.0f) + 0.5f;
			float NormY = ((RawY / (DisplayFOV.Y / 2.0f)) / 2.0f) + 0.5f;
			float NormValue = ((RawValue - Min) / (Max - Min));

			if (NormValue < 0.0f || NormValue > 1.0f)
			{
				UE_LOG(LogTemp, Error, TEXT("VARID: Normalised Value is out of expected 0...1 range: %f"), NormValue);
				return false;
			}

			// do One Minus to invert the value.
			// internally low numbers are good vision, high numbers are bad vision, which is the opposite of how they are defined in the profile. 
			NormValue = 1.0f - NormValue;

			// Correct for an origin offset
			if (Min < 0 && Max > 0)
			{
				NormValue -= 0.5f;
			}

			FVARIDVFMapPoint MapPoint(RawX, RawY, RawValue, Min, Max, NormX, NormY, NormValue);
			OutVFMap.Data.Add(MapPoint);
		}
	}

	return true;
}

bool FVARIDModule::LoadProfile(const FString& ProfileFullPath, FVARIDProfile& OutProfile)
{
	if (ProfileFullPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: ProfileFullPath is empty."));
		return false;
	}

	if (!FPaths::FileExists(ProfileFullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: File does not exist: %s"), *ProfileFullPath);
		return false;
	}

	const FVector2D& FOV = GetDisplayFOV();

	if (!CheckFOV(FOV))
	{
		return false;
	}

	/********************************************************************/
	// load raw data

	UE_LOG(LogTemp, Display, TEXT("VARID: ProfileFullPath: %s"), *ProfileFullPath);
	FString JsonString;
	FFileHelper::LoadFileToString(JsonString, *ProfileFullPath);
	UE_LOG(LogTemp, Display, TEXT("VARID: JsonString: %s"), *JsonString);
	json JsonObject = json::parse(TCHAR_TO_UTF8(*JsonString), nullptr, false, false);
	if (JsonObject.is_discarded())
	{
		UE_LOG(LogTemp, Error, TEXT("VARID: Could not parse the json profile."));
		return false;
	}

	/********************************************************************/
	// check mandatory fields

	{
		if (!JsonObject.contains("name"))
		{
			UE_LOG(LogTemp, Error, TEXT("VARID: Profile does not have 'name' field"));
			return false;
		}

		if (!JsonObject.contains("description"))
		{
			UE_LOG(LogTemp, Error, TEXT("VARID: Profile does not have 'description' field"));
			return false;
		}

		if (!JsonObject.contains("author"))
		{
			UE_LOG(LogTemp, Error, TEXT("VARID: Profile does not have 'author' field"));
			return false;
		}

		if (!JsonObject.contains("date"))
		{
			UE_LOG(LogTemp, Error, TEXT("VARID: Profile does not have 'date' field"));
			return false;
		}

		if (!JsonObject.contains("left_eye"))
		{
			UE_LOG(LogTemp, Error, TEXT("VARID: Profile does not have 'left_eye' field"));
			return false;
		}

		if (!JsonObject.contains("right_eye"))
		{
			UE_LOG(LogTemp, Error, TEXT("VARID: VFMap does not have 'right_eye' field"));
			return false;
		}
	}

	{
		std::string name = JsonObject.at("name").get<std::string>();
		std::string description = JsonObject.at("description").get<std::string>();
		std::string author = JsonObject.at("author").get<std::string>();
		std::string date = JsonObject.at("date").get<std::string>();

		OutProfile.Name = FString(name.c_str());
		OutProfile.Description = FString(description.c_str());
		OutProfile.Author = FString(author.c_str());
		OutProfile.Date = FString(date.c_str());
	}

	// NOTE: contrast map index is reversed internally compared with profile format. highest freq = 0 aligns better with mip level indexes

	{
		FVARIDEye& Eye = OutProfile.LeftEye;

		if (!ParseVFMap(JsonObject, "/left_eye/blur", FOV, Eye.Blur.VFMap)) return false;

		if (!ParseVFMap(JsonObject, "/left_eye/inpaint", FOV, Eye.Inpaint.VFMap)) return false;

		Eye.Contrast.VFMaps.Empty();
		Eye.Contrast.VFMaps.SetNum(10);
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_0_lowest_spatial_freq", FOV, Eye.Contrast.VFMaps[9])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_1", FOV, Eye.Contrast.VFMaps[8])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_2", FOV, Eye.Contrast.VFMaps[7])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_3", FOV, Eye.Contrast.VFMaps[6])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_4", FOV, Eye.Contrast.VFMaps[5])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_5", FOV, Eye.Contrast.VFMaps[4])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_6", FOV, Eye.Contrast.VFMaps[3])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_7", FOV, Eye.Contrast.VFMaps[2])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_8", FOV, Eye.Contrast.VFMaps[1])) return false;
		if (!ParseVFMap(JsonObject, "/left_eye/contrast/level_9_highest_spatial_freq", FOV, Eye.Contrast.VFMaps[0])) return false;

		if (!ParseVFMap(JsonObject, "/left_eye/warp", FOV, Eye.Warp.VFMap)) return false;
	}

	{
		FVARIDEye& Eye = OutProfile.RightEye;

		if (!ParseVFMap(JsonObject, "/right_eye/blur", FOV, Eye.Blur.VFMap)) return false;

		if (!ParseVFMap(JsonObject, "/right_eye/inpaint", FOV, Eye.Inpaint.VFMap)) return false;

		Eye.Contrast.VFMaps.Empty();
		Eye.Contrast.VFMaps.SetNum(10);
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_0_lowest_spatial_freq", FOV, Eye.Contrast.VFMaps[9])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_1", FOV, Eye.Contrast.VFMaps[8])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_2", FOV, Eye.Contrast.VFMaps[7])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_3", FOV, Eye.Contrast.VFMaps[6])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_4", FOV, Eye.Contrast.VFMaps[5])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_5", FOV, Eye.Contrast.VFMaps[4])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_6", FOV, Eye.Contrast.VFMaps[3])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_7", FOV, Eye.Contrast.VFMaps[2])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_8", FOV, Eye.Contrast.VFMaps[1])) return false;
		if (!ParseVFMap(JsonObject, "/right_eye/contrast/level_9_highest_spatial_freq", FOV, Eye.Contrast.VFMaps[0])) return false;

		if (!ParseVFMap(JsonObject, "/right_eye/warp", FOV, Eye.Warp.VFMap)) return false;
	}

	OutProfile.IsValid = true;

	UE_LOG(LogTemp, Display, TEXT("VARID: Profile is valid"));

	return true;
}

FVARIDEyeTracking& FVARIDModule::GetEyeTracking()
{
	return EyeTracking;
}

void FVARIDModule::SetEyeTracking(const FVARIDEyeTracking& InEyeTracking)
{
	EyeTracking = InEyeTracking;
}

const FVector2D& FVARIDModule::GetDisplayFOV()
{
	return DisplayFOV;
}

void FVARIDModule::SetDisplayFOV(const FVector2D& InDisplayFOV)
{
	DisplayFOV.X = InDisplayFOV.X;
	DisplayFOV.Y = InDisplayFOV.Y;
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVARIDModule, VARID)