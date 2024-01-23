# INF707 - Fundamental graphics programming elements

## The game

Race up to the finish line in as little time as possible!

Controls :
- move with `WASD`
- strafe with `left`/`right`
- switch camera using `F`
- respawn to checkpoint with `R`
- use boost with `space`
- open/close menus with `esc`

Try to beat 4'30"!

![missing image](documents/screen_2.png)
![missing image](documents/screen_3.png)

See more on [itch.io](https://aikeon.itch.io/celespeed).

## The engine

Key features :
- Scene system
- Graphics pipeline (forward)
- PhysX integration
- An ImGui-based editor

Visual effects & techniques :
- Transparents (`GameScene::render`)
- VFX Ribbons (`player_ribbons.cpp`, `ribbon.cpp`)
- MSAA (`frame_buffer.cpp`, `msaa.hlsl`)
- Depth of field, single-pass & seamless but with some leaking (`depth_of_field.fx`)
- Shadows (`sun.cpp`, `shadow.fx`, `phong.hlsl`)
- Screen shake (`player_cameras.cpp`)
- Vignette (`vignette.fx`)
- Chromatic aberration (`chromatic_aberration.fx`)
- Bloom, single-pass, see comments in `bloom.fx`

## Building & running

First, you will have to unzip `runtime/`, `vendor/` and `lib/`.

All libraries are self-contained, either use the provided visual studio solution or add `vendor/`, `vendor/physx` and `src/` in your include path, `dxguid.lib;winmm.lib;d3d11.lib;d3dcompiler.lib;dxgi.lib;Effects11d.lib;PhysX_64.lib;PhysXCommon_64.lib;PhysXExtensions_static_64.lib;PhysXFoundation_64.lib;PhysXPvdSDK_static_64.lib` as your required libraries and be sure to run in `runtime/`.

## The project

**Authors**:
- Erwan Sénéchal - [Aikeon](https://github.com/Aikeon)
- Jos Landuré - [Tyjos29](https://github.com/Tyjos29)
- Léa Arnaud - [lea-arnaud](https://github.com/lea-arnaud)
- Albin Calais - [Akahara](https://github.com/Akahara)

## Dev. notes

### The repo

The repository has been imported from a private gitlab instance, the commit history has been removed and lfs files have been put in .zip files instead (github's lfs storage is not cheap!).

### Include graph

A nice [tool](https://github.com/pvigier/dependency-graph) to generate the project's include graph. I suggest swapping lines 61-62 with 63-64 to have the headers inclusing in red and cpp inclusions in blue. Our goal is not to expose the DirectX header to "user-code" (ie. scenes and stuff). We can't do the same with physx but not having it transitively included in header files is still good practice.

Using dependency inversion (rarely injection), header dependencies is straightforward and user code is never poluted with unwanted libraries.

### Texture generation

We are using `.dds` files for most textures, we had some problems with standard image-editing tools but using NVidia's `texconv.exe` worked well, do not forget to flip the texture upside down.

### Skybox generation

`.dds` files can contain multiple types of textures, when generating cubemap textures *do not* forget to check that your DDS file contains a texture array with a length multiple of 6.  You may use [nvidia's tool](https://developer.nvidia.com/nvidia-texture-tools-exporter) to do so.

### Font generation

Font rendering is done using font-sheets, see https://github.com/evanw/font-texture-generator for the sheet generation. To add a font, apply [this patch](https://github.com/evanw/font-texture-generator/pull/2), save the generated image under `/runtime/res/fonts/yourfont.dds`, add the generated character array to `src/display/generated_fonts.h` and add the font declaration to `src/display/text.h|cpp`.
