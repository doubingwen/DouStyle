#pragma once

#include <iostream>

#include <GL/glew.h>
#include <glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Shader.h"

struct Texture2D {
	GLuint textureID = 0;
	int width = 1;
	int height = 1;
	int componentsPerPixel = 4;
};

struct Texture3D {
	GLuint textureID;
	int size;
	int componentsPerPixel;
};

Texture2D loadTexture2D(std::string path, bool srgb = false);

class Material {
public:
	Material();

	void loadFromAssimp(aiMaterial* assimpMaterial, std::string directory);

	void bind(Shader& shader);

	std::string name;

	glm::vec3 ambientColor;
	glm::vec3 diffuseColor;
	glm::vec3 specularColor;
	float shininess;
	float opacity;
	float metallicFactor;
	float roughnessFactor;

	Texture2D diffuseTexture;
	Texture2D specularTexture;
	Texture2D maskTexture;
	Texture2D heightTexture;
	Texture2D normalTexture;
	Texture2D metallicRoughnessTexture;
	Texture2D emissionTexture;
};
