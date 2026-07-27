#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

out vec3 PositionWorld;
out vec3 NormalWorld;
out vec3 TangentWorld;
out vec3 BitangentWorld;
out vec2 TexCoord;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

void main()
{
    PositionWorld = (ModelMatrix * vec4(vertexPosition, 1.0)).xyz;
    NormalWorld = normalize(mat3(transpose(inverse(ModelMatrix))) * vertexNormal);
    TangentWorld = normalize(mat3(ModelMatrix) * vertexTangent);
    BitangentWorld = normalize(mat3(ModelMatrix) * vertexBitangent);
    TexCoord = vertexTexCoord;
    gl_Position = ProjectionMatrix * ViewMatrix * vec4(PositionWorld, 1.0);
}
