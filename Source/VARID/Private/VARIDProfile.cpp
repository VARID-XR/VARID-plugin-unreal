// This source code is provided "as is" without warranty of any kind, either express or implied. Use at your own risk.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "VARIDProfile.h"

FVARIDVFMapPoint::FVARIDVFMapPoint()
{
	RawX = 0.0f;
	RawY = 0.0f;
	RawValue = 0.0f;
	Min = 0.0f;
	Max = 1.0f;
	NormX = 0.0f;
	NormY = 0.0f;
	NormValue = 0.0f;
}

FVARIDVFMapPoint::FVARIDVFMapPoint(float InRawX, float InRawY, float InRawValue, float InMin, float InMax, float InNormX, float InNormY, float InNormValue)
{
	RawX = InRawX;
	RawY = InRawY;
	RawValue = InRawValue;
	Min = InMin;
	Max = InMax;
	NormX = InNormX;
	NormY = InNormY;
	NormValue = InNormValue;
}

FVARIDVFMapPoint::FVARIDVFMapPoint(const FVARIDVFMapPoint& CopyMe)
{
	RawX = CopyMe.RawX;
	RawY = CopyMe.RawY;
	RawValue = CopyMe.RawValue;
	Min = CopyMe.Min;
	Max = CopyMe.Max;
	NormX = CopyMe.NormX;
	NormY = CopyMe.NormY;
	NormValue = CopyMe.NormValue;
}

FVARIDVFMap::FVARIDVFMap()
{
	ExpectedNumDataPoints = 0;
	FullField = false;
}

FVARIDVFMap::FVARIDVFMap(const FVARIDVFMap& CopyMe)
{
	ExpectedNumDataPoints = CopyMe.ExpectedNumDataPoints;
	FullField = CopyMe.FullField;

	for (int32 i = 0; i < CopyMe.Data.Num(); i++)
	{
		Data.Add(FVARIDVFMapPoint(CopyMe.Data[i]));
	}
}

FVARIDFX::FVARIDFX()
{
	ID = -1;
	Name = "UNKNOWN";
	Enabled = true;
}

FVARIDFX::FVARIDFX(const FVARIDFX& CopyMe)
{
	ID = CopyMe.ID;
	Name = FString(CopyMe.Name);
	Enabled = CopyMe.Enabled;
}

FVARIDFXBlur::FVARIDFXBlur() : FVARIDFX()
{

}

FVARIDFXBlur::FVARIDFXBlur(const FVARIDFXBlur& CopyMe) : FVARIDFX(CopyMe)
{
	VFMap = FVARIDVFMap(CopyMe.VFMap);
}

FVARIDFXContrast::FVARIDFXContrast() : FVARIDFX()
{

}

FVARIDFXContrast::FVARIDFXContrast(const FVARIDFXContrast& CopyMe) : FVARIDFX(CopyMe)
{
	for (int32 i = 0; i < CopyMe.VFMaps.Num(); i++)
	{
		VFMaps.Add(FVARIDVFMap(CopyMe.VFMaps[i]));
	}
}

FVARIDFXInpaint::FVARIDFXInpaint() : FVARIDFX()
{

}

FVARIDFXInpaint::FVARIDFXInpaint(const FVARIDFXInpaint& CopyMe) : FVARIDFX(CopyMe)
{
	VFMap = FVARIDVFMap(CopyMe.VFMap);
}

FVARIDFXWarp::FVARIDFXWarp() : FVARIDFX()
{

}

FVARIDFXWarp::FVARIDFXWarp(const FVARIDFXWarp& CopyMe) : FVARIDFX(CopyMe)
{
	VFMap = FVARIDVFMap(CopyMe.VFMap);
}

FVARIDEye::FVARIDEye()
{

}

FVARIDEye::FVARIDEye(const FVARIDEye& CopyMe)
{
	Blur = FVARIDFXBlur(CopyMe.Blur);
	Contrast = FVARIDFXContrast(CopyMe.Contrast);
	Inpaint = FVARIDFXInpaint(CopyMe.Inpaint);
	Warp = FVARIDFXWarp(CopyMe.Warp);
}

void FVARIDEye::EnableAllFX()
{
	Blur.Enabled = true;
	Contrast.Enabled = true;
	Inpaint.Enabled = true;
	Warp.Enabled = true;
}

void FVARIDEye::DisableAllFX()
{
	Blur.Enabled = false;
	Contrast.Enabled = false;
	Inpaint.Enabled = false;
	Warp.Enabled = false;
}

FVARIDProfile::FVARIDProfile()
{
	Name = "UNKNOWN";
	Description = "UNKNOWN";
	Author = "UNKNOWN";
	Date = "UNKNOWN";
	IsValid = false;

	LeftEye.Blur.ID = 0;
	LeftEye.Blur.Name = "LeftEye.Blur";

	LeftEye.Contrast.ID = 1;
	LeftEye.Contrast.Name = "LeftEye.Contrast";

	LeftEye.Inpaint.ID = 2;
	LeftEye.Inpaint.Name = "LeftEye.Inpaint";

	LeftEye.Warp.ID = 3;
	LeftEye.Warp.Name = "LeftEye.Warp";

	RightEye.Blur.ID = 4;
	RightEye.Blur.Name = "RightEye.Blur";

	RightEye.Contrast.ID = 5;
	RightEye.Contrast.Name = "RightEye.Contrast";

	RightEye.Inpaint.ID = 6;
	RightEye.Inpaint.Name = "RightEye.Inpaint";

	RightEye.Warp.ID = 7;
	RightEye.Warp.Name = "RightEye.Warp";
}

FVARIDProfile::FVARIDProfile(const FVARIDProfile& CopyMe)
{
	Name = FString(CopyMe.Name);
	Description = FString(CopyMe.Description);
	Author = FString(CopyMe.Author);
	Date = FString(CopyMe.Date);

	IsValid = CopyMe.IsValid;

	LeftEye = FVARIDEye(CopyMe.LeftEye);
	RightEye = FVARIDEye(CopyMe.RightEye);
}

void FVARIDProfile::EnableAllFX()
{
	LeftEye.EnableAllFX();
	RightEye.EnableAllFX();
}

void FVARIDProfile::DisableAllFX()
{
	LeftEye.DisableAllFX();
	RightEye.DisableAllFX();
}

TArray<FVARIDFX*> FVARIDProfile::GetFX()
{
	TArray<FVARIDFX*> FXArray;

	FXArray.Add(&LeftEye.Blur);
	FXArray.Add(&LeftEye.Contrast);
	FXArray.Add(&LeftEye.Inpaint);
	FXArray.Add(&LeftEye.Warp);
	FXArray.Add(&RightEye.Blur);
	FXArray.Add(&RightEye.Contrast);
	FXArray.Add(&RightEye.Inpaint);
	FXArray.Add(&RightEye.Warp);

	return FXArray;
}

FVARIDFX* FVARIDProfile::GetFX(int32 ID)
{
	TArray<FVARIDFX*> FXArray = GetFX();

	FVARIDFX* FX = nullptr;

	if (FXArray.IsValidIndex(ID))
	{
		FX = FXArray[ID];
	}

	return FX;
}

void FVARIDProfile::ToggleFX(int32 ID)
{
	FVARIDFX* FX = GetFX(ID);

	if (FX != nullptr)
	{
		FX->Enabled = !FX->Enabled;
	}
}