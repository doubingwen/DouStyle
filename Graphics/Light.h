#pragma once

#include <glm.hpp>

class Light {
public:
    enum class Type {
        Directional,
        Point
    };

    static Light createDirectional(const glm::vec3& direction,
        const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    static Light createPoint(const glm::vec3& position,
        const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f,
        float range = 25.0f);

    Type getType() const { return type; }
    bool isDirectional() const { return type == Type::Directional; }
    bool isPoint() const { return type == Type::Point; }

    const glm::vec3& getDirection() const { return direction; }
    glm::vec3 getDirectionFrom(const glm::vec3& worldPosition) const;
    void setDirection(const glm::vec3& inDirection);
    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getColor() const { return color; }
    float getIntensity() const { return intensity; }
    float getRange() const { return range; }

private:
    Light(Type inType, const glm::vec3& inDirection,
        const glm::vec3& inPosition, const glm::vec3& inColor,
        float inIntensity, float inRange);

    Type type = Type::Directional;
    glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float range = 25.0f;
};
