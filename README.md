# DouStyle

DouStyle is a small OpenGL learning renderer built around a pass-based pipeline.
It is intended as a place to experiment with deferred shading, VXGI, shadows,
voxel visualization, and other graphics algorithms.

## Features

- OpenGL 4.x rendering context
- GLFW window and camera controls
- Assimp model loading
- Forward Lambert lighting
- Deferred G-buffer and PBR direct lighting
- Voxelization with RGB and sample-count atomic accumulation
- VXGI cone tracing for indirect lighting
- Shadow map pass
- G-buffer and voxel debug views
- RenderDoc debug markers

## Controls

| Key | Action |
| --- | --- |
| `WASD` | Move camera |
| `E / Q` | Move camera up / down |
| Mouse | Look around |
| `1` | Toggle direct lighting |
| `2` | Toggle indirect diffuse lighting |
| `3` | Toggle indirect specular lighting |
| `4` | Show AO |
| `5` | Show voxel debug |
| `6` | Show G-buffer debug |
| `7` | Cycle G-buffer debug mode |
| `8` | Switch Forward / Deferred rendering |
| `Esc` | Exit |

## Building

Open `DouStyle.sln` with Visual Studio and build the `Debug|x64` or
`Release|x64` configuration.

The current Visual Studio project expects the existing third-party libraries
under `../VXGI-Renderer/vendor` and the Sponza model under
`../VXGI-Renderer/model/sponza_pbr/glTF/Sponza.gltf`.

## Screenshots

Add screenshots here later, for example:

```markdown
![Deferred lighting](images/deferred-lighting.png)
![Voxel debug](images/voxel-debug.png)
```

## Notes

Build outputs, Visual Studio user settings, RenderDoc captures, and local
assets are excluded from version control. The project is intended for graphics
learning and experimentation rather than production use.
