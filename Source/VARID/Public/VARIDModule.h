// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Modules/ModuleManager.h"
#include "VARIDProfile.h"
#include "VARIDEyeTracking.h"

class FVARIDSceneViewExtension;

// This class is the hub of the VARID plugin. The IModuleInterface gives us singleton behaviour which is fine because we only want one instance

class FVARIDModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FVARIDModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FVARIDModule>("VARID");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("VARID");
	}

public:
	void BeginRendering();
	void EndRendering();

public:
	void SetProfileRootPath(FString ProfileRootPath);
	void SetProfileExtension(FString ProfileExtension);
	bool ListProfiles(FString ProfileRootPath, FString Ext, TArray<FString>& Files);
	bool ListProfiles(TArray<FString>& Files);
	bool LoadProfile(const FString& ProfileFullPath, FVARIDProfile& OutProfile);
	void SetActiveProfile(const FVARIDProfile& InProfile);
	FVARIDProfile& GetActiveProfile();

public:
	FVARIDEyeTracking& GetEyeTracking();
	void SetEyeTracking(const FVARIDEyeTracking& EyeTracking);

public:
	const FVector2D& GetDisplayFOV();
	void SetDisplayFOV(const FVector2D& InDisplayFOV);

private:
	TSharedPtr<FVARIDSceneViewExtension, ESPMode::ThreadSafe> SceneViewExtension;
	FString DefaultProfileRootPath;
	FString DefaultProfileExtension;
	FVARIDProfile Profile;
	FVARIDEyeTracking EyeTracking;	
	FVector2D DisplayFOV;
};