#pragma once

#include "../Camera.h"
#include "../Model.h"
#include "../Shader.h"
#include "ShadowPass.h"
#include "VoxelizationPass.h"

#include <glm.hpp>

#include <string>

class LightingPass {
public:
    bool initialize(const std::string& vertexShader, const std::string& fragmentShader);
    void render(Model& model, const glm::mat4& modelMatrix, const Camera& camera,
        const ShadowPass& shadowPass, const VoxelizationPass& voxelPass,
        const glm::vec3& lightDirection, bool showDirect, bool showDiffuse,
        bool showSpecular, bool showAo);

private:
    Shader shader;
};
