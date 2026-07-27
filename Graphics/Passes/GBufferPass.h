#pragma once

#include "../Camera.h"
#include "../Model.h"
#include "../Shader.h"

#include <GL/glew.h>
#include <glm.hpp>

#include <string>

class GBufferPass {
public:
    ~GBufferPass();

    bool initialize(const std::string& vertexShader, const std::string& fragmentShader,
        int width, int height);
    void render(Model& model, const glm::mat4& modelMatrix, const Camera& camera);

    GLuint getPositionTexture() const { return positionTexture; }
    GLuint getNormalRoughnessTexture() const { return normalRoughnessTexture; }
    GLuint getAlbedoMetallicTexture() const { return albedoMetallicTexture; }
    GLuint getEmissionTexture() const { return emissionTexture; }

private:
    void destroy();

    Shader shader;
    GLuint framebuffer = 0;
    GLuint depthRenderbuffer = 0;
    GLuint positionTexture = 0;
    GLuint normalRoughnessTexture = 0;
    GLuint albedoMetallicTexture = 0;
    GLuint emissionTexture = 0;
    int width = 0;
    int height = 0;
};
