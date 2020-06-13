// Very basic material struct, done in 10 mins to 
// pull together textures and shaders so they can be stored in a mesh.
// Going with a struct to skip the need for getters/setters/attempts to 
// reinforce access levels that typically come with (or should at least
// be considered) when using a class.
#ifndef MATERIAL_HPP
#define MATERIAL_HPP


#include "XOF_Texture.hpp"
#include "XOF_Shader.hpp"
#include <memory>


struct Material {
    // Different obj files will use differing numbers of textures 
    // Smart pointers, dynamic allocation I know, meet me in Mesh.cpp, I'll explain
    std::vector<std::unique_ptr<Texture>>   diffuseMaps;
    std::vector<std::unique_ptr<Texture>>   normalMaps;
    std::vector<std::unique_ptr<Texture>>   specularMaps;

    // Only accounting for a vertex and fragment shader right now
    Shader                                  vertexShader;
    Shader                                  fragmentShader;

    unsigned int                            GetTextureCount() const { return diffuseMaps.size() + normalMaps.size() + specularMaps.size(); }
};


#endif // MATERIAL_HPP
