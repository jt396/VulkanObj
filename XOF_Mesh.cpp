/*
===============================================================================

    XOF
    ===
    File    :    XOF_Mesh.cpp
    Desc    :    Represents a mesh, be it from a file or manually created.

===============================================================================
*/
#include "XOF_Mesh.hpp"
#include "VulkanHelpers.hpp"
// For loading
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
// TEMP
#include <iostream>


enum TEMP_TEXTURE_TYPES {
    DIFFUSE,
    NORMAL,
    SPECULAR,
    COUNT
};


Mesh::Mesh() {
    mIsLoaded = false;
}

Mesh::~Mesh() {}

bool Mesh::Load( MeshDesc& desc ) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string error;
    if ( !tinyobj::LoadObj( &attributes, &shapes, &materials, &error, desc.fileName, "../../../Resources/" ) ) {
        std::cerr << "MESH FAILED TO LOAD: " << error << std::endl;
        return false;
    }

    std::unordered_map<Vertex, unsigned int> uniqueVertices;

    for( const auto& shape : shapes ) {
        for( const auto& index : shape.mesh.indices ) {
            Vertex v;

            v.pos = {
                attributes.vertices[3 * index.vertex_index + 0],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2]
            };

            if( attributes.normals.size() > 0 ) {
                v.normal = {
                    attributes.normals[3 * index.normal_index + 0],
                    attributes.normals[3 * index.normal_index + 1],
                    attributes.normals[3 * index.normal_index + 2]
                };
            }

            v.texCoord = {
                attributes.texcoords[2 * index.texcoord_index + 0],
                attributes.texcoords[2 * index.texcoord_index + 1]
            };

            if( uniqueVertices.count(v) == 0 ) {
                uniqueVertices[v] = mVertexData.size();
                mVertexData.push_back( v );
            }

            mIndexData.push_back( uniqueVertices[v] );
        }
    }

    // Calculate tangents
    for( unsigned int i=0; i<mIndexData.size(); i+=3 ) {
        Vertex v0 = mVertexData[mIndexData[i + 0]];
        Vertex v1 = mVertexData[mIndexData[i + 1]];
        Vertex v2 = mVertexData[mIndexData[i + 2]];

        glm::vec3 edge0 = v1.pos - v0.pos;
        glm::vec3 edge1 = v2.pos - v0.pos;

        float uDelta0 = v1.texCoord.x - v0.texCoord.x;
        float vDelta0 = v1.texCoord.y - v0.texCoord.y;
        float uDelta1 = v2.texCoord.x - v0.texCoord.x;
        float vDelta1 = v2.texCoord.y - v0.texCoord.y;

        float f = 1.f / ( uDelta0 * vDelta1 - uDelta1 * vDelta0 );

        glm::vec3 tangent;
        tangent.x = f * ( vDelta1 * edge0.x -vDelta0 * edge1.x ); 
        tangent.y = f * ( vDelta1 * edge0.y -vDelta0 * edge1.y ); 
        tangent.z = f *    ( vDelta1 * edge0.z -vDelta0 * edge1.z );

        mVertexData[mIndexData[i + 0]].tangent += tangent;
        mVertexData[mIndexData[i + 1]].tangent += tangent;
        mVertexData[mIndexData[i + 2]].tangent += tangent;
#if 0
        glm::vec3 biTangent;
        biTangent.x = f * ( -uDelta1 * edge0.x - uDelta0 * edge1.x );
        biTangent.y = f * ( -uDelta1 * edge0.y - uDelta0 * edge1.y );
        biTangent.z = f * ( -uDelta1 * edge0.z - uDelta0 * edge1.z );
#endif
    }

    // Setup temp material
    std::vector<std::string> texNames[TEMP_TEXTURE_TYPES::COUNT];
    for (unsigned int i = 0; i < materials.size(); ++i) {
        if (!(materials[i].diffuse_texname.empty())) {
            texNames[DIFFUSE].push_back(std::string(materials[i].diffuse_texname));
        }
        if (!(materials[i].normal_texname.empty())) {
            texNames[NORMAL].push_back(std::string(materials[i].normal_texname));
        }
        if(!(materials[i].specular_texname.empty())) {
            texNames[SPECULAR].push_back(std::string(materials[i].specular_texname));
        }
    }

    CreateTempMaterial(desc, &texNames[0]);

    // Setup per-material/texture submesh info 
    // (only accounts for a single mesh in the file, can be moved to the per-shape loop above) 
    mSubMeshes.resize(materials.size());
    unsigned int indexCount = 0;

    for (unsigned int i = 0; i < materials.size(); ++i) {
        mSubMeshes[i].baseIndex = indexCount * 3;
        mSubMeshes[i].textureIndex = i;
        while ((indexCount < shapes[0].mesh.material_ids.size()) && (shapes[0].mesh.material_ids[indexCount] == i)) {
            ++indexCount;
        }
        mSubMeshes[i].indexCount = (indexCount * 3) - mSubMeshes[i].baseIndex;
    }

    if (!GenerateVertexBuffer(desc)) {
        return mIsLoaded;
    }
    if (!GenerateIndexBuffer(desc)) {
        return mIsLoaded;
    }

    return (mIsLoaded = true);
}

bool Mesh::GenerateVertexBuffer(MeshDesc& desc) {
    VkDeviceSize bufferSize = sizeof(Vertex) * mVertexData.size();

    // Setup staging buffer
    BufferDesc stagingBufferDesc;
    stagingBufferDesc.size = bufferSize;
    stagingBufferDesc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferDesc.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBufferDesc.physicalDevice = desc.physicalDevice;
    stagingBufferDesc.logicalDevice = desc.logicalDevice;

    Buffer stagingBuffer(stagingBufferDesc);

    // Map vertex data to staging buffer
    stagingBuffer.WriteToBufferMemory(mVertexData.data(), bufferSize);

    // Setup device (GPU) local buffer
    BufferDesc bufferDesc;
    bufferDesc.size = bufferSize;
    bufferDesc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferDesc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferDesc.logicalDevice = desc.logicalDevice;
    bufferDesc.physicalDevice = desc.physicalDevice;

    mVertexBuffer.Create(bufferDesc);

    CopyBuffer(stagingBuffer, mVertexBuffer, bufferSize, desc.commandBuffer);
    FlushAndResetCommandBuffer(desc.commandBuffer, desc.queue);

    return true;
}

bool Mesh::GenerateIndexBuffer(MeshDesc& desc) {
    VkDeviceSize bufferSize = sizeof(mIndexData[0]) * mIndexData.size();

    // Setup staging buffer
    BufferDesc stagingBufferDesc;
    stagingBufferDesc.size = bufferSize;
    stagingBufferDesc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferDesc.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBufferDesc.physicalDevice = desc.physicalDevice;
    stagingBufferDesc.logicalDevice = desc.logicalDevice;

    Buffer stagingBuffer(stagingBufferDesc);

    // Map index data to staging buffer
    stagingBuffer.WriteToBufferMemory(mIndexData.data(), bufferSize);

    // Setup device (GPU) local buffer
    BufferDesc bufferDesc;
    bufferDesc.size = bufferSize;
    bufferDesc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferDesc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferDesc.logicalDevice = desc.logicalDevice;
    bufferDesc.physicalDevice = desc.physicalDevice;

    mIndexBuffer.Create(bufferDesc);

    CopyBuffer(stagingBuffer, mIndexBuffer, bufferSize, desc.commandBuffer);
    FlushAndResetCommandBuffer(desc.commandBuffer, desc.queue);

    return true;
}

void Mesh::CreateTempMaterial(MeshDesc& desc, std::vector<std::string> *textureNames) {
    mTempMaterial.vertexShader.Load(desc.vertexShaderConfig);
    mTempMaterial.fragmentShader.Load(desc.fragmentShaderConfig);

    // I know the dynamic allocation here is FAR from perferable, 
    // but I was tired and this was the last thing I had to do to get this working... (that's the explanation)
    mTempMaterial.diffuseMaps.resize(textureNames[DIFFUSE].size());
    for (unsigned int i = 0; i < textureNames[DIFFUSE].size(); ++i) {
        std::string fileNameAndPath("../../../Resources/" + textureNames[DIFFUSE][i]);
        desc.textureConfig.fileName = const_cast<char*>(fileNameAndPath.c_str());
        mTempMaterial.diffuseMaps[i].reset(new Texture(desc.textureConfig));
    }

    mTempMaterial.normalMaps.resize(textureNames[NORMAL].size());
    for (unsigned int i = 0; i < textureNames[NORMAL].size(); ++i) {
        std::string fileNameAndPath("../../../Resources/" + textureNames[NORMAL][i]);
        desc.textureConfig.fileName = const_cast<char*>(fileNameAndPath.c_str());
        mTempMaterial.normalMaps[i].reset(new Texture(desc.textureConfig));
    }

    mTempMaterial.specularMaps.resize(textureNames[SPECULAR].size());
    for (unsigned int i = 0; i < textureNames[SPECULAR].size(); ++i) {
        std::string fileNameAndPath("../../../Resources/" + textureNames[SPECULAR][i]);
        desc.textureConfig.fileName = const_cast<char*>(fileNameAndPath.c_str());
        mTempMaterial.specularMaps[i].reset(new Texture(desc.textureConfig));
    }
}
