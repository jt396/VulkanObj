/*
===============================================================================

    XOF
    ===
    File    :    XOF_Mesh.hpp
    Desc    :    Represents a mesh; uses Tinyobj for loading obj models.

===============================================================================
*/
#ifndef XOF_MESH_HPP
#define XOF_MESH_HPP


#include "VertexDesc.hpp"
#include "XOF_Buffer.hpp"
#include "Material.hpp"
#include "VulkanHelpers.hpp"

#include <vector>
#include <glm/vec3.hpp>



struct MeshDesc {
    VkPhysicalDevice    physicalDevice;
    VkDevice            logicalDevice;
    VkCommandBuffer     commandBuffer;
    VkQueue             queue;
    const char        * fileName;
    // temporarily put fields to set up the material here 
    // (this will obviously introduce a bit of duplication in the fields used)
    ShaderDesc          vertexShaderConfig;
    ShaderDesc          fragmentShaderConfig;
    // assume for now that all textures will be treated the same - hence a single instance
    ImageDesc           textureConfig;
};


class Mesh {
private:
    struct MeshDimensions;
    struct SubMesh;
    
public:
                                        Mesh();
                                        ~Mesh();

    bool                                Load(MeshDesc& desc);
    inline bool                         IsLoaded() const;

    inline std::vector<Mesh::SubMesh>&  GetSubMeshData() const;
    inline unsigned int                 GetSubMeshCount() const;

    inline const Mesh::MeshDimensions&  GetDimensions() const;

    inline std::vector<Vertex>&         GetVertexData() const;
    inline std::vector<unsigned int>&   GetIndexData() const;

    inline Buffer&                      GetVertexBuffer() const;
    inline Buffer&                      GetIndexBuffer() const;

    inline  Material&                   GetTempMaterial() const;

private:
    struct MeshDimensions {
        float        sizeAlongX;
        float        sizeAlongY;
        float        sizeAlongZ;
        glm::vec3    min;
        glm::vec3    max;
    };
    MeshDimensions                      mDimensions;

    // A whole mesh is built up as a collection of submeshes
    struct SubMesh {
        uint32_t    baseIndex;
        uint32_t    indexCount;
        int         textureIndex;
    };
    std::vector<Mesh::SubMesh>          mSubMeshes;

    std::vector<Vertex>                 mVertexData;
    std::vector<unsigned int>           mIndexData;
    Buffer                              mVertexBuffer;
    Buffer                              mIndexBuffer;
    
    Material                            mTempMaterial;

    bool                                mIsLoaded;

    bool                                GenerateVertexBuffer(MeshDesc& desc);
    bool                                GenerateIndexBuffer(MeshDesc& desc);
    void                                CreateTempMaterial(MeshDesc& desc, std::vector<std::string> *textureNames);
};


bool Mesh::IsLoaded() const {
    return mIsLoaded;
}

inline unsigned int Mesh::GetSubMeshCount() const {
    return static_cast<unsigned int>( mSubMeshes.size() );
}

inline std::vector<Mesh::SubMesh>& Mesh::GetSubMeshData() const {
    return const_cast<std::vector<Mesh::SubMesh>&>( mSubMeshes );
}

inline const Mesh::MeshDimensions& Mesh::GetDimensions() const {
    return mDimensions;
}

inline std::vector<Vertex>&    Mesh::GetVertexData() const {
    return const_cast<std::vector<Vertex>&>(mVertexData);
}

inline std::vector<unsigned int>& Mesh::GetIndexData() const {
    return const_cast<std::vector<unsigned int>&>(mIndexData);
}

inline Buffer& Mesh::GetVertexBuffer() const {
    return const_cast<Buffer&>(mVertexBuffer);
}

inline Buffer& Mesh::GetIndexBuffer() const {
    return const_cast<Buffer&>(mIndexBuffer);
}

inline Material& Mesh::GetTempMaterial() const {
    return const_cast<Material&>(mTempMaterial);
}


#endif // XOF_MESH_HPP
