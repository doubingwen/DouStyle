#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <iostream>

bool Model::load(const std::string& modelPath, float scale)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        modelPath,
        aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
    if (scene == nullptr) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << "\n";
        return false;
    }

    std::string normalizedPath = modelPath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    const std::string directory = normalizedPath.substr(0, normalizedPath.find_last_of('/') + 1);
    materials.reserve(scene->mNumMaterials);
    meshes.reserve(scene->mNumMeshes);

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        Material material;
        material.loadFromAssimp(scene->mMaterials[i], directory);
        materials.push_back(material);
    }
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        Mesh mesh;
        mesh.loadFromAssimp(scene->mMeshes[i]);
        mesh.material = &materials[scene->mMeshes[i]->mMaterialIndex];
        mesh.scale = scale;
        meshes.push_back(mesh);
    }
    return !meshes.empty();
}

void Model::draw(Shader& shader, const glm::mat4& modelMatrix, const glm::mat4& lightSpaceMatrix)
{
    shader.bind();
    shader.setUniformMatrix4fv("ModelMatrix", modelMatrix);
    shader.setUniformMatrix4fv("LightModelViewProjectionMatrix", lightSpaceMatrix * modelMatrix);
    for (Mesh& mesh : meshes) {
        mesh.material->bind(shader);
        mesh.draw();
    }
}
