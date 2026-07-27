#pragma once

#include "Material.h"
#include "Mesh.h"

#include <glm.hpp>

#include <string>
#include <vector>

class Model {
public:
    bool load(const std::string& path, float scale);
    void draw(Shader& shader, const glm::mat4& modelMatrix, const glm::mat4& lightSpaceMatrix);
    bool empty() const { return meshes.empty(); }

private:
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
};
