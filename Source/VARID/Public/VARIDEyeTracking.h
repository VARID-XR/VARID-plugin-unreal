// This source code is provided "as is" without warranty of any kind, either express or implied. Use at your own risk.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "VARIDEyeTracking.generated.h"

USTRUCT(BlueprintType)
struct FVARIDEyeTracking
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVector2D LeftEyeGazePoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VARID")
		FVector2D RightEyeGazePoint;

	bool operator == (const FVARIDEyeTracking& Other) const
	{
		return
			(
				LeftEyeGazePoint.X == Other.LeftEyeGazePoint.X
				&& LeftEyeGazePoint.Y == Other.LeftEyeGazePoint.Y
				&& RightEyeGazePoint.X == Other.RightEyeGazePoint.X
				&& RightEyeGazePoint.Y == Other.RightEyeGazePoint.Y
				);
	}

	bool operator != (const FVARIDEyeTracking& Other) const
	{
		return !(*this == Other);
	}

public:
	FVARIDEyeTracking();
	FVARIDEyeTracking(const FVARIDEyeTracking& CopyMe);
};