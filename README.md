# 1. Introduction

This is my exercise on ray tracing, based on

- Ray Tracing in One Weekend (completed)
- Ray Tracing the Next Week (currently working upon)
- Ray Tracing the Rest of Your Life (upcoming)
- Real-Time Rendering (forth Edition) (upcoming)

> Author's source URL
>
> https://github.com/petershirley/raytracinginoneweekend
> 
> https://github.com/petershirley/raytracingthenextweek
> 
> https://github.com/petershirley/raytracingtherestofyourlife

Along with changes, optimizations, and additions to the content
For details, please refer to the notes and source code
The graphics interface uses OpenGL 4.4

Recently working on > **Ray Tracing the Next Week**

# 2. Code Framework (will be updated)

Inside ./Raytracing-Sandbox/Src/
```
Compute-Shader/
   00_Basic_Compute_Shader
   01_Blur_Using_Compute_Shader
   02_Evolving_Pics (Partially-finished due to unforseen circumstances (ristricted and small gpu stack))

In-One-Weekend/
   00_Image
   01_Adding_Sphere
   02_Grouping_Objects(Scene)
   03_Adding_Materials

In-Next-Week/
	00_MotionBlur
	01_BoundingVolumeHierarchy (LBVH to be exact(not GPU but CPU based), NOTE: reflections are not good due to motion blurring not beacause of bug, I always confuse In spelling of heirarchy, code needs very-good amount of cleanup)
   
```

# 2. How to use

## 2.1 Dependencies

- Glad (OpenGL version 4.4 +) > https://glad.dav1d.de/
- glfw > https://github.com/glfw/glfw
- spdlog > https://github.com/gabime/spdlog
- glm > https://github.com/g-truc/glm
- imgui > https://github.com/ocornut/imgui/tree/docking
- stb_image > https://github.com/nothings/stb/blob/master/stb_image.h

## 2.1 Environment

- Visual Studio 2017+ (for C++ 17)
- Windows-10
- Git
- Premake (included)

## 2.2 Download code

```bash
git clone --recursive https://github.com/ishanshLal-tRED/Raytracing-tests
```

## 2.3 Compile

1. Build the project
`Just run {repo_DIR}/#Scripts/Win-Premake-GenProjects.bat`

2. Open `RayTracing-tests.sln`