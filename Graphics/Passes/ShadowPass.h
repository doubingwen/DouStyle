#pragma once

#include "../Model.h"
#include "../Shader.h"
#include "../Light.h"

#include <GL/glew.h>
#include <glm.hpp>

#include <string>

class ShadowPass {
public:
    ~ShadowPass();
    bool initialize(const std::string& vertexShader, const std::string& fragmentShader,
        const Light& light, int resolution = 4096);
    void updateLight(const Light& light);
    void render(Model& model, const glm::mat4& modelMatrix);
    GLuint getDepthTexture() const { return depthTexture; }
    GLuint getDepthColorTexture() const { return depthColorTexture; }
    const glm::mat4& getLightSpaceMatrix() const { return lightSpaceMatrix; }

private:
    void destroy();
    int resolution = 4096;
    GLuint framebuffer = 0;
    GLuint depthTexture = 0;
    GLuint depthColorTexture = 0;
    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
    Shader shader;
};
