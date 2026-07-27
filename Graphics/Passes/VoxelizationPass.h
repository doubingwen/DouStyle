#pragma once

#include "../Model.h"
#include "../Shader.h"
#include "../Light.h"
#include "ShadowPass.h"

#include <GL/glew.h>
#include <glm.hpp>

#include <string>

class VoxelizationPass {
public:
    ~VoxelizationPass();
    bool initialize(const std::string& vertexShader, const std::string& geometryShader,
        const std::string& fragmentShader, const std::string& resolveShader,
        int resolution = 512, float totalSize = 150.0f);
    void render(Model& model, const glm::mat4& modelMatrix, const ShadowPass& shadowPass,
        const Light& light);
    GLuint getVoxelTexture() const { return voxelTexture; }
    int getResolution() const { return resolution; }
    float getTotalSize() const { return totalSize; }

private:
    void destroy();
    int resolution = 512;
    float totalSize = 150.0f;
    GLuint voxelTexture = 0;
    // Separate atomic accumulation buffers: R, G, B and sample count.
    GLuint voxelSumR = 0;
    GLuint voxelSumG = 0;
    GLuint voxelSumB = 0;
    GLuint voxelCount = 0;

    // Kept for comparison with the packed RGBA8 + CAS accumulation path.
    GLuint packedVoxelTexture = 0;
    Shader shader;
    Shader resolveShader;
};
