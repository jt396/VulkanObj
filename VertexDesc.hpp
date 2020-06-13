#ifndef VERTEX_DESC_HPP
#define VERTEX_DESC_HPP


#include <vulkan\vulkan.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <array>
#include <cstddef>


struct Vertex {
    glm::vec3                                               tangent;
    glm::vec3                                               pos;
    glm::vec3                                               colour;
    glm::vec3                                               normal;
    glm::vec2                                               texCoord;

    static VkVertexInputBindingDescription                  GetBindingDescription() {
        VkVertexInputBindingDescription bindingDesc = {};

        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof( Vertex );
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDesc;
    }

    static std::array<VkVertexInputAttributeDescription, 5> GetVertexInputAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof( Vertex, pos );

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof( Vertex, colour );

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof( Vertex, normal );

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof( Vertex, tangent );

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[4].offset = offsetof( Vertex, texCoord );

        return attributeDescriptions;
    }

    bool operator==( const Vertex& v ) const {
        if( &v == this ) return true;
        return pos == v.pos && colour == v.colour && texCoord == v.texCoord && normal == v.normal && v.tangent == tangent;
    }
};


// To use vertices in a map
#include<glm/gtx/hash.hpp>
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(const Vertex& v) const {
            return ((hash<glm::vec3>()(v.pos) ^ (hash<glm::vec3>()(v.colour) << 1)) >> 1) ^ (hash<glm::vec2>()(v.texCoord) << 1);
        }
    };
}


#include <glm/mat4x4.hpp>
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};


#endif // VERTEX_DESC_HPP
