// This source code is provided "as is" without warranty of any kind, either express or implied. Use at your own risk.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "VARIDProfile.generated.h"

USTRUCT(BlueprintType)
struct FVARIDVFMapPoint
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		float RawX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		float RawY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		float RawValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		float Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		float Max;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		float NormX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		float NormY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		float NormValue;

public:
	FVARIDVFMapPoint();
	FVARIDVFMapPoint(float InRawX, float InRawY, float InRawValue, float InMin, float InMax, float InNormX, float InNormY, float InNormValue);
	FVARIDVFMapPoint(const FVARIDVFMapPoint& CopyMe);
};




USTRUCT(BlueprintType)
struct FVARIDVFMap
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		int32 ExpectedNumDataPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		bool FullField;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		TArray<FVARIDVFMapPoint> Data;

public:
	FVARIDVFMap();
	FVARIDVFMap(const FVARIDVFMap& CopyMe);
};




USTRUCT(BlueprintType)
struct FVARIDFX
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		int32 ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		bool Enabled;

public:
	FVARIDFX();
	FVARIDFX(const FVARIDFX& CopyMe);
};




USTRUCT(BlueprintType)
struct FVARIDFXBlur : public FVARIDFX
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDVFMap VFMap;

public:
	FVARIDFXBlur();
	FVARIDFXBlur(const FVARIDFXBlur& CopyMe);
};





USTRUCT(BlueprintType)
struct FVARIDFXContrast : public FVARIDFX
{
	GENERATED_BODY()

public:

	/** 0 = highest spatial freq, N = lowest spatial freq */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		TArray<FVARIDVFMap> VFMaps;

public:
	FVARIDFXContrast();
	FVARIDFXContrast(const FVARIDFXContrast& CopyMe);
};





USTRUCT(BlueprintType)
struct FVARIDFXInpaint : public FVARIDFX
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDVFMap VFMap;

public:
	FVARIDFXInpaint();
	FVARIDFXInpaint(const FVARIDFXInpaint& CopyMe);
};





USTRUCT(BlueprintType)
struct FVARIDFXWarp : public FVARIDFX
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDVFMap VFMap;

public:
	FVARIDFXWarp();
	FVARIDFXWarp(const FVARIDFXWarp& CopyMe);
};




USTRUCT(BlueprintType)
struct FVARIDEye
{
	GENERATED_BODY()

public:

	// explicitly state each FX as fields. Do not be tempted to generalise with a list/array/collection of FX. This is because different FX may need different resources. 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDFXBlur Blur;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDFXContrast Contrast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDFXInpaint Inpaint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDFXWarp Warp;

public:
	FVARIDEye();
	FVARIDEye(const FVARIDEye& CopyMe);
	void EnableAllFX();
	void DisableAllFX();
};




USTRUCT(BlueprintType)
struct FVARIDProfile
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FString Author;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FString Date;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDEye LeftEye;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVARIDEye RightEye;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		bool IsValid;

public:
	FVARIDProfile();
	FVARIDProfile(const FVARIDProfile& CopyMe);	//copy constructor useful for cloning the profile for the render thread
	void EnableAllFX();
	void DisableAllFX();
	TArray <FVARIDFX*> GetFX();
	FVARIDFX* GetFX(int32 ID);
	void ToggleFX(int32 ID);
};