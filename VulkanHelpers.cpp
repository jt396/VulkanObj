#include "VulkanHelpers.hpp"
#include <fstream>


VkResult CreateDebugReportCallbackEXT( VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *createInfo,
                                       const VkAllocationCallbacks *allocator, VkDebugReportCallbackEXT *callback ) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr( instance, "vkCreateDebugReportCallbackEXT" );
    return func? func( instance, createInfo, allocator, callback ) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugReportCallbackEXT( VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *allocator ) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugReportCallbackEXT" );
    if( func ) {
        func( instance, callback, allocator );
    }
}

// ---

void CreateImageView(VkDevice logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VulkanDeleter<VkImageView>& imageView) {
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = image;
    // How should the data be interpreted?
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Treat as 1D, 2D, 3D texture...
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // What is the images' purpose and how will it be accessed?
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    // VR could use multiple layers here...
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image view!");
    }
}
// ---

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags, VkPhysicalDevice physicaDevice) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicaDevice, &memProperties);

    for (uint32_t i = 0; i<memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VkCommandBuffer CreateCommandBuffer(VkDevice logicalDevice, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo cbAllocateInfo = {};
    cbAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocateInfo.commandPool = commandPool;
    cbAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;

    if (vkAllocateCommandBuffers(logicalDevice, &cbAllocateInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate the SETUP command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void FlushAndResetCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue) {
    // Finish recording, execute and reset
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the SETUP command buffer!");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    // 0 for reset flags - hold onto memory to potentially speed-up subsequent command recording
    if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset the SETUP command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
}
