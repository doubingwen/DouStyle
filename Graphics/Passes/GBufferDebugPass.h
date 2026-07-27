#pragma once

#include "GBufferPass.h"
#include "../Shader.h"

#include <string>

class GBufferDebugPass {
public:
    ~GBufferDebugPass();

    bool initialize(const std::string& vertexShader, const std::string& fragmentShader);
    void render(const GBufferPass& gBuffer, int mode, int width, int height);

private:
    Shader shader;
    GLuint emptyVao = 0;
};
