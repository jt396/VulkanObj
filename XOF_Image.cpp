/*
===============================================================================

    XOF
    ===
    File    :    XOF_Image.cpp
    Desc    :    Represents an image, fulfills genral image usages and serves as the base for textures.

===============================================================================
*/
#include "XOF_Image.hpp"


Image::Image() {}
Image::~Image() {}

bool Image::Create(ImageDesc& imageDesc) {
    mImage.Set(imageDesc.logicalDevice, vkDestroyImage);
    mImageView.Set(imageDesc.logicalDevice, vkDestroyImageView);
    mImageMemory.Set(imageDesc.logicalDevice, vkFreeMemory);

    if (CreateImage(imageDesc) && CreateImageView(imageDesc)) {
        return true;
    }

    return false;
}

bool Image::CreateImage(const ImageDesc& imageDesc) {
    return CreateImage(imageDesc, mImage, mImageMemory);
}

bool Image::CreateImage(const ImageDesc& imageDesc, VulkanDeleter<VkImage>& image, VulkanDeleter<VkDeviceMemory>& imageMemory) {
    // Parameters for an image
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D; // What coordinate system the texels in the image should be addressed in
    imageCreateInfo.extent.width = imageDesc.width;
    imageCreateInfo.extent.height = imageDesc.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = imageDesc.format;
    imageCreateInfo.tiling = imageDesc.tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageCreateInfo.usage = imageDesc.usage;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Only using one queue family so exclusive is fine
                                                             // Multisampling
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.flags = 0;

    // Image memory allocation is done in the same way as buffer memory allocation
    if (vkCreateImage(imageDesc.logicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
        return false;
    }

    VkMemoryRequirements imageMemReqs = {};
    vkGetImageMemoryRequirements(imageDesc.logicalDevice, image, &imageMemReqs);

    VkMemoryAllocateInfo imageMemAllocInfo = {};
    imageMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    imageMemAllocInfo.allocationSize = imageMemReqs.size;
    imageMemAllocInfo.memoryTypeIndex = FindMemoryType(imageMemReqs.memoryTypeBits, imageDesc.properties, imageDesc.physicalDevice);

    if (vkAllocateMemory(imageDesc.logicalDevice, &imageMemAllocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
        return false;
    }

    vkBindImageMemory(imageDesc.logicalDevice, image, imageMemory, 0);

    return true;
}

bool Image::CreateImageView(const ImageDesc& imageDesc) {
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = mImage;
    // How should the data be interpreted?
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Treat as 1D, 2D, 3D texture...
    imageViewCreateInfo.format = imageDesc.format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // What is the images' purpose and how will it be accessed?
    imageViewCreateInfo.subresourceRange.aspectMask = imageDesc.aspect;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    // VR could use multiple layers here...
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(imageDesc.logicalDevice, &imageViewCreateInfo, nullptr, &mImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image view!");
        return false;
    }
    return true;
}


// ---


void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer) {
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    // VK_IMAGE_LAYOUT_UNDEFINED can be used here if we don't care about the contents
    barrier.oldLayout = oldLayout;
    // ---
    barrier.newLayout = newLayout;
    // Setting these to queue family indices allows for queue family ownership transfers
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // ---
    barrier.image = image;
    barrier.subresourceRange.aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) ?
        VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    // Handle transition type - Specify which types of operations must happen before the barrier and
    // which types of operations must wait on the barrier
    if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else {
        throw std::runtime_error("Failed to handle iamge layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
        0, nullptr, 0, nullptr,
        1, &barrier);
}
