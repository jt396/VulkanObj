/*
===============================================================================

    XOF
    ===
    File    :    XOF_Buffer.hpp
    Desc    :    Basic wrapper for a Vulkan buffer, couples together the buffer and backing memory.

===============================================================================
*/
#ifndef XOF_BUFFER_HPP
#define XOF_BUFFER_HPP


#include "VulkanHelpers.hpp"
#include <vulkan/vulkan.h>


struct BufferDesc {
    VkDeviceSize            size;
    VkBufferUsageFlags      usage; 
    VkMemoryPropertyFlags   properties;
    VkDevice                logicalDevice;
    VkPhysicalDevice        physicalDevice;
};


class Buffer {
public:
                                    Buffer();
                                    Buffer(const BufferDesc& desc);
                                    ~Buffer();

    bool                            Create(const BufferDesc& desc);

    void                            WriteToBufferMemory(void *data, size_t size);

    inline VkBuffer                 GetBuffer();
    inline VkDeviceMemory           GetBufferMemory();

private:
    VkDevice                        mRendererLogicalDevice;
    VulkanDeleter<VkBuffer>         mBuffer;
    VulkanDeleter<VkDeviceMemory>   mBufferMemory;
};



inline VkBuffer Buffer::GetBuffer() {
    return mBuffer;
}

inline VkDeviceMemory Buffer::GetBufferMemory() {
    return mBufferMemory;
}


// ---


void CopyBuffer(Buffer& src, Buffer& dst, VkDeviceSize size, VkCommandBuffer buffer);


#endif // XOF_BUFFER_HPP
