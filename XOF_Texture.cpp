/*
===============================================================================

    XOF
    ===
    File    :    XOF_Texture.cpp
    Desc    :    Represents a texture; loaded in using the stb header-based library.

===============================================================================
*/
#include "XOF_Texture.hpp"
#include <iostream>

//STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


static void CopyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height, VkCommandBuffer commandBuffer);


Texture::Texture() { 
    mIsLoaded = false; 
}

Texture::Texture(ImageDesc& imageDesc) {
    mIsLoaded = Create(imageDesc);
}

Texture::~Texture() {}

bool Texture::Create(ImageDesc& imageDesc) {
    mImage.Set(imageDesc.logicalDevice, vkDestroyImage);
    mImageView.Set(imageDesc.logicalDevice, vkDestroyImageView);
    mImageMemory.Set(imageDesc.logicalDevice, vkFreeMemory);
    mTempSampler.Set(imageDesc.logicalDevice, vkDestroySampler);

    if (CreateTextureImage(imageDesc) && CreateTextureImageView(imageDesc) && CreateTextureSampler(imageDesc)) {
        return (mIsLoaded = true);
    }

    return mIsLoaded;
}

bool Texture::CreateTextureImage(ImageDesc& imageDesc) {
    int textureChannels;

    stbi_uc *texturePixelData = stbi_load(imageDesc.fileName, (int*)&(imageDesc.width), (int*)&(imageDesc.height), &textureChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = imageDesc.width * imageDesc.height * 4;

    if (!texturePixelData) {
        return false;
        throw std::runtime_error("Failed to load texture image!");
    }

    // Create the staging texture
    VulkanDeleter<VkImage> stagingImage;  stagingImage.Set(imageDesc.logicalDevice, vkDestroyImage);
    VulkanDeleter<VkDeviceMemory> stagingImageMemory{ imageDesc.logicalDevice, vkFreeMemory };

    //createimage()...
    ImageDesc stagingImageDesc(imageDesc);
    stagingImageDesc.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    stagingImageDesc.tiling = VK_IMAGE_TILING_LINEAR;
    stagingImageDesc.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    CreateImage(stagingImageDesc, stagingImage, stagingImageMemory);

    // Put the texture data into the staging image
    void *data;
    vkMapMemory(imageDesc.logicalDevice, stagingImageMemory, 0, imageSize, 0, &data);
    memcpy(data, texturePixelData, imageSize);
    vkUnmapMemory(imageDesc.logicalDevice, stagingImageMemory);

    stbi_image_free(texturePixelData);

    // Create the actual texture
    CreateImage(imageDesc);

    // Copy the staging image to the final texture image
    TransitionImageLayout(stagingImage, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageDesc.commandBuffer);
    TransitionImageLayout(mImage, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageDesc.commandBuffer); // <- TODO: Try VK_IMAGE_LAYOUT_UNDEFINED here
    // ADDED - Try doing the transitions all at once rather than in individual buffers
    FlushAndResetCommandBuffer(imageDesc.commandBuffer, imageDesc.queue);
    // ---
    CopyImage(stagingImage, mImage, imageDesc.width, imageDesc.height, imageDesc.commandBuffer);
    //FlushSetupCommandBuffer(); // <- NOTE: SHOULD WE FLUSH AGAIN HERE TO BE SAFE?
    // So we can sample the texture in a shader
    TransitionImageLayout(mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageDesc.commandBuffer);
    // ADDED - Try doing the transitions all at once rather than in individual buffers
    FlushAndResetCommandBuffer(imageDesc.commandBuffer, imageDesc.queue);

    return true;
}

bool Texture::CreateTextureImageView(const ImageDesc& imageDesc) {
    return CreateImageView(imageDesc);
}

bool Texture::CreateTextureSampler(const ImageDesc& imageDesc) {
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy = 16;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    // Could be used for shadow mapping
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // Mipmap specific
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.mipLodBias = 0.f;
    samplerCreateInfo.minLod = 0.f;
    samplerCreateInfo.maxLod = 0.f;

    if (vkCreateSampler(imageDesc.logicalDevice, &samplerCreateInfo, nullptr, &mTempSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler!");
        return false;
    }
    return true;
}


// ---


static void CopyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height, VkCommandBuffer commandBuffer) {
    VkImageSubresourceLayers subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResource.mipLevel = 0;
    subResource.baseArrayLayer = 0;
    subResource.layerCount = 1;

    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource = subResource;
    imageCopyRegion.dstSubresource = subResource;
    imageCopyRegion.srcOffset = { 0, 0, 0 };
    imageCopyRegion.dstOffset = { 0, 0, 0 };
    imageCopyRegion.extent.width = width;
    imageCopyRegion.extent.height = height;
    imageCopyRegion.extent.depth = 1;

    vkCmdCopyImage(commandBuffer,
                   srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1
                   &imageCopyRegion);
}
