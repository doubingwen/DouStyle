#include "VoxelizationPass.h"

#include "../DebugMarkers.h"

#include <gtc/matrix_transform.hpp>

#include <vector>

VoxelizationPass::~VoxelizationPass()
{
    destroy();
}

bool VoxelizationPass::initialize(const std::string& vertexShader, const std::string& geometryShader,
    const std::string& fragmentShader, const std::string& resolveShaderPath,
    int inResolution, float inTotalSize)
{
    resolution = inResolution;
    totalSize = inTotalSize;
    shader = Shader(vertexShader.c_str(), fragmentShader.c_str(), geometryShader.c_str());
    if (!resolveShader.loadCompute(resolveShaderPath.c_str())) return false;

    glGenTextures(1, &voxelTexture);
    DebugMarkers::Label(GL_TEXTURE, voxelTexture, "ZJU Voxel Texture RGBA8");
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    std::vector<GLubyte> data(static_cast<size_t>(resolution) * resolution * resolution * 4, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, resolution, resolution, resolution, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);

    GLuint accumulationTextures[4] = {};
    const char* accumulationLabels[] = {
        "ZJU Voxel Sum R",
        "ZJU Voxel Sum G",
        "ZJU Voxel Sum B",
        "ZJU Voxel Sample Count"
    };
    glGenTextures(4, accumulationTextures);
    voxelSumR = accumulationTextures[0];
    voxelSumG = accumulationTextures[1];
    voxelSumB = accumulationTextures[2];
    voxelCount = accumulationTextures[3];
    for (int i = 0; i < 4; ++i) {
        glBindTexture(GL_TEXTURE_3D, accumulationTextures[i]);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, resolution, resolution, resolution);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        DebugMarkers::Label(GL_TEXTURE, accumulationTextures[i], accumulationLabels[i]);
    }
    glBindTexture(GL_TEXTURE_3D, 0);

    // The packed CAS texture remains allocated so the old path can be restored easily.
    glGenTextures(1, &packedVoxelTexture);
    glBindTexture(GL_TEXTURE_3D, packedVoxelTexture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, resolution, resolution, resolution);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    DebugMarkers::Label(GL_TEXTURE, packedVoxelTexture, "ZJU Packed Atomic Voxel RGBA8 Average");
    glBindTexture(GL_TEXTURE_3D, 0);
    return true;
}

void VoxelizationPass::render(Model& model, const glm::mat4& modelMatrix,
    const ShadowPass& shadowPass, const glm::vec3& lightDirection)
{
    const float size = totalSize;
    const glm::mat4 projection = glm::ortho(-size * 0.5f, size * 0.5f,
        -size * 0.5f, size * 0.5f, size * 0.5f, size * 1.5f);
    const glm::mat4 projectionX = projection * glm::lookAt(
        glm::vec3(size, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 projectionY = projection * glm::lookAt(
        glm::vec3(0.0f, size, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    const glm::mat4 projectionZ = projection * glm::lookAt(
        glm::vec3(0.0f, 0.0f, size), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, resolution, resolution);
    const GLuint clearValue[] = { 0 };
    glClearTexImage(voxelSumR, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, clearValue);
    glClearTexImage(voxelSumG, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, clearValue);
    glClearTexImage(voxelSumB, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, clearValue);
    glClearTexImage(voxelCount, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, clearValue);
    glBindImageTexture(0, voxelSumR, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(1, voxelSumG, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(2, voxelSumB, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(3, voxelCount, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

    shader.bind();
    shader.setUniformMatrix4fv("ProjectionFromXAxis", projectionX);
    shader.setUniformMatrix4fv("ProjectionFromYAxis", projectionY);
    shader.setUniformMatrix4fv("ProjectionFromZAxis", projectionZ);
    shader.setUniform1i("VoxelDimensions", resolution);
    shader.setUniform3f("ToLightDirection", lightDirection.x, lightDirection.y, lightDirection.z);
    shader.setUniform1i("ShadowMap", 5);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowPass.getDepthTexture());
    model.draw(shader, modelMatrix, shadowPass.getLightSpaceMatrix());

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    resolveShader.bind();
    resolveShader.setUniform1i("VoxelSumR", 1);
    resolveShader.setUniform1i("VoxelSumG", 2);
    resolveShader.setUniform1i("VoxelSumB", 3);
    resolveShader.setUniform1i("VoxelCount", 4);
    resolveShader.setUniform1i("VoxelDimensions", resolution);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, voxelSumR);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, voxelSumG);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, voxelSumB);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_3D, voxelCount);
    glBindImageTexture(0, voxelTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glDispatchCompute(static_cast<GLuint>((resolution + 7) / 8),
        static_cast<GLuint>((resolution + 7) / 8),
        static_cast<GLuint>((resolution + 7) / 8));
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(1, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(2, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(3, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glGenerateMipmap(GL_TEXTURE_3D);
    glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
}

void VoxelizationPass::destroy()
{
    if (voxelTexture != 0) glDeleteTextures(1, &voxelTexture);
    if (voxelSumR != 0) glDeleteTextures(1, &voxelSumR);
    if (voxelSumG != 0) glDeleteTextures(1, &voxelSumG);
    if (voxelSumB != 0) glDeleteTextures(1, &voxelSumB);
    if (voxelCount != 0) glDeleteTextures(1, &voxelCount);
    if (packedVoxelTexture != 0) glDeleteTextures(1, &packedVoxelTexture);
    voxelTexture = 0;
    voxelSumR = 0;
    voxelSumG = 0;
    voxelSumB = 0;
    voxelCount = 0;
    packedVoxelTexture = 0;
}
