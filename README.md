# **VARID**
### Virtual & Augmented Reality for Inclusive Design

## Summary
- VARID is a plugin for Unreal Engine.
- VARID can simulate a number of eye conditions via user defined profiles.
- VARID ultimately aims to become the ‘standard’ for simulating eye conditions. 
- It is an open, accessible tool that continues to grow with the help of the open source community.
- VARID is complete rewrite of the OpenVisSim project by Peter Jones.
  - https://github.com/petejonze/OpenVisSim
  - https://www.nature.com/articles/s41746-020-0242-6

IMPORTANT
- VARID requires new SceneViewExtension functionality only available in Unreal 4.26 (and above)


## Credits
- Joe Bacon - Make Transition Ltd - Initial Software engineering - https://www.maketransition.co.uk/
- Peter Jones - City University - Original creator of OpenVisSim, VARID Consortium
- David Gillespie - Foster & Partners - Applied R&D, VARID Consortium
- Francis Aish - Foster & Partners - Applied R&D, VARID Consortium
- Epic Games - Creator of Unreal Engine, Funding, Support


## Use Cases

### Unreal Blueprint designer:
- download plugin from git as a zip or clone the repo
- create a new or find an existing Unreal project
- place the plugin into the project's plugins directory
- use one of the example levels in the plugins content folder OR use an existing user level OR create a new level
- ENSURE game mode to VARID_GameMode. This can be set for in the level or the project:
  - World Settings > Game mode
  - Project Settings > Project > Maps & Modes > Default Game mode
- control the plugin using console commands or blueprint functions

### Unreal C++/Shader Developer:
...



## Design
- The simulation of the eye should be the LAST image processing to happen in the image processing pipeline - in order to be true to human eyesight. 
- Therefore VARID FX are applied after typical post processing such as antialiasing & tone mapping.
- Plugin does not require custom Unreal engine changes.


### Software Structure Overview
- Unreal Application
  - Content e.g. app levels, models, blueprints, materials, etc
  - Plugins
  	- VARIDPlugin
    	- Content - levels - Examples AR and VR
  		- VARIDModule
  		- VARIDRendering
  		- VARIDProfile
  		- VARIDEyeTracking
  		- VARIDBlueprintFunctionLibrary
  		- VARIDCheatManager
  		- USBCamera - blueprint only
  	- 3rd Party Eye Tracking Plugin e.g. SRanipal, OpenXR
  	- 3rd Party Passthrough Cameras e.g. SRWorks, ZedMini

## Tools
- Unreal Engine 4.26.2. 
  - Additionally also download PDB synbols via install options. This makes it easier to step into Engine code if need be)
- Visual Studio Community 2019
- Git
- Git client e.g. source tree, kraken, Git Desktop. Just makes it easier to visualise the repo. Not essential.
- RenderDoc - for profiling graphics

## Example Hardware

### Developement PC
- Intel Core i7-6700K CPU @ 4.00GHz
- 32.0 GB RAM
- NVIDIA GeForce GTX TITAN X
- DirectX 11 - shader model 5
- Windows 10


### Performance
plugin 5ms



### HMD
- VIVE Pro Eye
- Front facing cameras - 96 degs horizontal and 80 degs vertical field of view with a 480p resolution per eye. 
- They can also capture up to 90fps with an average latency of 200ms, so they're good enough to avoid any lag-related nausea.
- AMOLED displays - 1440 x 1600 pixels per eye (2880 x 1600 pixels combined), 90 Hz, 110 degrees
- https://www.vive.com/uk/product/vive-pro-eye/specs/
- In reality it is 110 degrees on vertical and 106 degrees on horizontal. source: https://forum.vive.com/topic/8550-configuration-of-fov-of-htc-vive-pro-eye/?ct=1626168483

## Unreal

### Important Components

- Plugins
- RDG
- RHI
- Shaders
- SceneViewExtension

### Typical Unreal Project Settings
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
Go to: Engine\Build\BatchFiles
execute: .\RunUAT.bat BuildPlugin -Plugin="YOUR_ROOT/YOUR_APP/Plugins/VARID/VARID.uplugin" -Package="YOUR_PACKAGE_OUTPUT_FOLDER" -CreateSubFolder -TargetPlatforms=Win64 -VS2019

### Learning
- https://www.unrealengine.com/en-US/onlinelearning-courses
- https://www.youtube.com/watch?v=afodIcU_vK4&ab_channel=UnrealEngine
- https://www.unrealengine.com/en-US/support
- http://www.tharlevfx.com/
- https://www.tomlooman.com/ue4-gameplay-framework/

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
- Unreal provide a generic Eye Tracking API via a blueprint function library
- Under the hood this will use a specific eye tracking API e.g. Unreal will automatically link it to the most appropriate eye tracker: SRanipal, OpenXR, Windows MR, Magic Leap... 
  - The downside of using a generic API is that functionality is limited to a common set of functions - which may mean you cant access vendor specific data. For example VIVE SRanipal has additional conveniance functions for getting pupil position.
  - The upside is that VARID is not tied to a specific 3rd party eye tracking API.
   VARID requires raw eye tracking data in a normalised format (0,0) means the eye is centralised
- Of course you may replace the generic API with more specific API as it suits. 
- VARID is designed to use the generic eye tracking functions in order to make the plugin compatible with more eye tracking hardware. 
- if no eye tracking API is found then VARID will fall back to using the mouse to emulate eye movement
- To use the generic eye tracking API, you have to use the eye gaze direction data (not eye gaze origin!)
- You also have to to use the Y and Z components of the direction. 
- Direction to pupil position mapping: Y -> X and Z -> Y
- Values have to be attenuated by a sensitivity factor (set in BP_VARID_Pawn)

- VERY IMPORTANT - you still have to install and/or enable a specific eye tracker API (usually a plugin e.g SRanipal) depending on your hardware

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



## Profile
- Json text file
- Use an online json linter to check for problems with a profile e.g https://jsonparser.org/


### VF Maps
- Data is defined so that high numbers are good vision, low numbers are low vision.
- black patches on a normal VF map would indicate low vision.
- Internally VARID uses textures to represent the VF map.
- The convention within these textures is that black patches are good and white patches are bad.
- This is because a white value == higher value == more attenuation == increase FX
- Most VF maps only require a single channel texture so in fact low vision patches are represented by red colour
- The normal map used by the warp FX is a two channel texture - red and green. 
- If an FX has been defined in the json, then it is expected to be well formed. i.e. it must have a data field

#### Field: Data
- Mandatory
- VF Map Points can be specified in tuples of 3 or 5:
  - 5-Tuple: [X degs, Y degs, Value dB, Min dB, Max dB]
  - Single 3-Tuple: [Value dB, Min dB, Max dB]  
- If more than one value is given then it means the effect will be interpolated between the value points
- If only one value is given in a tuple of 3, then it means it applies to the entire field. X and Y are not specified

#### Field: expected_num_data_points
- optional
- used to sanity check the number of points actually defined in the data array. helpful if you have many data points

### Comments are not allowed (in json!) 
- yes you can trick some json parsers into allowing comments but its not proper json and makes is less portable. 
- use the description field for notes. 


## Possible Eye Tracking Inputs
- Static
- Scripted
- Mouse
- Touch Screen
- Eye tracking hardware e.g. Tobii
- Camera e.g. Apple True Depth Camera + ARKit


## How to interact with the VARID plugin
- blueprint function library
- Console commands
  - Implemented using a cheatmanager attached to the player controller.
  - All commands prefixed with VARID_
  - only availale in non shipped version. Useful for development, debugging, testing.
  - The best way to see all the up to date commands is to bring up the full console
  - to get the full console press the backquote key twice i.e: ``
  - console intellisense will kick in and list all VARID BP functions.
  - The full console is large and allows output messages to be displayed - useful for displaying lists of profiles and fx, error messages etc
  - Alternatively have a look at: 'VARID\Source\VARID\Public\VARIDCheatManager.h'

## Augmented Reality Passthrough Cameras

### Options

#### Zed Mini
- (Stereo) 
- has a dedicated unreal plugin: https://www.stereolabs.com/docs/unreal/​ 
- however this requires changes to the engine. 

#### VIVE SRWorks
- This plugin enables realtime passthrough video from the VIVE Pro Eye front facing cameras into Unreal.
- Whilst the video is stereo and acceptable latency, the video is low relatively low resolution: 640x480

#### Logitech C920
- Mono
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
- use the level VARID_AR_C920
- insert artificial delays between calls to the camera hardware. This gives it time to complete. Implies that the hardware calls are asynchronous. 
- camera is othographic
- the plane with the rendered texture should be more than 10 units away from the camera
- enumerators for Audio, Video and webcam capture devices (where webcam is used for Mobile devices as you can get the Front or Rear cameras).
- camera is set to start on BeginPlay.
- 'c' key can be used to retart the camera
- important part of the AR camera blueprint is at the end. It switches to the static camera using node 'Set view target with blend'. 
- By default a new camera would normally be spawned by the player camera controller. We dont want this camera. We want the camera that has been manually setup and positioned in front of the video texture plane.

## Relevant Graphics Techniques
use of compute shaders

### Pyramids & Mipmaps
- Mipmap = multum in parvo, meaning "much in little"
- Terms are interchangable​
- Mipmap is commonly used within computer graphics to describe a texture with multiple levels. Each level is half the size of the level preceeding it.​
- Very useful in image processing. Working on lower resolution levels can often offer big performance gains​
- We manually build the our texture Mipmaps in order to completely control interpolation and scaling technique
- Use trilinear filtering to get values between mips levels (assuming bilinear has been used in 2d)
- https://en.wikipedia.org/wiki/Pyramid_(image_processing)

### Resampling
- The most intensive downsampling stage is the first few as these are on the biggest size textures. Compute shaders offer big speed ups on these first few passes. They perform the work in a tile - reduce down to 1x1 tile, sync all tiles and reperform reducing on the tiles. 
- Cant just use default mip map generator as it uses linear interpolation. We need the downsample to use gaussian interpolation. This is the case for blur and csf FX.

### blurs
- https://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
- https://venturebeat.com/2017/07/13/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms/
- https://fgiesen.wordpress.com/2012/07/30/fast-blurs-1/
- https://www.gamasutra.com/view/feature/3102/four_tricks_for_fast_blurring_in_.php?print=1
- https://software.intel.com/content/www/us/en/develop/blogs/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html

Separable filter - applies to gaussian
- https://bartwronski.com/2020/02/03/separate-your-filters-svd-and-low-rank-approximation-of-image-filters/

Order
- https://dsp.stackexchange.com/questions/18281/can-the-order-of-filtering-and-downsampling-be-exchanged


## VF Map Textures
- VARID Profiles are converted into VF Map Textures for internal usage by the shaders. 

### Height Map
- 1 channel texture
  - R: Interpolated Value
- Gaussian RBF interpolation

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

## FX

### Inpainter
- Aim: to diminish reality by replacing it with sythesized image information. 
- or in other words to fill missing areas of an image with believable but fake data​

- Check out the shader: VARID\Shaders\Private\VARIDInpainterFillCS.usf
- This is run multiple times on the same texture. Each pass allows the fill algorithm to tak another step.
- Developers are encouraged to try different inpainter algorithms in this shader. 
- VARIDInpainterInitialiserCS.usf and VARIDInpainterFinaliserCS.usf start surround the multipass inpainter shader 
- The number of passes is currently defined in the C++ rendering code. This should be turned into a user definable parameter - or perhaps even make it self terminating if the shader returns a flag indicating no more pixels left to fill
- 
- First approach: Infer the fake data using the regions that surround the missing areas​
- Larger missing areas make the job harder to create convincing results​
- Realtime inpainting exists but for smaller missing areas e.g. scratches and relatively low resolutions I.e. HD 1080. We are dealing with higher resolutions and two displays therefore two rendering passes. ​
- Larger areas often solved by machine learning (ML)​
- Non-ML implementations are CPU based rather than GPU. The advantage of the CPU is that it can manipulate the image in a more straightforward and easily programmable manner. I.e. it can operate in a logical sequence. 
- Improvements: Find constraints in the image – automatic structure detection

- Complications - possibly not dealing with just small areas - e.g. scratches 
- openCV implementation uses two approaches
	- Navier stokes fluid dynamics
	- Alexandru Telea. fast marching method
- Often heavy on computation/deep learning!

See: p.162 Computer Vision: Algorithms and Applications (March 27, 2021 draft) describes how pyramid could be use for texture synthesis
- https://recreationstudios.blogspot.com/2010/04/sobel-filter-compute-shader.html
- https://docs.opencv.org/master/df/d3d/tutorial_py_inpainting.html
- https://en.wikipedia.org/wiki/Inpainting
- https://github.com/Mugichoko445/PixMix-Inpainting
- https://en.wikipedia.org/wiki/Seam_carving
- https://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch38.html
- https://upload.wikimedia.org/wikipedia/commons/5/53/Creation_of_Adam_seam_carving_interactive.svg
- https://patents.google.com/patent/EP3097535A1/en


NEW
- Target: fill a reasonably sized scotoma within 20 shader passes at mip level 4

### Blur
- Gaussian Pyramid (bottom layer – highest resolution is unprocessed I.e. not gaussian low pass). This is purely for convenience. We don’t ever need to use a gaussian low pass texture in the highest resolution however we do often need the original unprocessed image available. ​
- VF Map controls which level of the pyramid to sample from​
- Sample the layers of the pyramid using Trilinear Filtering for smoother interpolation between the layers (bilinear = 2d, trilinear = 3d where the 3rd dimension equals the layers of the pyramid)​
- VF Map pixel value = 0 = sample from the highest resolution pyramid​
- VF Map pixel value = 1 = sample from the lowest resolution pyramid

### Warp
- Warping, bending, bulging of the image​
- VF Map created using a shader that converts a height map to a normal map
- Add/subtract offset to texture position​
- Stretches and squeezes the texture / pinch, bulge
- No agreed standard for distortion data


### Contrast
- Analogy – a 10 band 'graphic equaliser' for image contrast. ​
- Ability to attenuate specific frequency bands. ​
- Ability to attenuate specific regions.​
- Negative side effect: Banding artifacts can sometimes be seen. Most likely data type precisions. Needs research...​
- Typically 10 bands – could be slightly more of less depending on the resolution which impacts the number of times the image can be halved all the way down to 1 pixel size. 
- https://stackoverflow.com/questions/12568627/gpu-based-laplacian-pyramid
- Simplified gaussian: exp(-0.5 * pow(3.141 * (x), 2));
- https://automaticaddison.com/how-the-laplacian-of-gaussian-filter-works/
- https://geolographer.xyz/blog/2017/2/27/an-introduction-to-pyramid-shader
- Multi pass render target. 


## CloudXR
- CloudXR is not compatible with VARID. At time of writing Q2 2021, it is not Not possible to send realtime camera image to the server (therefore AR not possible) and eye tracking is not supported therefore even in VR mode it would be quite limited. 
- No back channels for sending eye tracking data. not even for sending microphone audio. This will likely change. 



## TODO

Here is a list of outstanding jobs for the future. 

### BUG: Aspect ratio for VF Map point positions
- VF point positions are internally converted to normalised 0..1 space
- The aspect ratio of the display needs to be part of the conversion process otherwise the VF map points can appear stretched on displays with non square aspect ratios
- This is more prevelent on a 16:9 desktop display 

### BUG: VR to PIE image scaling
- Run VARID in VR mode. 
- Then run VARID in a desktop 'play in editor' window - the rendered area does not change with window size. The image is scaled down.
- VR will keep working but desktop is stuck in this scaled size
- The only way to fix it is to restart the editor.
- The problem appears to be how the size of the back buffer is managed. The VR size is infecting the PIE size. I have tried manually setting the size but not luck.
- For VR the back buffer seems to be managed elsewhere:
- FVARIDSceneViewExtension::PostProcessPassAfterTonemap_RenderThread
- BackBufferRenderTarget = InOutMaterialInputs.OverrideOutput;
- Where as for PIE, the back buffer is created within FVARIDSceneViewExtension::PostProcessPassAfterTonemap_RenderThread
- If you remove the VARID plugin, the problem goes away. It is definitely caused by the VARID plugin but not obvious how. It may be that the VARID plugin being preset is exposing an unreal bug

### BUG: Console does show up properly in VR mode
- The console is present but it is not possible to read the commands easily on the desktop spectator screen
- Work around is to bind keys to blueprint functions

### VF Map Extrapolation
- Improve the VF map by extrapolating the data outwards to always fill the screen to the edges. This will most likely involve taking the outer most points of the VF map and use them as the basis of the extrapolation. 
- Any solution needs to take into account that the VF map points may define more than one area of sight loss and may form non convex (concave!) shapes ideas 
- tagging the points with different types i.e. 0 = real point, 1 = display edge point, etc
- create convex hull around all points and copy, scale, repeat the points on the hull. 
- Algorithms: 
  - https://www.codeproject.com/Articles/1210225/Fast-and-improved-D-Convex-Hull-algorithm-and-its
  - https://www.geeksforgeeks.org/quickhull-algorithm-convex-hull/

### VF Map Shader Parameters
- ability to tune standard deviation of the RBF height map shader
- ability to tune standard deviation of the gaussian low pass/blur/filter shader

### External camera zoom and position parameters
- This effectively scales and moves the camera texture plane in x and y. use keyboard controls? arrows and +/-?

### Testing Contrast FX
- ability to test the contrast FX using gabor wavelets "Sine-wave grating charts"

### Console commands
- Refactor cheat manager UFUNCTIONS so that additional meta data can be specified i.e. display name, hints 
- This means the method names can stop being prefixed with VARID_
- use FAutoConsoleCommand

### Cache VF Map textures for performance
- Improve performance of the VF Map texture generation
- After the initial generate, these textures could be saved and reused in subsequent render passes. 
- This will require translating the entire texture when rendered

### Shader performance - memory
- Make better use of LDS/groupshared memory
- Inparticular see where shaders can be combined e.g. ContrastReconstruct. 
- There are earlier versions of VARIDContrastReconstructCS.usf that include more LDS usage.
- It was abandoned for simplicity and in order to get base line stats. 
- Initial work should include making common shared functions for working with the LDS. See VARIDCommon.ush

### Shader performance - combining shaders
- Some shaders logic be combined e.g. upsample and blur. see Shader performance - memory
- GroupMemoryBarrierWithGroupSync() may be needed

### Shader performance - region specific FX
- limit the number of pixels that are processed by limiting the compute shader dispatch size to contain only the affected areas of the screen and no more
- i.e. instead of full screen processing, do regional processing using bounding boxes.
- be aware that some shaders need to gather full screen information so cant use regions. 

### Shader performance - test if we can cache inpainter random map textures and maintain effectiveness
- this could improve performance but make the randomness stale

### Load first valid profile
- BP_VARID_Pawn loads the first profile returned in an alphabetical list. if that first profile fails to load then no initial profile is loaded. 
- Change this behaviour so that the first valid profile is initially loaded. 

### Better external camera for AR mode. Requirements:
- stereo camera
- wider FOV cameras - ideally match the displays
- higher frame rate - ideally match the displays
- USB 3


### Additional FX
- Frost
- Bloom
- Simple Blur
- Simple Contrast
- Brightness
- Gamma
- Floaters
- Glitch
- LED
- Noise
- Wiggle
- Scintillate
- Nystagmus
- Recolour