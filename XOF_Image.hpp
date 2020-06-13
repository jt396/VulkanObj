/*
===============================================================================

    XOF
    ===
    File    :    XOF_Image.hpp
    Desc    :    Represents an image, fulfills genral image usages and serves as a base for textures.

===============================================================================
*/
#ifndef XOF_IMAGE_HPP
#define XOF_IMAGE_HPP


#include <vulkan/vulkan.h>
#include "VulkanHelpers.hpp"


struct ImageDesc {
                            ImageDesc() { memset(this, 0x00, sizeof(ImageDesc)); }
                            ImageDesc(const ImageDesc& desc) { memcpy(this, (void*)&desc, sizeof(ImageDesc));}

                            // Renderer pointers
    VkPhysicalDevice        physicalDevice;
    VkDevice                logicalDevice;
    VkQueue                 queue;
                            // Core image properties
    uint32_t                width;
    uint32_t                height;
    VkFormat                format;
    VkImageTiling           tiling;
    VkImageUsageFlags       usage;
    VkImageAspectFlags      aspect;
    VkMemoryPropertyFlags   properties;
                            // Texture-image
    char                  * fileName;
    VkCommandBuffer         commandBuffer;
                            // Texture-image sampler
                            // ...
};


class Image {
public:
                                    Image();
    virtual                         ~Image();

    virtual bool                    Create(ImageDesc& imageDesc);

    inline VkImage                  GetImageTEMP();
    inline VkImageView              GetImageViewTEMP();

protected:
    VulkanDeleter<VkImage>          mImage;
    VulkanDeleter<VkImageView>      mImageView;
    VulkanDeleter<VkDeviceMemory>   mImageMemory;

    bool                            CreateImage(const ImageDesc& imageDesc);
    bool                            CreateImage(const ImageDesc& imageDesc, VulkanDeleter<VkImage>& image, VulkanDeleter<VkDeviceMemory>& imageMemory);
    bool                            CreateImageView(const ImageDesc& imageDesc);
};


VkImage Image::GetImageTEMP() {
    return mImage;
}

VkImageView Image::GetImageViewTEMP() {
    return mImageView;
}


// ---


void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer);


#endif // XOF_IMAGE_HPP
