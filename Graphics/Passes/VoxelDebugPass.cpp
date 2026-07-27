#include "VoxelDebugPass.h"

#include "../DebugMarkers.h"

VoxelDebugPass::~VoxelDebugPass()
{
    if (emptyVao != 0) glDeleteVertexArrays(1, &emptyVao);
}

bool VoxelDebugPass::initialize(const std::string& vertexShader, const std::string& geometryShader,
    const std::string& fragmentShader)
{
    shader = Shader(vertexShader.c_str(), fragmentShader.c_str(), geometryShader.c_str());
    glGenVertexArrays(1, &emptyVao);
    DebugMarkers::Label(GL_VERTEX_ARRAY, emptyVao, "Dou Voxel Debug VAO");
    return true;
}

void VoxelDebugPass::render(const Camera& camera, const VoxelizationPass& voxelPass,
    int width, int height)
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.bind();
    shader.setUniformMatrix4fv("ViewMatrix", camera.getViewMatrix());
    shader.setUniformMatrix4fv("ProjectionMatrix", camera.getProjectionMatrix());
    shader.setUniform1i("VoxelDimensions", voxelPass.getResolution());
    shader.setUniform1f("VoxelTotalSize", voxelPass.getTotalSize());
    shader.setUniform1i("VoxelTexture", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_3D, voxelPass.getVoxelTexture());

    glBindVertexArray(emptyVao);
    const int count = voxelPass.getResolution() * voxelPass.getResolution() * voxelPass.getResolution();
    glDrawArrays(GL_POINTS, 0, count);
    glBindVertexArray(0);
}
