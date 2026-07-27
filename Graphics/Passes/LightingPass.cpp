#include "LightingPass.h"

bool LightingPass::initialize(const std::string& vertexShader, const std::string& fragmentShader)
{
    shader = Shader(vertexShader.c_str(), fragmentShader.c_str());
    return true;
}

void LightingPass::render(Model& model, const glm::mat4& modelMatrix, const Camera& camera,
    const ShadowPass& shadowPass, const VoxelizationPass& voxelPass,
    const glm::vec3& lightDirection, bool showDirect, bool showDiffuse,
    bool showSpecular, bool showAo)
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1280, 720);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.bind();
    const glm::vec3 cameraPosition = camera.getPosition();
    shader.setUniform3f("CameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    shader.setUniform3f("ToLightDirection", lightDirection.x, lightDirection.y, lightDirection.z);
    shader.setUniformMatrix4fv("ViewMatrix", camera.getViewMatrix());
    shader.setUniformMatrix4fv("ProjectionMatrix", camera.getProjectionMatrix());
    shader.setUniform1i("VoxelDimensions", voxelPass.getResolution());
    shader.setUniform1f("VoxelTotalSize", voxelPass.getTotalSize());
    shader.setUniform1i("ShowDirect", showDirect ? 1 : 0);
    shader.setUniform1i("ShowIndirectDiffuse", showDiffuse ? 1 : 0);
    shader.setUniform1i("ShowIndirectSpecular", showSpecular ? 1 : 0);
    shader.setUniform1i("ShowAmbientOcculision", showAo ? 1 : 0);
    shader.setUniformMatrix4fv("LightModelViewProjectionMatrix",
        shadowPass.getLightSpaceMatrix() * modelMatrix);
    shader.setUniform1i("ShadowMap", 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadowPass.getDepthTexture());
    shader.setUniform1i("ShadowMapDepthTexture", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shadowPass.getDepthColorTexture());
    shader.setUniform1i("VoxelTexture", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_3D, voxelPass.getVoxelTexture());

    model.draw(shader, modelMatrix, shadowPass.getLightSpaceMatrix());
}
