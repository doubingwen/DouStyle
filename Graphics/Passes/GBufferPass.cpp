#include "GBufferPass.h"

#include "../DebugMarkers.h"

#include <array>
#include <iostream>

GBufferPass::~GBufferPass()
{
    destroy();
}

bool GBufferPass::initialize(const std::string& vertexShader, const std::string& fragmentShader,
    int inWidth, int inHeight)
{
    width = inWidth;
    height = inHeight;
    shader = Shader(vertexShader.c_str(), fragmentShader.c_str());

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    DebugMarkers::Label(GL_FRAMEBUFFER, framebuffer, "Dou GBuffer Framebuffer");

    GLuint* colorTexturePointers[] = {
        &positionTexture,
        &normalRoughnessTexture,
        &albedoMetallicTexture,
        &emissionTexture
    };
    const char* labels[] = {
        "Dou GBuffer Position",
        "Dou GBuffer Normal Roughness",
        "Dou GBuffer Albedo Metallic",
        "Dou GBuffer Emission"
    };

    GLuint textureIds[4] = {};
    glGenTextures(4, textureIds);
    positionTexture = textureIds[0];
    normalRoughnessTexture = textureIds[1];
    albedoMetallicTexture = textureIds[2];
    emissionTexture = textureIds[3];
    for (int i = 0; i < 4; ++i) {
        glBindTexture(GL_TEXTURE_2D, *colorTexturePointers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
            GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
            GL_TEXTURE_2D, *colorTexturePointers[i], 0);
        DebugMarkers::Label(GL_TEXTURE, *colorTexturePointers[i], labels[i]);
    }

    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, depthRenderbuffer);
    DebugMarkers::Label(GL_RENDERBUFFER, depthRenderbuffer, "Dou GBuffer Depth");

    const std::array<GLenum, 4> drawBuffers = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3
    };
    glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!complete) {
        std::cerr << "GBuffer framebuffer is incomplete: 0x"
            << std::hex << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::dec << "\n";
    }

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

void GBufferPass::render(Model& model, const glm::mat4& modelMatrix, const Camera& camera)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.bind();
    shader.setUniformMatrix4fv("ViewMatrix", camera.getViewMatrix());
    shader.setUniformMatrix4fv("ProjectionMatrix", camera.getProjectionMatrix());
    model.draw(shader, modelMatrix, glm::mat4(1.0f));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBufferPass::destroy()
{
    if (framebuffer != 0) glDeleteFramebuffers(1, &framebuffer);
    if (depthRenderbuffer != 0) glDeleteRenderbuffers(1, &depthRenderbuffer);

    GLuint textures[] = {
        positionTexture,
        normalRoughnessTexture,
        albedoMetallicTexture,
        emissionTexture
    };
    glDeleteTextures(4, textures);

    framebuffer = 0;
    depthRenderbuffer = 0;
    positionTexture = 0;
    normalRoughnessTexture = 0;
    albedoMetallicTexture = 0;
    emissionTexture = 0;
}
