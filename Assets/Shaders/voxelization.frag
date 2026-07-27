#version 430
#extension GL_ARB_shader_image_load_store : enable

// Current path: accumulate fixed-point RGB sums and sample count independently.
layout(binding = 0, r32ui) coherent volatile uniform uimage3D VoxelSumR;
layout(binding = 1, r32ui) coherent volatile uniform uimage3D VoxelSumG;
layout(binding = 2, r32ui) coherent volatile uniform uimage3D VoxelSumB;
layout(binding = 3, r32ui) coherent volatile uniform uimage3D VoxelCount;

// Kept for comparison/future switching: packed RGBA8 average using a CAS loop.
layout(binding = 4, r32ui) coherent volatile uniform uimage3D CASVoxelTexture;

uniform sampler2D DiffuseTexture;

uniform sampler2DShadow ShadowMap;
uniform sampler2D ShadowMapDepthTexture;

uniform float Opacity;

uniform vec3 ToLightDirection;
uniform vec3 BaseColorFactor;
uniform int VoxelDimensions;

in GeomData
{
	vec3 normal;
    vec2 texCoord;
    flat int axis;            // 1 represent X, 2 represent Y, 3 represent Z
    vec3 positionLightSpace;
} geomOut;

const int BlockerSearchSamples = 16;
const int PCFSearchSamples = 16;
vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
);

float PCSS(vec3 lightSpacePos)
{
    vec3 shadowCoords = lightSpacePos;
    if(shadowCoords.z > 1.0 || any(lessThan(shadowCoords.xy, vec2(0.0))) || any(greaterThan(shadowCoords.xy, vec2(1.0))))
        return 1.0;    // out of range

    float currentDepth = shadowCoords.z;

    // find blocker depth
    float blockerDepthSum = 0.0;
    int blockerCount = 0;
    float searchRadius = 0.002;
    for(int i = 0; i < BlockerSearchSamples; i++)
    {
        vec2 offset = poissonDisk[i] * searchRadius;
        vec2 sampleCoords = shadowCoords.xy + offset;
        if(any(lessThan(sampleCoords, vec2(0.0))) || any(greaterThan(sampleCoords, vec2(1.0))))
            continue;

        float blockerDepth = texture(ShadowMapDepthTexture, sampleCoords).z;
        if(texture(ShadowMap, vec3(sampleCoords, currentDepth - 0.005)) < 0.5)
        {
            blockerDepthSum += blockerDepth;
            blockerCount++;
        }
    }

    if(blockerCount == 0)
        return 1.0;
    else if(blockerCount == BlockerSearchSamples)
        return 0.0;

    // calculate PCF radius
    float averageBlockerDepth = blockerDepthSum / float(blockerCount);
    float PCFRadius = 0.005 * (currentDepth - averageBlockerDepth);

    // PCF
    float visibilitySum = 0.0;
    for(int i = 0; i < PCFSearchSamples; i++)
    {
        vec2 offset = poissonDisk[i] * PCFRadius;
        vec2 sampleCoords = shadowCoords.xy + offset;
        if(any(lessThan(sampleCoords, vec2(0.0))) || any(greaterThan(sampleCoords, vec2(1.0))))
            continue;

        visibilitySum += texture(ShadowMap, vec3(sampleCoords, currentDepth - 0.005));
    }

    return visibilitySum / float(PCFSearchSamples);
}

// Legacy path: accumulate RGB and the sample count in one uint using a CAS retry loop.
void imageAtomicRGBA8AvgCAS(ivec3 coord, vec4 value)
{
    uint previousValue = 0u;
    uint newValue = packUnorm4x8(value);

    while (true)
    {
        uint currentValue = imageAtomicCompSwap(CASVoxelTexture, coord, previousValue, newValue);
        if (currentValue == previousValue)
            break;

        previousValue = currentValue;
        vec4 stored = unpackUnorm4x8(currentValue);
        stored.w *= 256.0;
        stored.rgb *= stored.w;
        vec4 accumulated = stored + value;
        accumulated.rgb /= accumulated.w;
        accumulated.w /= 256.0;
        newValue = packUnorm4x8(accumulated);
    }
}

// Current path: atomic integer additions avoid the packed CAS quantization and contention.
// Radiance is in the [0, 1] range here, so 65536 gives useful fixed-point precision.
const float RadianceScale = 65536.0;

void imageAtomicRGBCount(ivec3 coord, vec3 radiance)
{
    vec3 clampedRadiance = clamp(radiance, vec3(0.0), vec3(1.0));
    imageAtomicAdd(VoxelSumR, coord, uint(clampedRadiance.r * RadianceScale + 0.5));
    imageAtomicAdd(VoxelSumG, coord, uint(clampedRadiance.g * RadianceScale + 0.5));
    imageAtomicAdd(VoxelSumB, coord, uint(clampedRadiance.b * RadianceScale + 0.5));
    imageAtomicAdd(VoxelCount, coord, 1u);
}

void main()
{
    ivec3 fragmentPos = ivec3(gl_FragCoord.x, gl_FragCoord.y, VoxelDimensions * gl_FragCoord.z);
	ivec3 voxelPos;
	if(geomOut.axis == 1)
	{
	    voxelPos.x = VoxelDimensions - fragmentPos.z - 1;
		voxelPos.y = fragmentPos.y;
		voxelPos.z = VoxelDimensions - fragmentPos.x - 1;
	}
	else if(geomOut.axis == 2)
	{
		voxelPos.x = fragmentPos.x;
		voxelPos.y = VoxelDimensions - fragmentPos.z - 1;
	    voxelPos.z = VoxelDimensions - fragmentPos.y - 1;
	}
	else
	{
		voxelPos.x = fragmentPos.x;
		voxelPos.y = fragmentPos.y;
	    voxelPos.z = VoxelDimensions - fragmentPos.z - 1;
	}

	vec4 materialColor = texture(DiffuseTexture, geomOut.texCoord);

	vec3 lightDir = normalize(ToLightDirection);
	float cosTheta = max(0, dot(geomOut.normal, lightDir));

	//float visibility = texture(ShadowMap, vec3(geomOut.positionLightSpace.xy, geomOut.positionLightSpace.z - 0.005));
	float visibility = PCSS(geomOut.positionLightSpace);
  
    // Match render.frag: voxel radiance uses the sampled diffuse color directly.
    vec3 radiance = materialColor.rgb * cosTheta * visibility;
    imageAtomicRGBCount(voxelPos, radiance);
    // To use the legacy packed CAS path later, replace the line above with:
    // imageAtomicRGBA8AvgCAS(voxelPos, vec4(radiance, 1.0));
}
