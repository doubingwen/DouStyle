#version 430 core

in vec2 TexCoord;
out vec4 Color;

uniform sampler2D GPosition;
uniform sampler2D GNormalRoughness;
uniform sampler2D GAlbedoMetallic;
uniform sampler2D GEmission;
uniform sampler2DShadow ShadowMap;
uniform sampler3D VoxelTexture;

uniform vec3 CameraPosition;
uniform vec3 ToLightDirection;
uniform mat4 LightSpaceMatrix;
uniform int VoxelDimensions;
uniform float VoxelTotalSize;
uniform int ShowDirect;
uniform int ShowIndirect;
uniform int ShowSpecular;
uniform int ShowAo;

const float PI = 3.14159265359;
const float MaxDistance = 100.0;
const int ConeNum = 6;

const vec3 ConeDirections[ConeNum] = vec3[](
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.5, 0.866025),
    vec3(0.823639, 0.5, 0.267617),
    vec3(0.509037, 0.5, -0.700629),
    vec3(-0.509037, 0.5, -0.700629),
    vec3(-0.823639, 0.5, 0.267617)
);

const float ConeWeights[ConeNum] = float[](0.25, 0.15, 0.15, 0.15, 0.15, 0.15);

bool inVoxelVolume(vec3 worldPosition)
{
    vec3 uvw = worldPosition / (VoxelTotalSize * 0.5) * 0.5 + 0.5;
    return all(greaterThanEqual(uvw, vec3(0.0))) && all(lessThanEqual(uvw, vec3(1.0)));
}

vec4 sampleVoxel(vec3 worldPosition, float lod)
{
    vec3 uvw = worldPosition / (VoxelTotalSize * 0.5) * 0.5 + 0.5;
    if (!inVoxelVolume(worldPosition)) return vec4(0.0);

    float border = 0.5 / float(VoxelDimensions);
    return textureLod(VoxelTexture, clamp(uvw, vec3(border), vec3(1.0 - border)), lod);
}

vec4 coneTrace(vec3 origin, vec3 direction, float tanHalfAngle)
{
    float voxelSize = VoxelTotalSize / float(VoxelDimensions);
    vec3 startPosition = origin + direction * voxelSize * 0.5;
    float distance = voxelSize;
    float accumulatedAlpha = 0.0;
    vec3 accumulatedRadiance = vec3(0.0);

    while (distance < MaxDistance && accumulatedAlpha < 0.95)
    {
        vec3 currentPosition = startPosition + distance * direction;
        if (!inVoxelVolume(currentPosition)) break;

        float diameter = max(voxelSize, 2.0 * tanHalfAngle * distance);
        float lod = clamp(log2(diameter / voxelSize), 0.0, log2(float(VoxelDimensions)));
        vec4 voxel = sampleVoxel(currentPosition, lod);
        float weight = (1.0 - accumulatedAlpha) * voxel.a;
        accumulatedRadiance += weight * voxel.rgb;
        accumulatedAlpha += weight;

        distance += diameter * 0.5;
    }

    return vec4(accumulatedRadiance, accumulatedAlpha);
}

vec4 indirectLambert(vec3 worldPosition, vec3 normal)
{
    vec3 helper = abs(normal.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(helper, normal));
    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 tangentToWorld = mat3(tangent, bitangent, normal);

    vec3 tracedRadiance = vec3(0.0);
    float occlusion = 0.0;
    for (int i = 0; i < ConeNum; ++i)
    {
        vec3 direction = normalize(tangentToWorld * ConeDirections[i]);
        vec4 result = coneTrace(worldPosition, direction, 0.577);
        tracedRadiance += ConeWeights[i] * result.rgb;
        occlusion += ConeWeights[i] * result.a;
    }

    float ao = 1.0 - clamp(occlusion, 0.0, 1.0);
    ao *= ao;

    // Return the unscaled result. Forward applies its factor to the complete
    // vec4 at the call site, including the AO alpha.
    return vec4(tracedRadiance, ao);
}

vec3 fresnelSchlick(float cosTheta, vec3 f0);

vec3 indirectSpecular(vec3 worldPosition, vec3 normal, vec3 viewDirection,
    vec3 albedo)
{
    vec3 reflectionDirection = normalize(reflect(-viewDirection, normal));
    // Match render.frag: fixed 4-degree reflection cone (tan(4 degrees) ~= 0.07).
    return coneTrace(worldPosition, reflectionDirection, 0.07).rgb * albedo;
}

float shadowVisibility(vec3 worldPosition)
{
    vec4 lightPosition = LightSpaceMatrix * vec4(worldPosition, 1.0);
    vec3 shadowCoords = lightPosition.xyz / lightPosition.w;
    shadowCoords = shadowCoords * 0.5 + 0.5;
    if (shadowCoords.z > 1.0 || any(lessThan(shadowCoords.xy, vec2(0.0))) ||
        any(greaterThan(shadowCoords.xy, vec2(1.0)))) return 1.0;

    // Deliberately one comparison for the first deferred-lighting version.
    return texture(ShadowMap, vec3(shadowCoords.xy, shadowCoords.z - 0.005));
}

float distributionGGX(float nDotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denominator = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denominator * denominator, 0.0001);
}

float geometrySchlickGGX(float nDotV, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return nDotV / max(nDotV * (1.0 - k) + k, 0.0001);
}

float geometrySmith(float nDotV, float nDotL, float roughness)
{
    return geometrySchlickGGX(nDotV, roughness) * geometrySchlickGGX(nDotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}
vec3 linearToSRGB(vec3 linear)
{
    return pow(linear, vec3(1.0 / 2.2));
}
void main()
{
    vec4 position = texture(GPosition, TexCoord);
    if (position.a < 0.5) discard;

    vec4 normalRoughness = texture(GNormalRoughness, TexCoord);
    vec4 albedoMetallic = texture(GAlbedoMetallic, TexCoord);
    vec3 emission = texture(GEmission, TexCoord).rgb;

    vec3 worldPosition = position.xyz;
    vec3 normal = normalize(normalRoughness.rgb * 2.0 - 1.0);
    vec3 albedo = albedoMetallic.rgb;
    float metallic = clamp(albedoMetallic.a, 0.0, 1.0);
    float roughness = clamp(normalRoughness.a, 0.04, 1.0);
    vec3 viewDirection = normalize(CameraPosition - worldPosition);
    vec3 lightDirection = normalize(ToLightDirection);
    vec3 halfVector = normalize(viewDirection + lightDirection);

    float nDotL = max(dot(normal, lightDirection), 0.0);
    float nDotV = max(dot(normal, viewDirection), 0.0);
    float nDotH = max(dot(normal, halfVector), 0.0);
    float vDotH = max(dot(viewDirection, halfVector), 0.0);
    float visibility = shadowVisibility(worldPosition);
    
    vec3 direct = vec3(0.0);
    if (ShowDirect > 0 && nDotL > 0.0) {
        vec3 f0 = mix(vec3(0.04), albedo, metallic);
        vec3 fresnel = fresnelSchlick(vDotH, f0);
        float distribution = distributionGGX(nDotH, roughness);
        float geometry = geometrySmith(nDotV, nDotL, roughness);
        vec3 specular = distribution * geometry * fresnel /
            max(4.0 * nDotV * nDotL, 0.0001);
        vec3 diffuse = (1.0 - fresnel) * (1.0 - metallic) * albedo / PI;
        if (ShowSpecular <= 0) specular = vec3(0.0);
        direct = (diffuse + specular) * nDotL * visibility;
    }

    // Match Forward: apply the factor to RGB and alpha at this point.
    vec4 indirectResult = 4.0 * indirectLambert(worldPosition, normal);
    vec3 indirect = ShowIndirect > 0
        ? indirectResult.rgb * albedo  *indirectResult.a
        : vec3(0.0);
    if (ShowIndirect > 0 && ShowSpecular > 0) {
        indirect += indirectSpecular(worldPosition, normal, viewDirection,
            albedo) * indirectResult.a;
    }
    vec3 result = direct + indirect+ emission;
    result = linearToSRGB(result);
    if (ShowAo > 0) result = vec3(indirectResult.a);
    Color = vec4(result, 1.0);
}
