#include "Material.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture2D loadTexture2D(std::string path, bool srgb)
{
	Texture2D texture;

	const char* filename = path.c_str();
	GLubyte* textureData = stbi_load(filename, &texture.width, &texture.height, &texture.componentsPerPixel, 0);

	if (!textureData)
	{
		std::cout << "Couldn't load image: " << filename << std::endl;
		return texture;
	}

	glGenTextures(1, &texture.textureID);
	glBindTexture(GL_TEXTURE_2D, texture.textureID);

	if (texture.componentsPerPixel == 4)
	{
		glTexImage2D(GL_TEXTURE_2D,
			0,
			srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8,
			texture.width,
			texture.height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			textureData);
	}
	else if (texture.componentsPerPixel == 3)
	{
		glTexImage2D(GL_TEXTURE_2D,
			0,
			srgb ? GL_SRGB8 : GL_RGB8,
			texture.width,
			texture.height,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			textureData);
	}
	else if (texture.componentsPerPixel == 1)
	{
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RED,
			texture.width,
			texture.height,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			textureData);
	}

	if (texture.componentsPerPixel == 4 || texture.componentsPerPixel == 3 || texture.componentsPerPixel == 1)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	GLenum glError = glGetError();
	if (glError)
		std::cout << "Error loading texture: " << path << std::endl;

	stbi_image_free(textureData);

	return texture;
}


Material::Material()
	: ambientColor(1.0f), diffuseColor(1.0f), specularColor(1.0f),
	  shininess(32.0f), opacity(1.0f), metallicFactor(0.0f), roughnessFactor(1.0f)
{
}

void Material::loadFromAssimp(aiMaterial* assimpMaterial, std::string directory)
{
	aiString matName;
	assimpMaterial->Get(AI_MATKEY_NAME,matName);
	name = matName.data;

	aiColor3D color;
	if (assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
		ambientColor = glm::vec3(color.r, color.g, color.b);
	if (assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
		specularColor = glm::vec3(color.r, color.g, color.b);
	if (assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
		diffuseColor = glm::vec3(color.r, color.g, color.b);

	aiColor4D baseColor;
	if (assimpMaterial->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
		diffuseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);

	assimpMaterial->Get(AI_MATKEY_SHININESS, shininess);
	assimpMaterial->Get(AI_MATKEY_OPACITY, opacity);
	assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor);
	assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor);

	auto loadTexture = [&](aiTextureType type, Texture2D& destination, bool srgb = false) {
		if (assimpMaterial->GetTextureCount(type) == 0) return false;
		aiString texturePath;
		if (assimpMaterial->GetTexture(type, 0, &texturePath) != AI_SUCCESS) return false;
		destination = loadTexture2D(directory + texturePath.data, srgb);
		return destination.textureID != 0;
	};

	if (!loadTexture(aiTextureType_BASE_COLOR, diffuseTexture, true))
		loadTexture(aiTextureType_DIFFUSE, diffuseTexture, true);

	if (assimpMaterial->GetTextureCount(aiTextureType_AMBIENT) > 0)
	{
		aiString texturePath;
		if (assimpMaterial->GetTexture(aiTextureType_AMBIENT, 0, &texturePath) == AI_SUCCESS) {
			std::string fullPath = directory + texturePath.data;
			specularTexture = loadTexture2D(fullPath);
		}
	}

	if (assimpMaterial->GetTextureCount(aiTextureType_HEIGHT) > 0)
	{
		aiString texturePath;
		if (assimpMaterial->GetTexture(aiTextureType_HEIGHT, 0, &texturePath) == AI_SUCCESS) {
			std::string fullPath = directory + texturePath.data;
			heightTexture = loadTexture2D(fullPath);
		}
	}

	loadTexture(aiTextureType_NORMALS, normalTexture);
	if (!loadTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, metallicRoughnessTexture) &&
		!loadTexture(aiTextureType_METALNESS, metallicRoughnessTexture))
		loadTexture(aiTextureType_DIFFUSE_ROUGHNESS, metallicRoughnessTexture);
	loadTexture(aiTextureType_EMISSIVE, emissionTexture, true);

	if (assimpMaterial->GetTextureCount(aiTextureType_OPACITY) > 0)
	{
		aiString texturePath;
		if (assimpMaterial->GetTexture(aiTextureType_OPACITY, 0, &texturePath) == AI_SUCCESS) {
			std::string fullPath = directory + texturePath.data;
			maskTexture = loadTexture2D(fullPath);
		}
	}
}

void Material::bind(Shader& shader)
{
	shader.bind();

	shader.setUniform1f("Shininess", shininess);
	shader.setUniform1f("Opacity", opacity);
	shader.setUniform1f("MetallicFactor", metallicFactor);
	shader.setUniform1f("RoughnessFactor", roughnessFactor);
	shader.setUniform3f("BaseColorFactor", diffuseColor.r, diffuseColor.g, diffuseColor.b);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseTexture.textureID);
	shader.setUniform1i("DiffuseTexture", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularTexture.textureID);
	shader.setUniform1i("SpecularTexture", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, heightTexture.textureID);
	shader.setUniform1i("HeightTexture", 2);
	shader.setUniform2f("HeightTextureSize", heightTexture.width, heightTexture.height);

	// PBR-only bindings. Units 3-5 remain reserved for the existing shadow/VXGI path.
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, normalTexture.textureID);
	shader.setUniform1i("NormalTexture", 6);
	shader.setUniform1i("HasNormalTexture", normalTexture.textureID != 0 ? 1 : 0);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, metallicRoughnessTexture.textureID);
	shader.setUniform1i("MetallicRoughnessTexture", 7);
	shader.setUniform1i("HasMetallicRoughnessTexture", metallicRoughnessTexture.textureID != 0 ? 1 : 0);

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, emissionTexture.textureID);
	shader.setUniform1i("EmissionTexture", 8);
	shader.setUniform1i("HasEmissionTexture", emissionTexture.textureID != 0 ? 1 : 0);
}
