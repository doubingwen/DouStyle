#pragma once

#include "../Camera.h"
#include "../Shader.h"
#include "VoxelizationPass.h"

#include <string>

class VoxelDebugPass {
public:
    ~VoxelDebugPass();
    bool initialize(const std::string& vertexShader, const std::string& geometryShader,
        const std::string& fragmentShader);
    void render(const Camera& camera, const VoxelizationPass& voxelPass, int width, int height);

private:
    Shader shader;
    GLuint emptyVao = 0;
};
