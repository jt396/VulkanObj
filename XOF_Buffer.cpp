/*
===============================================================================

    XOF
    ===
    File    :    XOF_Buffer.cpp
    Desc    :    Basic wrapper for a Vulkan buffer, couples together the buffer and backing memory.

===============================================================================
*/
#include "XOF_Buffer.hpp"


Buffer::Buffer() {}

Buffer::Buffer(const BufferDesc& desc) {
    Create(desc);
}

Buffer::~Buffer() {}

bool Buffer::Create(const BufferDesc& desc) {
    mRendererLogicalDevice = desc.logicalDevice;
    mBuffer.Set(mRendererLogicalDevice, vkDestroyBuffer);
    mBufferMemory.Set(mRendererLogicalDevice, vkFreeMemory);

    // Create the buffer
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = desc.size;
    bufferCreateInfo.usage = desc.usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(desc.logicalDevice, &bufferCreateInfo, nullptr, &mBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
        return false;
    }

    // Allocate the memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(desc.logicalDevice, mBuffer, &memRequirements);

    VkMemoryAllocateInfo memAllocateInfo = {};
    memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocateInfo.allocationSize = memRequirements.size;
    // using vkFlushMappedMemoryRanges & vkInvalidateMappedMemoryRanges is potentially faster than VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    memAllocateInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, desc.properties, desc.physicalDevice);

    if (vkAllocateMemory(desc.logicalDevice, &memAllocateInfo, nullptr, &mBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory for vertex buffer!");
        return false;
    }

    // Bind
    vkBindBufferMemory(desc.logicalDevice, mBuffer, mBufferMemory, 0);

    return true;
}

void Buffer::WriteToBufferMemory(void *data, size_t size) {
    void *mappedRange;
    vkMapMemory(mRendererLogicalDevice, mBufferMemory, 0, size, 0, &mappedRange);
    memcpy(mappedRange, data, size);
    vkUnmapMemory(mRendererLogicalDevice, mBufferMemory);
}


// ---


void CopyBuffer(Buffer& src, Buffer& dst, VkDeviceSize size, VkCommandBuffer commandBuffer) {
    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src.GetBuffer(), dst.GetBuffer(), 1, &copyRegion);
}
