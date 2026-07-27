#include "ShadowPass.h"

#include "../DebugMarkers.h"

#include <gtc/matrix_transform.hpp>

#include <iostream>

ShadowPass::~ShadowPass()
{
    destroy();
}

bool ShadowPass::initialize(const std::string& vertexShader, const std::string& fragmentShader,
    const glm::vec3& lightDirection, int inResolution)
{
    resolution = inResolution;
    shader = Shader(vertexShader.c_str(), fragmentShader.c_str());

    // Keep the original ZJU light-space construction unchanged.
    const glm::mat4 lightView = glm::lookAt(lightDirection, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, -150.0f, 50.0f);
    lightSpaceMatrix = lightProjection * lightView;

    glGenFramebuffers(1, &framebuffer);
    DebugMarkers::Label(GL_FRAMEBUFFER, framebuffer, "ZJU ShadowPass FBO");
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenTextures(1, &depthTexture);
    DebugMarkers::Label(GL_TEXTURE, depthTexture, "ZJU ShadowMap Depth");
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    glGenTextures(1, &depthColorTexture);
    DebugMarkers::Label(GL_TEXTURE, depthColorTexture, "ZJU ShadowMap Encoded Depth");
    glBindTexture(GL_TEXTURE_2D, depthColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, resolution, resolution, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthColorTexture, 0);

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!complete) std::cerr << "Error creating shadow map framebuffer\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

void ShadowPass::render(Model& model, const glm::mat4& modelMatrix)
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, resolution, resolution);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    model.draw(shader, modelMatrix, lightSpaceMatrix);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::destroy()
{
    if (depthColorTexture != 0) glDeleteTextures(1, &depthColorTexture);
    if (depthTexture != 0) glDeleteTextures(1, &depthTexture);
    if (framebuffer != 0) glDeleteFramebuffers(1, &framebuffer);
}
