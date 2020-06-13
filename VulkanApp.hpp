#ifndef VULKAN_APP_HPP
#define VULKAN_APP_HPP


#include "VulkanHelpers.hpp"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <memory>
#include <algorithm>
#include <set>
#include <vector>
#include <iostream>
#include <stdexcept>


#include "XOF_Mesh.hpp"
#include "XOF_Buffer.hpp"
#include "XOF_Lights.hpp"


static const char* gValidationLayers[] = {
    "VK_LAYER_LUNARG_standard_validation",
};
#define VALIDATION_LAYER_COUNT sizeof( gValidationLayers ) / sizeof( char* )
#if NDEBUG
    const bool enableValidationLayers = false;
#else 
    const bool enableValidationLayers = true;
#endif

static const char* gRequiredExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
#define REQUIRED_EXTENSION_COUNT sizeof( gRequiredExtensions ) / sizeof( char* )


// Helper structs
struct QueueFamilyDesc {
    int     graphicsFamily = -1;
    int     presentationFamily = -1;

    bool    IsComplete() const {
                return ( graphicsFamily >= 0 && presentationFamily >= 0 ); 
            }
};

struct SwapChainDesc {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentationModes;
};


class VulkanApp {
public:
    void                                        Run();

    static VkBool32                             DebugCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
                                                               uint64_t obj, size_t location, int32_t code,
                                                               const char *layerPrefix, const char *msg, void *userData );

private:
    GLFWwindow                                * mWindow;
    
                                                // The order here matters
    VulkanDeleter<VkInstance>                   mInstance{vkDestroyInstance};
    VulkanDeleter<VkDebugReportCallbackEXT>     mDebugCallback{mInstance, DestroyDebugReportCallbackEXT};
    VulkanDeleter<VkSurfaceKHR>                 mSurface{mInstance, vkDestroySurfaceKHR};
    
    VkPhysicalDevice                            mPhysicalDevice = VK_NULL_HANDLE;
    VulkanDeleter<VkDevice>                     mLogicalDevice{vkDestroyDevice};

    QueueFamilyDesc                             queueFamilyDesc;
    VkQueue                                     mGraphicsQueue;
    VkQueue                                     mPresentationQueue;

    VulkanDeleter<VkSwapchainKHR>               mSwapChain{mLogicalDevice, vkDestroySwapchainKHR};
    std::vector<VkImage>                        mSwapChainImages;
    VkFormat                                    mSwapChainFormat;
    VkExtent2D                                  mSwapChainExtents;
    std::vector<VulkanDeleter<VkImageView>>     mSwapChainImageViews;
    
    VulkanDeleter<VkRenderPass>                 mRenderPass{mLogicalDevice, vkDestroyRenderPass};
                                                // Uniform-buffers specific
    VulkanDeleter<VkDescriptorSetLayout>        mDescriptorSetLayout{mLogicalDevice, vkDestroyDescriptorSetLayout};
    VulkanDeleter<VkPipelineLayout>             mPipelineLayout{mLogicalDevice, vkDestroyPipelineLayout};
                                                // ------------------------
    VulkanDeleter<VkPipeline>                   mPipeline{mLogicalDevice, vkDestroyPipeline};
    
    VulkanDeleter<VkCommandPool>                mCommandPool{mLogicalDevice, vkDestroyCommandPool};
    std::vector<VkCommandBuffer>                mCommandBuffers;
                                                // ADDED
                                                // Remember - command buffers are freed when their respective command pool is destroyed - so no wrapper is needed
    VkCommandBuffer                             mSetupCommandBuffer;
    void                                        PrepSetupCommandBuffer();
    void                                        FlushSetupCommandBuffer();
                                                // ------------------------
    VulkanDeleter<VkSemaphore>                  mImageAvailableSemaphore{mLogicalDevice, vkDestroySemaphore};
    VulkanDeleter<VkSemaphore>                  mRenderFinishedSemaphore{mLogicalDevice, vkDestroySemaphore};

    std::vector<VulkanDeleter<VkFramebuffer>>   mFramebuffers;

    Buffer                                      mUniformBuffer;
    Buffer                                      mUniformStagingBuffer;

    VulkanDeleter<VkDescriptorPool>             mDescritporPool{mLogicalDevice, vkDestroyDescriptorPool};
    VkDescriptorSet                             mDescriptorSet;

                                                // Added for depth-buffering
    Image                                       mDepthImageInst;
    void                                        SetupDepthBufferingResources();
    VkFormat                                    SelectDepthImageFormat();
    VkFormat                                    FindSuitableFormat( const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags features );
                                                // ------------------------

    Mesh                                        mTempMesh;
                                                // ------------------------

                                                // Added for directional light
    DirectionalLight                            mDirectionalLight;
    Buffer                                      mDirectionalLightUniformStagingBuffer;
    Buffer                                      mDirectionalLightUniformBuffer;
                                                // ------------------------

    std::vector<const char*>                    GetRequiredExtensions();
    bool                                        CheckValidationLayerSupport();

    static void                                 OnWindowResized( GLFWwindow *window, int newWidth, int newHeight );
    void                                        InitWindow();

                                                // Vulkan startup
    void                                        CreateInstance();
    void                                        SetupDebugCallback();
    void                                        CreateSurface();
    void                                        PickPhysicalDevice();
    void                                        CreateLogicalDevice();
    void                                        CreateSwapChain();
    void                                        CreateSwapChainImageViews();
    void                                        CreateRenderPass();
                                                // Uniform-buffers specific
    void                                        CreateDescriptorSetLayout();
    void                                        CreateGraphicsPipeline();
    void                                        CreateFramebuffers();
    void                                        CreateCommandPool();
    void                                        CreateCommandBuffers();
    void                                        CreateSemaphores();
                                                // Uniform-buffers specific
    void                                        CreateUniformBuffer();
    void                                        CreateDescriptorPool();
                                                // ------------------------
    void                                        CreateDescriptorSet();

    bool                                        IsPhysicalDeviceSuitable( VkPhysicalDevice *physicalDevice );
    bool                                        CheckDeviceExtensionSupport( VkPhysicalDevice *physicalDevice );
    QueueFamilyDesc                             FindQueueFamilies( VkPhysicalDevice *physicalDevice );

                                                // Swap chain
    void                                        QuerySwapChainSupport( VkPhysicalDevice *physicalDevice, SwapChainDesc &swapChainDesc );
    VkSurfaceFormatKHR                          SelectSwapChainFormat( const std::vector<VkSurfaceFormatKHR>& formats );
    VkPresentModeKHR                            SelectSwapChainPresentMode( const std::vector<VkPresentModeKHR>& presentModes );
    VkExtent2D                                  SelectSwapChainSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );
    void                                        RecreateSwapChain();

    void                                        InitVulkan();
    void                                        DrawFrame();
                                                // Staging buffer specific
    void                                        UpdateUniformBuffer();
                                                // ------------------------
    void                                        MainLoop();
};


#endif // VULKAN_APP_HPP
