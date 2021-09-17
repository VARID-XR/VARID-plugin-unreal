# **VARID**
### Virtual & Augmented Reality for Inclusive Design

## Summary
- VARID originates from the OpenVisSim project by Dr Peter Jones. 
  - https://github.com/petejonze/OpenVisSim
  - https://www.nature.com/articles/s41746-020-0242-6
- VARID is a plugin for Unreal Engine.
- VAIRD performs realtime image processing.
- VARID realistically simulates a number of eye conditions via user defined profiles.
- VARID ultimately aims to become the ‘standard’ for simulating eye conditions. 
- VARID is an open, accessible tool that should continue to grow with the help of the open source community.
- VARID does not require custom Unreal engine changes.

## Credits
- Joe Bacon - Make Transition Ltd - Initial Software engineering, https://www.maketransition.co.uk/
- Dr Peter Jones - City University - Original creator of OpenVisSim, VARID Consortium, https://www.appliedpsychophysics.com/
- David Gillespie - Foster & Partners - Applied R&D, VARID Consortium
- Francis Aish - Foster & Partners - Applied R&D, VARID Consortium
- Epic Games - Creator of Unreal Engine, Funding, Support

## Requirements
- Unreal Engine 4.26+ - VARID requires the new post process SceneViewExtension functionality introduced in Unreal 4.26.

## Recommendations
- Software
  - Unreal Engine 4.26.2 PDB synbols via install options. This makes it easier to step into Engine code if need be.
  - Visual Studio Community 2019 - preferred IDE.
  - Git - source control.
  - RenderDoc - profiling/debugging graphics.
- Hardware
  - Windows 10 powerful desktop or laptop.
  - Multi-core CPU.
  - As much RAM as you can give! Unreal likes having a lot of RAM. 
  - Storage - fast as possible. There are a lot of files to touch when building an Unreal Project.
  - GPU - anything from NVIDIA GTX 1060 or Radeon RX 580 upwards (for minimum VR support).

## How to...

### Get Started
- Download this repo (clone or zip). This repo is the VARID plugin. It will require an Unreal application to plug into and actually work.
- Create a new Unreal project or find an existing suitable Unreal project.
- Place the VARID plugin into the project's plugins directory.
- The plugin directory should simply be called 'VARID'. (Not 'ue4-varid-plugin')
- Ensure the VARID plugin is enabled: Editor > Edit > Plugins > Other > VARID > Enabled = True.
- VARID FX are applied to an Unreal 'level'.
- Load one of the example AR/VR levels found in the VARID plugin content folder.
- If you want to add VARID to your own level please ensure the level's 'game mode' is set to VARID_GameMode. This can be set via World Settings > Game mode. Alternatively, manually add BP_VARID_Pawn to your level. 
- Using the VARID game mode will automatically spawn a VARID Pawn in the level. (More about the VARID Pawn below).
- To get the most out of VARID it is recommended you use an HMD with eye tracking.
- VARID Pawn is setup for the VIVE Pro Eye by default - if you are using a different HMD you must set the HMDs vertical and horizontal display FOV using the VARID blueprint function - or just change the default in the Pawn.

### Typical Project Structure
- Unreal Project
  - Project Content e.g. app levels, models, blueprints, materials, etc
  - Plugins
    - VARID
      - Content
        - Blueprints
          - Pawn
          - Player Controller
          - Game Mode
        - Levels
          - AR
          - VR
        - Profiles
          - Templates full/minimum
          - Tests
        - USBCamera
          - Implemented using only Blueprints
          - Experimental
          - Currently only supports Logitech C920
      - C++
        - VARIDModule
        - VARIDRendering
        - VARIDProfile
        - VARIDEyeTracking
        - VARIDBlueprintFunctionLibrary
        - VARIDCheatManager
      - Shaders
    - 3rd Party Eye Tracking Plugins e.g. VIVE SRanipal, OpenXR
    - 3rd Party Passthrough Camera Plugins e.g. VIVE SRWorks, ZedMini

### Helper Blueprints
- The following blueprints are purely for convenience. They do not have to be used but they will accelerate VARID integration as common events are already wired up ready to go.
- VARID Pawn 
  - Links VARID functions to game events - does all the required tasks: e.g. init, start rendering, tick, end rendering.
  - Can be used as an example starting point for controlling the VARID plugin from a blueprint. 
  - A Pawn is used rather than an Actor as Pawns are already setup to use player input e.g. walking around the level using mouse look/keyboard (WASD keys).
  - Basic key bindings to enable/disable FX 1-4 for each eye.
  - Helper functions for getting eye position from either a hardware eye tracker or the mouse.
- VARID player Controller - links the cheat manager. Changes mouse to use cross hairs cursor. 
- VARID Game Mode - sets default Pawn and default player controller. 

### Interact with the VARID plugin functionality
- The VARID Pawn will setup the plugin ready for most scenarios.
- It is expected that the user will be responsible for binding VARID functionality to specfic motion controllers, keyboard presses, bounding box collisions, game controller button presses etc
- Interaction is usually done via the blueprint function library. See VARIDBlueprintFunctionLibrary.h
  - SetProfileRootPath
  - SetProfileExtension
  - ListProfiles
  - LoadProfile
  - GetActiveProfile
  - SetActiveProfile
  - ListFX
  - ToggleFX
  - EnableAllFX
  - DisableAllFX
  - BeginRendering
  - EndRendering
  - GetEyeTracking
  - SetEyeTracking
  - GetDisplayFOV
  - SetDisplayFOV
- Console commands also available. 
  - Implemented using a cheatmanager attached to the player controller.
  - All commands prefixed with VARID_ so that console intellisense can find them.
  - only availale in non shipped version. Useful for development, debugging, testing.
  - The best way to see all the up to date commands is to bring up the full console
  - To get the full console press the backquote key twice i.e: ``
  - Console intellisense will kick in and list all VARID BP functions.
  - The full console is large and allows output messages to be displayed - useful for displaying lists of profiles and fx, error messages etc
  - Alternatively have a look at: VARIDCheatManager.h
- Default Key Bindings (defined in VARID Pawn)
  - 0 = Toggle FX Left Blur
  - 1 = Toggle FX Left Contrast
  - 2 = Toggle FX Left Inpaint
  - 3 = Toggle FX Left Warp
  - 4 = Toggle FX Right Blur
  - 5 = Toggle FX Right Contrast
  - 6 = Toggle FX Right Inpaint
  - 7 = Toggle FX Right Warp
  - , = All FX enabled
  - . = All FX disable

### VR vs Desktop
- In Desktop mode only the left eye FX is displayed.

### Possible Eye Tracking Inputs
- Eye tracking hardware e.g. Tobii. This is the preferred way of using VARID.
- Mouse - If eye tracking hardware is not available VARID will fall back to using the mouse.
- Static - You could disable the mouse and provide a constant gaze position.
- Scripted - You could move the eyes in a scripted commands for repeated testing.
- Touch Screen - You could port VARID to a mobile device and use its touch screen to determine gaze point.
- Camera e.g. Apple True Depth Camera + ARKit - eye tracking from modern devices is getting reliable. It should be possible to control VARID this way if successfully ported onto an Apple device. 

## Tested On

### Developement PC
- Intel Core i7-6700K CPU @ 4.00GHz
- 32.0 GB RAM
- NVIDIA GeForce GTX TITAN X
- DirectX 11 - shader model 5
- Windows 10
- Samsung SSD 970 EVO 1TB
- Renderdoc processing time = ~5ms @ 4k

### HMD
- VIVE Pro Eye
- Front facing cameras
  - 96 degs horizontal and 80 degs vertical field of view with a 480p resolution per eye. The resolution is poor, hence the experimental work using the Logitech C920.
  - They can also capture up to 90fps with an average latency of 200ms, so they're good enough to avoid any lag-related nausea.
- AMOLED displays - 1440 x 1600 pixels per eye (2880 x 1600 pixels combined), 90 Hz, 110 degrees.
- https://www.vive.com/uk/product/vive-pro-eye/specs/
- In reality it is 110 degrees on vertical and 106 degrees on horizontal. source: https://forum.vive.com/topic/8550-configuration-of-fov-of-htc-vive-pro-eye/?ct=1626168483

## Unreal

### Important Top-Level Components
- Plugins - Architecture that allows functionality to be plugged into existing Unreal applications. 
- RDG - Render Graph - allows rendering tasks to be built in a graph like manner. Makes it easier, safer and more efficient to chain lots of shader calls together.
- RHI - Rendering Hardware Interface - provides an abstract interface so that the developer doesnt have to deal with a specific graphics library e.g. DirectX, OpenGL, Vulkan.
- Shaders - Written in HLSL, these are small C like programs run on the GPU.
- SceneViewExtension - Extend from this base class to allow us to tap into the Unreal graphics pipeline during the post process stage.

### Typical Unreal New Project Settings
- New Project
  - Hardware Target: Mobile/Tablet
  - Graphics Target: Scalable 3D or 2D
  - No starter content

- Project settings > Engine
    - Rendering
    	- VR
    		- Instanced Stereo = TRUE
    		- Round robin occlusions = TRUE
    		- Mobile Multi-View = FALSE
    		- Mobile HDR = FALSE
    	- Forward Renderer
    		- Forward shading = TRUE
    - Default
    	- Anti Aliasing Method = MSAA
    	- Ambient occlusion static fraction = FALSE
    - Project settings > Project
    	- Description
    		- Settings
    			- Start in VR = TRUE

- Enable any additional plugins you may need e.g. for a Steam VR HMD
  - Edit > Plugins > Virtual Reality > Enable SteamVR

### Packaging the Plugin
- Should you need to distribute the plugin as a binary e.g. dll then you need to package it. 
- Perform this from a system command line. Running via the Engine Editor causes issue: 'requires VS2017'. Resolved by manually running with -VS2019 switch as show below.
- Go to directory: Engine\Build\BatchFiles
- execute: .\RunUAT.bat BuildPlugin -Plugin="YOUR_ROOT/YOUR_APP/Plugins/VARID/VARID.uplugin" -Package="YOUR_PACKAGE_OUTPUT_FOLDER" -CreateSubFolder -TargetPlatforms=Win64 -VS2019

### Communities
- UDN (paid subscription)
- Unreal slackers discord
- Unreal forums
- Reddit r/unreal
- Stackoverflow unreal
- Meetup events - e.g. https://www.meetup.com/London-Unreal-Engine-Meetup/


## Eye tracking

### APIs

#### Generic
- Unreal provide a generic Eye Tracking API via a blueprint function library.
- Under the hood this will use a specific eye tracking API e.g. Unreal will automatically link it to the most appropriate eye tracker: SRanipal, OpenXR, Windows MR, Magic Leap... 
  - The downside of using a generic API is that functionality is limited to a common set of functions - which may mean you cant access vendor specific data. For example VIVE SRanipal has additional conveniance functions for getting pupil position.
  - The upside is that VARID is not tied to a specific 3rd party eye tracking API.
- VARID requires raw eye tracking data in a normalised format (0,0) means the eye is centralised.
- You may replace the generic API with more specific API as it suits. 
- VARID is designed to use the generic eye tracking functions in order to make the plugin compatible with more eye tracking hardware. 
- If no eye tracking API is found then VARID will fall back to using the mouse to emulate eye movement.
- To use the generic eye tracking API, you have to use the eye gaze direction data (not eye gaze origin!).
- You also have to to use the Y and Z components of the direction.
- Direction to pupil position mapping: Y -> X and Z -> Y.
- Values have to be attenuated by a sensitivity factor (set in BP_VARID_Pawn).

- VERY IMPORTANT - you still have to install and/or enable a specific eye tracker API (usually in the form of a 3rd party plugin e.g VIVE SRanipal) depending on your hardware

#### SRanipal
- This 3rd party plugin provided by VIVE enables realtime eye tracking from VIVE Pro Eye Tobii.
- FIXED SRanipal : https://github.com/Temaran/SRanipalUE4SDK
- main tobii core API : https://vr.tobii.com/sdk/develop/ue4/api/core/

- Install runtime (VIVE_SRanipalInstaller_1.3.2.0.msi)
- It will probably fail
- Manually update sranipal service ini
	- C:\Users\joejb\AppData\LocalLow\HTC Corporation\SR_Config
	- EnableEyeTracking=1
	- AcceptEULA=1
- Copy unreal plugin to project plugins folder
- Restart unreal

System tray > Runtime > About: should report:
- runtime version: 1.3.2.0
- eye camera version: 2.41.0-942e3e4

- VERY IMPORTANT - once the SRanipal plugin has been enabled go to:
  -  Project settings > PLugins > SRanipal > Eye Settings > Enable Eye By default = ticked
  -  Project settings > PLugins > SRanipal > Eye Settings > Eye Version = 2

#### OpenXR
- I was not able to get OpenXR eye tracking to work. Crashes Unreal engine at startup. 
- Plugin provided by Unreal Engine. Disabled by default
- Beta versiopn. unstable

### Hardware

#### Tobii
- Tobii make various eye tracking solutions for VR headsets and Unreal. 
- https://www.tobiipro.com/product-listing/vr-integration/
- https://vr.tobii.com/sdk/develop/ue4/getting-started/
- https://vr.tobii.com/sdk/develop/ue4/
- https://developer.tobii.com/pc-gaming/unreal-engine-sdk/api-reference/core/


## VARID Profiles
- Profiles are defined in Json text files.
- 1 profile per json file. 
- Use an online json linter to check for problems with a profile e.g https://jsonparser.org/

### VF Maps
- Data is defined so that high numbers are good vision, low numbers are low vision.
- Black patches on a normal VF map would indicate low vision.
- Internally VARID uses textures to represent the VF map.
- The convention within these textures is that black patches are good and white patches are bad.
- This is because a white value == higher value == more attenuation == increase FX.
- Most VF maps only require a single channel texture so in fact low vision patches are represented by red colour.
- The normal map used by the warp FX is a two channel texture - red and green. 
- If an FX has been defined in the json, then it is expected to be well formed. i.e. it must have a data field.

#### Field: Data
- Mandatory.
- VF Map Points can be specified in tuples of 3 or 5:
  - 5-Tuple: [X degs, Y degs, Value dB, Min dB, Max dB].
  - Single 3-Tuple: [Value dB, Min dB, Max dB].
- If more than one value is given then it means the effect will be interpolated between the value points.
- If only one value is given in a tuple of 3, then it means it applies to the entire field (entire image). X and Y are not specified in this scenario.

#### Field: expected_num_data_points
- Optional.
- Used to sanity check the number of points actually defined in the data array. helpful if you have many data points.

### Comments are not allowed (in json!) 
- Yes you can trick some json parsers into allowing comments but its not proper json and makes is less portable. 
- Use the description field for notes. 

## Augmented Reality Passthrough Cameras

### Options

#### Zed Mini
- Stereo camera.
- Has a dedicated unreal plugin: https://www.stereolabs.com/docs/unreal/​ 
- However this requires changes to the engine itself. 

#### VIVE Pro Eye + VIVE SRWorks
- Stereo camera.
- This plugin enables realtime passthrough video from the VIVE Pro Eye front facing cameras into Unreal.
- Whilst the video is stereo and acceptable latency, the video is low relatively low resolution: 640x480.

#### Logitech C920
- Mono camera.
- a single Logitech C920 usb camera. Requires Unreal webcam input. 
- Latency is acceptable. 
- Resolution up to 1080p. 
- Framerate is uncomfortable when head of target is moving fast.
- FOV of camera and display should ideally be matched. In the example of the C920 with VIVE Pro Eye : 70 degs vs 110 degs respectively. 
- In order to fill the display the image is zoomed but this produces an unnatural experience

##### Links
  - https://docs.unrealengine.com/4.26/en-US/WorkingWithMedia/MediaFramework/HowTo/UsingWebCams/
  - https://www.stereolabs.com/docs/unreal/
  - http://ar.uplugins.com/product/augmented-reality-plugin-for-ue4/
  - https://answers.unrealengine.com/questions/813473/streaming-video-from-a-webcam-onto-the-htc-vive-he.html
  - https://docs.unrealengine.com/4.26/en-US/WorkingWithMedia/MediaFramework/TechReference/
  - https://forums.unrealengine.com/t/webcam-live-video-doesnt-work-black-on-some-of-my-machines/128506/6
  - https://answers.unrealengine.com/questions/720423/live-video-capture-only-works-for-me.html

##### WARNING
  - BP_VARID_AR_Camera will automatically pick the first camera device.
  - If you have multiple cameras attached it may not pick the one you expect. 
  - You can inspect camera devices via the MP_VARID_AR_MediaPlayer asset

##### Notes for using Unreal Media player 
- Use the level VARID_AR_C920.
- Insert artificial delays between calls to the camera hardware. This gives it time to complete. Implies that the hardware calls are asynchronous. 
- Camera is othographic.
- The plane with the rendered texture should be more than 10 units away from the camera.
- Enumerators for Audio, Video and webcam capture devices (where webcam is used for Mobile devices as you can get the Front or Rear cameras).
- Camera is set to start on BeginPlay.
- 'c' key can be used to retart the camera.
- Important part of the AR camera blueprint is at the end. It switches to the static camera using node 'Set view target with blend'. 
- By default a new camera would normally be spawned by the player camera controller. We dont want this camera. We want the camera that has been manually setup and positioned in front of the video texture plane.

## Relevant Graphics Techniques

### Pyramids & Mipmaps
- Mipmap = multum in parvo, meaning "much in little".
- Pyramids & Mipmaps - terms are interchangable.​
- Mipmap is commonly used within computer graphics to describe a texture with multiple levels. Each level is half the size of the level preceeding it.​
- Very useful in image processing. Working on lower resolution levels can often offer big performance gains.​
- We manually build the our texture Mipmaps in order to completely control interpolation and scaling technique.
- Use trilinear filtering to get values between mips levels (assuming bilinear has been used in 2d).
- https://en.wikipedia.org/wiki/Pyramid_(image_processing)

### Resampling
- The most intensive downsampling stage is the first few as these are on the biggest size textures. Compute shaders offer big speed ups on these first few passes. 
- Can't just use default mip map generator as it uses linear interpolation. We need the downsample to use gaussian interpolation. This is the case for blur and contrast FX.

### blurs
- Links
  - https://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
  - https://venturebeat.com/2017/07/13/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms/
  - https://fgiesen.wordpress.com/2012/07/30/fast-blurs-1/
  - https://www.gamasutra.com/view/feature/3102/four_tricks_for_fast_blurring_in_.php?print=1
  - https://software.intel.com/content/www/us/en/develop/blogs/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html

Separable filter - applies to gaussian
- https://bartwronski.com/2020/02/03/separate-your-filters-svd-and-low-rank-approximation-of-image-filters/

Ordering of operations
- https://dsp.stackexchange.com/questions/18281/can-the-order-of-filtering-and-downsampling-be-exchanged


## VF Map Textures
- VARID Profiles are converted into VF Map Textures for internal usage by the shaders. 

### Height Map
- 1 channel texture
  - R: Interpolated Value
- Implements Gaussian RBF interpolation

### Normal Map
- 2 channel texture
  - R: dX
  - G: dY
- UV Normals built from intensity gradients
- Height map is an input parameter

### Position Map
- 4 channel texture
  - R: Interpolated Value (same as height map)
  - G: Distance from nearest VF Map point
  - B: X component distance from nearest VF Map Point
  - A: Y component distance from nearest VF Map Point
-  Not currently used by any FX but could be helpful for future development

## FX

### Blur
- Gaussian Pyramid (bottom layer – highest resolution is unprocessed I.e. not gaussian low pass). This is purely for convenience. We don’t ever need to use a gaussian low pass texture in the highest resolution however we do often need the original unprocessed image available. ​
- VF Map controls which level of the pyramid to sample from​ in the final pixel shader
- Sample the layers of the pyramid using Trilinear Filtering for smoother interpolation between the layers (bilinear = 2d, trilinear = 3d where the 3rd dimension equals the layers of the pyramid)​
- VF Map pixel value = 0 = sample from the finest mip level (original image)
- VF Map pixel value = 1 = sample from the coarsest mip level

### Contrast
- Analogy – a 10 band 'graphic equaliser' for image contrast. ​
- Ability to attenuate specific frequency bands. ​
- Ability to attenuate specific regions.
- Maximum 10 bands – could be less if display resolution is not large enough and therefore does not permit the image to be halved all the way down to 1 pixel size. 
- Inspired by paper: 'Implementation of a spatio-temporal Laplacian image pyramid on the GPU' - Ludwig, 2008 (http://www.gazecom.eu/FILES/ludw08.pdf)
- Links
  - https://stackoverflow.com/questions/12568627/gpu-based-laplacian-pyramid
  - https://automaticaddison.com/how-the-laplacian-of-gaussian-filter-works/
  - https://geolographer.xyz/blog/2017/2/27/an-introduction-to-pyramid-shader

### Inpainter
- Diminished reality by replacing it with synthesized image information - in other words to fill missing areas of an image with believable but fake data​
- Basic Implementation
  - Infer the fake data using the regions that surround the missing areas​.
  - Fill the masked area of a texture using multiple iterative passes e.g. 16 passes.
  - Iterative passes run on a lower resolution version of the image e.g. mip level 3. This achieves much faster processing e.g. 15us vs 250us per pass at the cost of some detail. 
  - Each pass allows the fill algorithm to take another step inward from the boundary of the inpainting area.
  - The inpainted pixels are filled with average colour data taken from neighbouring pixels - hence the result looks somewhat blurred.
  - If an area remains black after inpainting it means that were not enough passes to fill.
- Potential Implementation Issues
  - Larger missing areas make the job harder to create convincing results​.
  - Realtime inpainting exists but for smaller missing areas e.g. scratches and relatively low resolutions I.e. HD 1080@30Hz. We are dealing with higher resolutions and higher refresh rates and two displays for VR therefore two rendering passes. ​
  - There is a lot of commercial interest for inpainting. Be aware of patent infringement when implmenting.
- Links
  - https://en.wikipedia.org/wiki/Inpainting
  - https://www.researchgate.net/publication/260722824_Image_Inpainting_Overview_and_Recent_Advances
- Future work
  - The community is encouraged to improve this FX. 
  - Additional data image is available in this FX pipeline which maybe helpful. For example UV position data and a processing pass counter

### Warp
- Warping = bending, bulging, stretching, squeezing, pinching of the image​.
- VF Map created using a shader that converts a height map to a normal map.
- Add/subtract offset to texture position​ in the final pixel shader.
- No agreed standard for distortion data.
- Sensitivity can be adjust in shader VARIDNormalMapCS.usf

## CloudXR
- CloudXR is not compatible with VARID. 
- At time of writing Q3 2021, it is not Not possible to send realtime camera image to the server (therefore AR not possible) and eye tracking is not supported therefore even in VR mode it would be quite limited. 
- No back channels for sending eye tracking data. not even for sending microphone audio. This will likely change. 
