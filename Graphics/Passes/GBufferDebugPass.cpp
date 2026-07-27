#include "GBufferDebugPass.h"

#include "../DebugMarkers.h"

GBufferDebugPass::~GBufferDebugPass()
{
    if (emptyVao != 0) glDeleteVertexArrays(1, &emptyVao);
}

bool GBufferDebugPass::initialize(const std::string& vertexShader, const std::string& fragmentShader)
{
    shader = Shader(vertexShader.c_str(), fragmentShader.c_str());
    glGenVertexArrays(1, &emptyVao);
    DebugMarkers::Label(GL_VERTEX_ARRAY, emptyVao, "Dou GBuffer Debug VAO");
    return true;
}

void GBufferDebugPass::render(const GBufferPass& gBuffer, int mode, int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.bind();
    shader.setUniform1i("DebugMode", mode);
    shader.setUniform1i("GPosition", 0);
    shader.setUniform1i("GNormalRoughness", 1);
    shader.setUniform1i("GAlbedoMetallic", 2);
    shader.setUniform1i("GEmission", 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getPositionTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getNormalRoughnessTexture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getAlbedoMetallicTexture());
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getEmissionTexture());

    glBindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}
