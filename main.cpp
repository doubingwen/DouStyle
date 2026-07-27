#include "Core/Application.h"
#include "Graphics/Camera.h"
#include "Graphics/DebugMarkers.h"
#include "Graphics/Model.h"
#include "Graphics/Passes/GBufferPass.h"
#include "Graphics/Passes/GBufferDebugPass.h"
#include "Graphics/Passes/DeferredLightingPass.h"
#include "Graphics/Passes/LightingPass.h"
#include "Graphics/Passes/ShadowPass.h"
#include "Graphics/Passes/VoxelDebugPass.h"
#include "Graphics/Passes/VoxelizationPass.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <filesystem>
#include <iostream>

namespace {

constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;

struct SceneState {
    Camera* camera = nullptr;
    bool showDirect = true;
    bool showDiffuse = true;
    bool showSpecular = true;
    bool showAo = false;
    bool showVoxels = false;
    bool showGBuffer = false;
    int gBufferMode = 0;
    bool useDeferred = true;
};

SceneState* g_state = nullptr;

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (action != GLFW_PRESS || g_state == nullptr) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_1) g_state->showDirect = !g_state->showDirect;
    if (key == GLFW_KEY_2) g_state->showDiffuse = !g_state->showDiffuse;
    if (key == GLFW_KEY_3) g_state->showSpecular = !g_state->showSpecular;
    if (key == GLFW_KEY_4) g_state->showAo = !g_state->showAo;
    if (key == GLFW_KEY_5) g_state->showVoxels = !g_state->showVoxels;
    if (key == GLFW_KEY_6) g_state->showGBuffer = !g_state->showGBuffer;
    if (key == GLFW_KEY_7) g_state->gBufferMode = (g_state->gBufferMode + 1) % 6;
    if (key == GLFW_KEY_8) g_state->useDeferred = !g_state->useDeferred;
}

void UpdateCamera(GLFWwindow* window, Camera& camera, float deltaTime)
{
    static bool firstMouse = true;
    static double lastX = 0.0;
    static double lastY = 0.0;
    if (firstMouse) {
        glfwGetCursorPos(window, &lastX, &lastY);
        firstMouse = false;
    }

    double currentX = 0.0;
    double currentY = 0.0;
    glfwGetCursorPos(window, &currentX, &currentY);
    camera.turnAround(static_cast<float>(currentX - lastX) * 0.001f,
        static_cast<float>(lastY - currentY) * 0.001f);
    lastX = currentX;
    lastY = currentY;

    const float movement = deltaTime * 2.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(movement);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.moveBackward(movement);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.moveRight(movement);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.moveLeft(movement);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.moveUp(movement);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.moveDown(movement);
}

std::filesystem::path AssetPath(const std::filesystem::path& root, const char* path)
{
    return (root / "Assets" / "Shaders" / path).lexically_normal();
}

const char* GBufferModeName(int mode)
{
    switch (mode) {
    case 0: return "Position";
    case 1: return "Normal";
    case 2: return "Albedo";
    case 3: return "Roughness";
    case 4: return "Metallic";
    case 5: return "Emission";
    default: return "Unknown";
    }
}

}

int main()
{
    Application application(WindowWidth, WindowHeight, "ZJU VXGI - DouStyle");
    if (!application.initialize()) return 1;

    const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path();
    const glm::vec3 lightDirection(-0.3f, 0.9f, -0.25f);
    const glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

    Camera camera(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        0.0f, 0.0f, 45.0f, static_cast<float>(WindowWidth) / WindowHeight, 0.1f, 1000.0f);
    SceneState state;
    state.camera = &camera;
    g_state = &state;
    glfwSetKeyCallback(application.getWindow(), KeyCallback);

    Model model;
    const auto modelPath = root.parent_path() / "VXGI-Renderer" / "model" /
        "sponza_pbr" / "glTF" / "Sponza.gltf";
    if (!model.load(modelPath.string(), 0.05f)) {
        std::cerr << "Failed to load model: " << modelPath << "\n";
        return 1;
    }

    ShadowPass shadowPass;
    if (!shadowPass.initialize(AssetPath(root, "shadowMap.vert").string(),
        AssetPath(root, "shadowMap.frag").string(), lightDirection)) return 1;

    VoxelizationPass voxelizationPass;
    if (!voxelizationPass.initialize(AssetPath(root, "voxelization.vert").string(),
        AssetPath(root, "voxelization.geom").string(), AssetPath(root, "voxelization.frag").string(),
        AssetPath(root, "voxel_resolve.comp").string(),
        128, 150.0f)) return 1;

    VoxelDebugPass voxelDebugPass;
    if (!voxelDebugPass.initialize(AssetPath(root, "voxelVisualization.vert").string(),
        AssetPath(root, "voxelVisualization.geom").string(), AssetPath(root, "voxelVisualization.frag").string())) return 1;

    GBufferPass gBufferPass;
    if (!gBufferPass.initialize(AssetPath(root, "gbuffer.vert").string(),
        AssetPath(root, "gbuffer.frag").string(), WindowWidth, WindowHeight)) return 1;

    GBufferDebugPass gBufferDebugPass;
    if (!gBufferDebugPass.initialize(AssetPath(root, "gbuffer_debug.vert").string(),
        AssetPath(root, "gbuffer_debug.frag").string())) return 1;

    LightingPass lightingPass;
    if (!lightingPass.initialize(AssetPath(root, "render.vert").string(),
        AssetPath(root, "render.frag").string())) return 1;

    DeferredLightingPass deferredLightingPass;
    if (!deferredLightingPass.initialize(AssetPath(root, "deferred_lighting.vert").string(),
        AssetPath(root, "deferred_lighting.frag").string())) return 1;

    double lastTime = glfwGetTime();
    double fpsTimer = 0.0;
    int frameCount = 0;
    while (!glfwWindowShouldClose(application.getWindow())) {
        glfwPollEvents();
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime       ;
        fpsTimer += deltaTime;
        ++frameCount;
        if (fpsTimer >= 0.5) {
            const int fps = static_cast<int>(static_cast<double>(frameCount) / fpsTimer);
            std::string title = "VXGI - DouStyle | FPS: " + std::to_string(fps);
            if (state.showVoxels) title += " | Voxel Debug";
            if (state.showGBuffer) {
                title += " | GBuffer Debug | ";
                title += GBufferModeName(state.gBufferMode);
            }
            if (!state.showGBuffer && !state.showVoxels) {
                title += state.useDeferred ? " | Deferred PBR + VXGI" : " | Forward render.frag";
            }
            glfwSetWindowTitle(application.getWindow(), title.c_str());
            fpsTimer = 0.0;
            frameCount = 0;
        }
        UpdateCamera(application.getWindow(), camera, deltaTime);

        DebugMarkers::Push("Frame");
        DebugMarkers::Push("ShadowPass");
        shadowPass.render(model, modelMatrix);
        DebugMarkers::Pop();

        DebugMarkers::Push("GBufferPass");
        gBufferPass.render(model, modelMatrix, camera);
        DebugMarkers::Pop();

        DebugMarkers::Push("VoxelizationPass");
        voxelizationPass.render(model, modelMatrix, shadowPass, lightDirection);
        DebugMarkers::Pop();

        if (state.showGBuffer) {
            DebugMarkers::Push("GBufferDebugPass");
            gBufferDebugPass.render(gBufferPass, state.gBufferMode, WindowWidth, WindowHeight);
            DebugMarkers::Pop();
        } else if (state.showVoxels) {
            DebugMarkers::Push("VoxelDebugPass");
            voxelDebugPass.render(camera, voxelizationPass, WindowWidth, WindowHeight);
            DebugMarkers::Pop();
        } else {
            if (state.useDeferred) {
                DebugMarkers::Push("DeferredLightingPass");
                deferredLightingPass.render(gBufferPass, camera, shadowPass, voxelizationPass,
                    lightDirection, state.showDirect, state.showDiffuse,
                    state.showSpecular, state.showAo);
                DebugMarkers::Pop();
            } else {
                DebugMarkers::Push("ForwardLightingPass");
                lightingPass.render(model, modelMatrix, camera, shadowPass, voxelizationPass,
                    lightDirection, state.showDirect, state.showDiffuse,
                    state.showSpecular, state.showAo);
                DebugMarkers::Pop();
            }
        }

        DebugMarkers::Pop();

        glfwSwapBuffers(application.getWindow());
    }

    return 0;
}
