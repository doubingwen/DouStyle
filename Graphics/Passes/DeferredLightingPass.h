#pragma once

#include "GBufferPass.h"
#include "ShadowPass.h"
#include "VoxelizationPass.h"
#include "../Camera.h"
#include "../Light.h"
#include "../Shader.h"

#include <glm.hpp>

#include <string>

class DeferredLightingPass {
public:
    ~DeferredLightingPass();

    bool initialize(const std::string& vertexShader, const std::string& fragmentShader);
    void render(const GBufferPass& gBuffer, const Camera& camera,
        const ShadowPass& shadowPass, const VoxelizationPass& voxelPass,
        const Light& light, bool showDirect, bool showIndirect,
        bool showSpecular, bool showAo);

private:
    Shader shader;
    GLuint emptyVao = 0;
};
