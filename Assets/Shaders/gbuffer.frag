#version 430 core

in vec3 PositionWorld;
in vec3 NormalWorld;
in vec3 TangentWorld;
in vec3 BitangentWorld;
in vec2 TexCoord;

layout(location = 0) out vec4 GPosition;
layout(location = 1) out vec4 GNormalRoughness;
layout(location = 2) out vec4 GAlbedoMetallic;
layout(location = 3) out vec4 GEmission;

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalTexture;
uniform sampler2D MetallicRoughnessTexture;
uniform sampler2D EmissionTexture;
uniform float Shininess;
uniform float Opacity;
uniform float MetallicFactor;
uniform float RoughnessFactor;
uniform vec3 BaseColorFactor;
uniform int HasNormalTexture;
uniform int HasMetallicRoughnessTexture;
uniform int HasEmissionTexture;

void main()
{
    vec4 albedo = texture(DiffuseTexture, TexCoord);

    vec3 normal = normalize(NormalWorld);
    if (HasNormalTexture > 0) {
        vec3 tangent = normalize(TangentWorld - normal * dot(normal, TangentWorld));
        vec3 bitangent = normalize(cross(normal, tangent));
        vec3 tangentNormal = texture(NormalTexture, TexCoord).xyz * 2.0 - 1.0;
        normal = normalize(mat3(tangent, bitangent, normal) * tangentNormal);
    }

    vec4 metallicRoughness = texture(MetallicRoughnessTexture, TexCoord);
    float roughness = HasMetallicRoughnessTexture > 0
        ? metallicRoughness.g * RoughnessFactor
        : clamp(sqrt(2.0 / max(Shininess + 2.0, 2.0)), 0.04, 1.0);
    float metallic = HasMetallicRoughnessTexture > 0
        ? metallicRoughness.b * MetallicFactor
        : MetallicFactor;
    vec3 emission = HasEmissionTexture > 0 ? texture(EmissionTexture, TexCoord).rgb : vec3(0.0);

    GPosition = vec4(PositionWorld, 1.0);
    GNormalRoughness = vec4(normal * 0.5 + 0.5, clamp(roughness, 0.04, 1.0));
    // Match render.frag's indirect-light input: use sampled diffuse directly.
    GAlbedoMetallic = vec4(albedo.rgb, metallic);
    GEmission = vec4(emission, 1.0);
}
