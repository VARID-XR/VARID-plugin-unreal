// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "VARIDEyeTracking.h"

FVARIDEyeTracking::FVARIDEyeTracking()
{
	LeftEyeGazePoint = FVector2D(0.0f, 0.0f);
	RightEyeGazePoint = FVector2D(0.0f, 0.0f);
}

FVARIDEyeTracking::FVARIDEyeTracking(const FVARIDEyeTracking& CopyMe)
{
	LeftEyeGazePoint = FVector2D(CopyMe.LeftEyeGazePoint);
	RightEyeGazePoint = FVector2D(CopyMe.RightEyeGazePoint);
}