#ifndef XOF_LIGHTS_HPP
#define XOF_LIGHTS_HPP


#include <glm/vec4.hpp>


struct DirectionalLight {
    glm::vec4    colour;
    glm::vec4    direction;
    float        ambientIntensity;
    float        diffuseIntensity;
};


#endif // XOF_LIGHTS_HPP
