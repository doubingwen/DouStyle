#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();

    bool initialize();
    GLFWwindow* getWindow() const { return window; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    int width;
    int height;
    const char* title;
    GLFWwindow* window = nullptr;
};
