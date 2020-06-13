#ifndef VULKAN_HELPERS_HPP
#define VULKAN_HELPERS_HPP


#include <vulkan/vulkan.h>
#include <functional>
#include <vector>


// Wraps vulkan objects and takes care of cleanup
template<typename T>
class VulkanDeleter {
public:
    // Init on construction
    VulkanDeleter() {}

    VulkanDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
        mDeleter = [=](T obj) { deletef(obj, nullptr); };
    }

    VulkanDeleter(const VulkanDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
        mDeleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    VulkanDeleter(const VulkanDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        mDeleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }
    VulkanDeleter(const VkDevice device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        mDeleter = [device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    ~VulkanDeleter() {
        Cleanup();
    }

    // Init after construction
    void Set(std::function<void(T, VkAllocationCallbacks*)> deletef) {
        mDeleter = [=](T obj) { deletef(obj, nullptr); };
    }

    void Set(const VulkanDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
        mDeleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    void Set(const VulkanDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        mDeleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }
    void Set(const VkDevice device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        mDeleter = [device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    // Operators
    T* operator&() {
        Cleanup();
        return &mObject;
    }

    operator T() const { return mObject; }

    T Get() { return mObject; }

private:
    T mObject{ VK_NULL_HANDLE };
    std::function<void(T)> mDeleter;

    void Cleanup() { 
        if (mObject) { 
            mDeleter(mObject); 
        } 
        mObject = VK_NULL_HANDLE; 
    }
};
// ---


// Utility and helper functions
VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *createInfo,
                                      const VkAllocationCallbacks *allocator, VkDebugReportCallbackEXT *callback);

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *allocator);

void CreateImageView(VkDevice logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VulkanDeleter<VkImageView>& imageView);

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags, VkPhysicalDevice physicaDevice);

VkCommandBuffer CreateCommandBuffer(VkDevice logicalDevice, VkCommandPool commandPool);
void FlushAndResetCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue);
// ---


#endif // VULKAN_HELPERS_HPP
