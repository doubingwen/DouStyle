#include "DeferredLightingPass.h"

#include "../DebugMarkers.h"

DeferredLightingPass::~DeferredLightingPass()
{
    if (emptyVao != 0) glDeleteVertexArrays(1, &emptyVao);
}

bool DeferredLightingPass::initialize(const std::string& vertexShader, const std::string& fragmentShader)
{
    shader = Shader(vertexShader.c_str(), fragmentShader.c_str());
    glGenVertexArrays(1, &emptyVao);
    DebugMarkers::Label(GL_VERTEX_ARRAY, emptyVao, "Dou Deferred Lighting VAO");
    return true;
}

void DeferredLightingPass::render(const GBufferPass& gBuffer, const Camera& camera,
    const ShadowPass& shadowPass, const VoxelizationPass& voxelPass,
    const Light& light, bool showDirect, bool showIndirect,
    bool showSpecular, bool showAo)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1280, 720);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.bind();
    const glm::vec3 cameraPosition = camera.getPosition();
    shader.setUniform3f("CameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    const glm::vec3 lightDirection = light.getDirection();
    shader.setUniform3f("ToLightDirection", lightDirection.x, lightDirection.y, lightDirection.z);
    shader.setUniformMatrix4fv("LightSpaceMatrix", shadowPass.getLightSpaceMatrix());
    shader.setUniform1i("VoxelDimensions", voxelPass.getResolution());
    shader.setUniform1f("VoxelTotalSize", voxelPass.getTotalSize());
    shader.setUniform1i("ShowDirect", showDirect ? 1 : 0);
    shader.setUniform1i("ShowIndirect", showIndirect ? 1 : 0);
    shader.setUniform1i("ShowSpecular", showSpecular ? 1 : 0);
    shader.setUniform1i("ShowAo", showAo ? 1 : 0);

    shader.setUniform1i("GPosition", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getPositionTexture());
    shader.setUniform1i("GNormalRoughness", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getNormalRoughnessTexture());
    shader.setUniform1i("GAlbedoMetallic", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getAlbedoMetallicTexture());
    shader.setUniform1i("GEmission", 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getEmissionTexture());

    shader.setUniform1i("ShadowMap", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shadowPass.getDepthTexture());
    shader.setUniform1i("VoxelTexture", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_3D, voxelPass.getVoxelTexture());

    glBindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}
