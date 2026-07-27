#include "Light.h"

Light::Light(Type inType, const glm::vec3& inDirection,
    const glm::vec3& inPosition, const glm::vec3& inColor,
    float inIntensity, float inRange)
    : type(inType), direction(inDirection), position(inPosition),
      color(inColor), intensity(inIntensity), range(inRange)
{
}

Light Light::createDirectional(const glm::vec3& direction,
    const glm::vec3& color, float intensity)
{
    return Light(Type::Directional, glm::normalize(direction),
        glm::vec3(0.0f), color, intensity, 0.0f);
}

Light Light::createPoint(const glm::vec3& position,
    const glm::vec3& color, float intensity, float range)
{
    return Light(Type::Point, glm::vec3(0.0f), position,
        color, intensity, range);
}

glm::vec3 Light::getDirectionFrom(const glm::vec3& worldPosition) const
{
    if (isDirectional()) return direction;
    return glm::normalize(position - worldPosition);
}

void Light::setDirection(const glm::vec3& inDirection)
{
    if (glm::length(inDirection) > 0.0001f)
        direction = glm::normalize(inDirection);
}
