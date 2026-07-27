#version 430 core

in vec2 TexCoord;
out vec4 Color;

uniform sampler2D GPosition;
uniform sampler2D GNormalRoughness;
uniform sampler2D GAlbedoMetallic;
uniform sampler2D GEmission;
uniform int DebugMode;

void main()
{
    vec4 position = texture(GPosition, TexCoord);
    vec4 normalRoughness = texture(GNormalRoughness, TexCoord);
    vec4 albedoMetallic = texture(GAlbedoMetallic, TexCoord);
    vec4 emission = texture(GEmission, TexCoord);

    if (DebugMode == 0) {
        Color = vec4(position.xyz / 75.0 + 0.5, 1.0);
    } else if (DebugMode == 1) {
        Color = vec4(normalRoughness.rgb, 1.0);
    } else if (DebugMode == 2) {
        Color = vec4(albedoMetallic.rgb, 1.0);
    } else if (DebugMode == 3) {
        Color = vec4(vec3(normalRoughness.a), 1.0);
    } else if (DebugMode == 4) {
        Color = vec4(vec3(albedoMetallic.a), 1.0);
    } else {
        Color = vec4(emission.rgb, 1.0);
    }
}
