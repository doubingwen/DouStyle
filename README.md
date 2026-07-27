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

The current Visual Studio project keeps its third-party libraries under
`ThirdParty/`. The Sponza model is expected under
`../VXGI-Renderer/model/sponza_pbr/glTF/Sponza.gltf`.

## Adding a Render Pass

DouStyle keeps passes as small, independent C++ classes. There is no required
base class: a pass normally owns its shader and OpenGL resources, exposes an
`initialize()` function, and provides a `render()` function that receives the
resources it needs from earlier passes.

### 1. Create the pass class

For example, create `Graphics/Passes/MyPass.h`:

```cpp
#pragma once

#include "../Shader.h"

#include <string>

class MyPass {
public:
    bool initialize(const std::string& vertexShader,
        const std::string& fragmentShader);

    void render(GLuint inputTexture, int width, int height);

private:
    Shader shader;
    GLuint emptyVao = 0;
};
```

Then implement the OpenGL setup and draw call in `MyPass.cpp`:

```cpp
#include "MyPass.h"

bool MyPass::initialize(const std::string& vertexShader,
    const std::string& fragmentShader)
{
    shader = Shader(vertexShader.c_str(), fragmentShader.c_str());
    glGenVertexArrays(1, &emptyVao);
    return true;
}

void MyPass::render(GLuint inputTexture, int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);

    shader.bind();
    shader.setUniform1i("InputTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    glBindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}
```

For a geometry pass, replace the fullscreen triangle with model drawing and
attach the pass-owned framebuffer before rendering.

### 2. Add the shaders

Put the shaders in `Assets/Shaders/`, then initialize the pass from `main.cpp`:

```cpp
MyPass myPass;
myPass.initialize(
    AssetPath(root, "my_pass.vert").string(),
    AssetPath(root, "my_pass.frag").string());
```

`AssetPath()` resolves shader files relative to the DouStyle project directory.

### 3. Insert the pass into the frame

Passes are executed explicitly in `main.cpp`, so place the new call after the
passes that produce its inputs:

```cpp
glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "MyPass");
myPass.render(
    gBufferPass.getAlbedoMetallicTexture(),
    WindowWidth,
    WindowHeight);
glPopDebugGroup();
```

Typical ordering is:

```text
ShadowPass
    -> GBufferPass
    -> VoxelizationPass
    -> MyPass
    -> DeferredLightingPass
```

Passes should receive textures or data from earlier passes through their public
getters instead of reaching into another pass's private OpenGL state.

### 4. Register files in Visual Studio

Add the new `.cpp` file to the `ClCompile` section of `DouStyle.vcxproj`, the
`.h` file to the `ClInclude` section, and add both files to
`DouStyle.vcxproj.filters` if you want them to appear in the correct Solution
Explorer folder. Shader files belong in the `None` section of the project file.

After that, rebuild `DouStyle.sln`. Use RenderDoc markers around the new pass
so its GPU work is easy to find in the Event Browser.

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
