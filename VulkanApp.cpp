#include "VulkanApp.hpp"

#include <chrono>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


static const unsigned int INITIAL_WINDOW_WIDTH = 800;
static const unsigned int INITIAL_WINDOW_HEIGHT = 600;


static unsigned int fps;
static double lastTime;


void VulkanApp::Run() { 
    InitWindow();
    InitVulkan();
    MainLoop();
}

VkBool32 VulkanApp::DebugCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
                                   uint64_t obj, size_t location, int32_t code,
                                   const char *layerPrefix, const char *msg, void *userData ) {
    std::cerr << "\nValidation layer: " << msg << std::endl;
    return VK_FALSE;
}

std::vector<const char*> VulkanApp::GetRequiredExtensions() {
    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    for( unsigned int i=0; i<glfwExtensionCount; ++i ) {
        extensions.push_back( glfwExtensions[i] );
    }

    if( enableValidationLayers ) {
        extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
    }

    return extensions;
}

bool VulkanApp::CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

    VkLayerProperties *availableLayers = new VkLayerProperties[layerCount];
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers );

    for( unsigned int i=0; i<VALIDATION_LAYER_COUNT; ++i ) {
        bool layerFound = false;
        for( unsigned int j=0; j<layerCount; ++j ) {
            if( strcmp( gValidationLayers[i], availableLayers[j].layerName ) == 0 ) {
                layerFound = true;
                break;
            }
        }
        if( !layerFound ) {
            delete [] availableLayers;
            return false;
        }
    }

    delete [] availableLayers;
    return true;
}

void VulkanApp::OnWindowResized( GLFWwindow *window, int newWidth, int newHeight ) {
    if( newWidth == 0 || newHeight == 0 ) {
        return;
    }

    VulkanApp *app = reinterpret_cast<VulkanApp*>( glfwGetWindowUserPointer( window ) );
    app->RecreateSwapChain();
}

void VulkanApp::InitWindow() {
    glfwInit();
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

    mWindow = glfwCreateWindow( INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, "Vulkan", nullptr, nullptr );

    glfwSetWindowUserPointer( mWindow, this );
    glfwSetWindowSizeCallback( mWindow, VulkanApp::OnWindowResized );
}

void VulkanApp::CreateInstance() {
    // Technically optional
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan - Example (Loading OBJ)";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Which extensions and global validation layers do we want to use
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    // We need an extension to interact with the underlying platforms' window system (due to Vulkan being API agnostic)
    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Global validation layers
    createInfo.enabledLayerCount = enableValidationLayers? VALIDATION_LAYER_COUNT : 0;
    createInfo.ppEnabledLayerNames = enableValidationLayers? gValidationLayers : nullptr;

    if( vkCreateInstance( &createInfo, nullptr, &mInstance ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create Vulkan instance!" );
    }
}

void VulkanApp::SetupDebugCallback() {
    if( enableValidationLayers && !CheckValidationLayerSupport() ) {
        throw std::runtime_error( "Validation layers requested but not available!" );
    }

    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)DebugCallback;

    if( CreateDebugReportCallbackEXT( mInstance, &createInfo, nullptr, &mDebugCallback ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to setup debug callback!" );
    }
}

void VulkanApp::CreateSurface() {
    if( glfwCreateWindowSurface( mInstance, mWindow, nullptr, &mSurface ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create window surface!" );
    }
}

void VulkanApp::PickPhysicalDevice() {
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices( mInstance, &physicalDeviceCount, nullptr );

    if( physicalDeviceCount == 0 ) {
        throw std::runtime_error( "No physical devices found!" );
    }

    std::unique_ptr<VkPhysicalDevice> physicalDevices( new VkPhysicalDevice[physicalDeviceCount] );
    vkEnumeratePhysicalDevices( mInstance, &physicalDeviceCount, physicalDevices.get() );

    for( unsigned int i=0; i<physicalDeviceCount; ++i ) {
        if( IsPhysicalDeviceSuitable( &physicalDevices.get()[i] ) ) {
            mPhysicalDevice = physicalDevices.get()[i];
            break;
        }
    }

    if( mPhysicalDevice == VK_NULL_HANDLE ) {
        throw std::runtime_error( "Couldn't find a suitable physical device!" );
    }
}

// Specify queues to create, specify device features to be used
void VulkanApp::CreateLogicalDevice() {
    QueueFamilyDesc queueFamilyDesc = FindQueueFamilies( &mPhysicalDevice );

    std::set<int> uniqueQueueFamies = {queueFamilyDesc.graphicsFamily, queueFamilyDesc.presentationFamily};
    std::unique_ptr<VkDeviceQueueCreateInfo> createQueueInfo( new VkDeviceQueueCreateInfo[uniqueQueueFamies.size()] );

    float priority = 1.f;
    for( auto queueFamily : uniqueQueueFamies ) {
        memset( &createQueueInfo.get()[queueFamily], 0x00, sizeof( VkDeviceQueueCreateInfo ) );
        createQueueInfo.get()[queueFamily].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createQueueInfo.get()[queueFamily].queueFamilyIndex = queueFamily;
        createQueueInfo.get()[queueFamily].queueCount = 1;
        createQueueInfo.get()[queueFamily].pQueuePriorities = &priority;
    }

    VkPhysicalDeviceFeatures physicaDeviceFeatures = {};

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = createQueueInfo.get();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>( uniqueQueueFamies.size() );
    deviceCreateInfo.pEnabledFeatures = &physicaDeviceFeatures;
    deviceCreateInfo.enabledLayerCount = enableValidationLayers? VALIDATION_LAYER_COUNT : 0;
    deviceCreateInfo.ppEnabledLayerNames = enableValidationLayers? gValidationLayers : nullptr;
    deviceCreateInfo.enabledExtensionCount = REQUIRED_EXTENSION_COUNT;
    deviceCreateInfo.ppEnabledExtensionNames = gRequiredExtensions;

    if( vkCreateDevice( mPhysicalDevice, &deviceCreateInfo, nullptr, &mLogicalDevice ) != VK_SUCCESS ) {
        throw std::runtime_error( "Could not create logical device!" );
    }

    vkGetDeviceQueue( mLogicalDevice, queueFamilyDesc.graphicsFamily, 0, &mGraphicsQueue );
    vkGetDeviceQueue( mLogicalDevice, queueFamilyDesc.presentationFamily, 0, &mPresentationQueue );
}

void VulkanApp::CreateSwapChain() {
    SwapChainDesc swapChainDesc;
    QuerySwapChainSupport( &mPhysicalDevice, swapChainDesc );

    VkSurfaceFormatKHR surfaceFormat = SelectSwapChainFormat( swapChainDesc.formats );
    VkPresentModeKHR presentMode = SelectSwapChainPresentMode( swapChainDesc.presentationModes );
    VkExtent2D extent = SelectSwapChainSwapExtent( swapChainDesc.capabilities );

    // Add one for triple-buffering
    uint32_t imageCount = swapChainDesc.capabilities.minImageCount + 1;
    // A maxImageCount of 0 means there is no hard-limit (subject only to memory constraints), hence the check
    if( swapChainDesc.capabilities.maxImageCount > 0 && imageCount > swapChainDesc.capabilities.maxImageCount ) {
        imageCount = swapChainDesc.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = mSurface;

    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    // How many layers each image consists of (will be > 1 for stereoscopic 3D for example)
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    FindQueueFamilies( &mPhysicalDevice );
    uint32_t queueFamilyIndices[] = {queueFamilyDesc.graphicsFamily, queueFamilyDesc.presentationFamily};

    if( queueFamilyDesc.graphicsFamily != queueFamilyDesc.presentationFamily ) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        // TODO: Get rid of hard-coded 2
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapChainCreateInfo.preTransform = swapChainDesc.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    VkSwapchainKHR oldSwapChain = mSwapChain;
    swapChainCreateInfo.oldSwapchain = oldSwapChain;

    VkSwapchainKHR newSwapChain;
    if( vkCreateSwapchainKHR( mLogicalDevice, &swapChainCreateInfo, nullptr, &newSwapChain ) ) {
        throw std::runtime_error( "Failed to create swap chain!" );
    }
    *&mSwapChain = newSwapChain;

    vkGetSwapchainImagesKHR( mLogicalDevice, mSwapChain, &imageCount, nullptr );
    mSwapChainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( mLogicalDevice, mSwapChain, &imageCount, mSwapChainImages.data() );

    mSwapChainFormat = surfaceFormat.format;
    mSwapChainExtents = extent;
}

void VulkanApp::CreateSwapChainImageViews() {
    // Setup device and destroy function for each image view
    mSwapChainImageViews.resize( mSwapChainImages.size(), VulkanDeleter<VkImageView>{ mLogicalDevice, vkDestroyImageView } );

    for( uint32_t i=0; i<mSwapChainImages.size(); ++i ) {
        CreateImageView( mLogicalDevice, mSwapChainImages[i], mSwapChainFormat, VK_IMAGE_ASPECT_COLOR_BIT, mSwapChainImageViews[i] );
    }
}

void VulkanApp::CreateRenderPass() {
    // Colour attachment
    VkAttachmentDescription colourAttachmentDesc = {};
    colourAttachmentDesc.format = mSwapChainFormat;
    colourAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    // Clear to black
    colourAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;        // colour data
    // So we can see the triangle on screen
    colourAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;      // colour data
    colourAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colourAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colourAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colourAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colourAttachmentRef = {};
    // Index into attachments array corresponds to layout(location=N) directives in the shader source
    colourAttachmentRef.attachment = 0;
    colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // ---

    // Depth attachment
    VkAttachmentDescription depthAttachmentDesc = {};
    depthAttachmentDesc.format = SelectDepthImageFormat();
    depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Depth image contents won't change during actual rendering so keep them the same
    depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // ---

    VkAttachmentDescription attachments[] = { colourAttachmentDesc, depthAttachmentDesc };


    VkSubpassDescription subPass = {};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colourAttachmentRef;
    subPass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency subpassDependency = {};
    // VK_SUBPASS_EXTERNAL refers to implicit subpass before or after the actual render pass
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    // This is the first and only subpass
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = sizeof( attachments ) / sizeof( VkAttachmentDescription );
    // Index into attachments array corresponds to layout(location=N) directives in the shader source
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subPass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    if( vkCreateRenderPass( mLogicalDevice, &renderPassCreateInfo, nullptr, &mRenderPass ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create render pass!" );
    }
}

void VulkanApp::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    // Corresponds to the binding used in the shader
    uboLayoutBinding.binding = 0;
    // Corresponds to the type used in the shader
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // NOTE: Could store a matrix here for each bone in a skeleton;
    // (for now we're using a single object so we set it to 1)
    uboLayoutBinding.descriptorCount = 1;
    // Only using the ubo in the vertex shader right now
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // Only relevant to texture sampling
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerBinding[3] = {};
    samplerBinding[0].binding = 1;
    samplerBinding[0].descriptorCount = 2; // 2 diffuse, 2 normal and 2 specular maps for the barrel model
    samplerBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding[0].pImmutableSamplers = nullptr;
    samplerBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    samplerBinding[1].binding = 2;
    samplerBinding[1].descriptorCount = 2;
    samplerBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding[1].pImmutableSamplers = nullptr;
    samplerBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    samplerBinding[2].binding = 3;
    samplerBinding[2].descriptorCount = 2;
    samplerBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding[2].pImmutableSamplers = nullptr;
    samplerBinding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Directional light specific
    VkDescriptorSetLayoutBinding directionalLightBinding = {};
    directionalLightBinding.binding = 4;
    directionalLightBinding.descriptorCount = 1;
    directionalLightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    directionalLightBinding.pImmutableSamplers = nullptr;
    directionalLightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // -----------------------

    VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerBinding[0], samplerBinding[1], samplerBinding[2], directionalLightBinding};

    VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCreateInfo = {};
    descriptorSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetlayoutCreateInfo.bindingCount = sizeof( bindings ) / sizeof( VkDescriptorSetLayoutBinding );
    descriptorSetlayoutCreateInfo.pBindings = bindings;

    if( vkCreateDescriptorSetLayout( mLogicalDevice, &descriptorSetlayoutCreateInfo, nullptr, &mDescriptorSetLayout ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create descriptor set!" );
    }
}

void VulkanApp::CreateGraphicsPipeline() {
    VkPipelineShaderStageCreateInfo shaderStages[] = { 
        mTempMesh.GetTempMaterial().vertexShader.GetPipelineCreationInfo(),
        mTempMesh.GetTempMaterial().fragmentShader.GetPipelineCreationInfo(),
    };

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &Vertex::GetBindingDescription();
    auto attributeDescs = Vertex::GetVertexInputAttributeDescriptions();
    vertexInputCreateInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescs.size();
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescs.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<float>( mSwapChainExtents.width );
    viewport.height = static_cast<float>( mSwapChainExtents.height );
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = mSwapChainExtents;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    // If this is true then geometry will never pass the rasterizer = no oputput in framebuffer
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    // Any mode other than FILL requires enabling a GPU feature
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // Sometimes used for shadow-mapping amongst other things (not used here)
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

    // Multisampling (enable a GPU feature to use)
    VkPipelineMultisampleStateCreateInfo msCreateInfo = {};
    msCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msCreateInfo.sampleShadingEnable = VK_FALSE;
    msCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    /* Following fields are all optional
    msCreateInfo.minSampleShading = 1.f;
    msCreateInfo.pSampleMask = nullptr;
    msCreateInfo.alphaToCoverageEnable = VK_FALSE;
    msCreateInfo.alphaToOneEnable = VK_FALSE;*/

    // Depth and stencil
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    // Optional depth-bound test, only keep fragments within a specified range (not used)
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.minDepthBounds = 0.f;
    depthStencilCreateInfo.maxDepthBounds = 1.f;
    // ---

    // Colour-blending
    VkPipelineColorBlendAttachmentState colourBlendAttachment = {};
    colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // FALSE = pass the new colour from the fragment shader through with no modification
    colourBlendAttachment.blendEnable = VK_FALSE;
    /* The following fields are all optional
    colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;*/

    VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo = {};
    colourBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colourBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    colourBlendStateCreateInfo.attachmentCount = 1;
    colourBlendStateCreateInfo.pAttachments = &colourBlendAttachment;
    // The following fields are optional
    colourBlendStateCreateInfo.blendConstants[0] = 0.f;
    colourBlendStateCreateInfo.blendConstants[1] = 0.f;
    colourBlendStateCreateInfo.blendConstants[2] = 0.f;
    colourBlendStateCreateInfo.blendConstants[3] = 0.f;

    /* Dynamic state (a limited amount of pipeline state can be changed without rebuilding the pipeline)
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;*/

    // Pipeline layout (details uniforms etc.)
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkDescriptorSetLayout descriptorSetLayouts[] = {mDescriptorSetLayout};
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;

    // Push constants
    VkPushConstantRange textureIndexPushConstant = { 0 };
    textureIndexPushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureIndexPushConstant.offset = 0;
    textureIndexPushConstant.size = sizeof(int);
    // Add to pipeline layout
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &textureIndexPushConstant;

    if( vkCreatePipelineLayout( mLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create pipeline layout!" );
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // 0) Shader stage
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;
    // 1) Fixed-function stage
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &msCreateInfo;
    // Optional
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    graphicsPipelineCreateInfo.pColorBlendState = &colourBlendStateCreateInfo;
    // Optional
    graphicsPipelineCreateInfo.pDynamicState = nullptr;
    // 2) Pipeline layout
    graphicsPipelineCreateInfo.layout = mPipelineLayout;
    // 3) Render-pass
    graphicsPipelineCreateInfo.renderPass = mRenderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    // Other
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    //graphicsPipelineCreateInfo.basePipelineIndex = -1;

    if( vkCreateGraphicsPipelines( mLogicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &mPipeline ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create graphics pipeline(s)!" );
    }
}

void VulkanApp::CreateFramebuffers() {
    mFramebuffers.resize( mSwapChainImageViews.size(), VulkanDeleter<VkFramebuffer>{mLogicalDevice, vkDestroyFramebuffer} );

    for(size_t i=0; i<mSwapChainImageViews.size(); ++i ) {
        VkImageView attachments[] = {
            mSwapChainImageViews[i],
            mDepthImageInst.GetImageViewTEMP()
        };

        VkFramebufferCreateInfo fbCreateInfo = {};
        fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCreateInfo.renderPass = mRenderPass;
        // Attachment fields describe the attachments to bind to the attachment descriptions in the render pass pAttachment array
        fbCreateInfo.attachmentCount = sizeof( attachments ) / sizeof( VkImageView );
        fbCreateInfo.pAttachments = attachments;
        fbCreateInfo.width = mSwapChainExtents.width;
        fbCreateInfo.height = mSwapChainExtents.height;
        fbCreateInfo.layers = 1;

        if( vkCreateFramebuffer( mLogicalDevice, &fbCreateInfo, nullptr, &mFramebuffers[i] ) != VK_SUCCESS ) {
            throw std::runtime_error( "Failed to create framebuffer!" );
        }
    }
}

void VulkanApp::CreateCommandPool() {
    QueueFamilyDesc queueFamilyDesc = FindQueueFamilies( &mPhysicalDevice );

    VkCommandPoolCreateInfo cpCreateInfo = {};
    cpCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpCreateInfo.queueFamilyIndex = queueFamilyDesc.graphicsFamily;                        
    // Optional
    cpCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if( vkCreateCommandPool( mLogicalDevice, &cpCreateInfo, nullptr, &mCommandPool ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create command pool! ");
    }
}

void VulkanApp::CreateCommandBuffers() {
    if( mCommandBuffers.size() > 0 ) {
        vkFreeCommandBuffers( mLogicalDevice, mCommandPool, (uint32_t)mCommandBuffers.size(), mCommandBuffers.data() );
    }

    // Allocate and record commands for each swap-chain image
    mCommandBuffers.resize( mFramebuffers.size() );

    VkCommandBufferAllocateInfo cbAllocateInfo = {};
    cbAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocateInfo.commandPool = mCommandPool;
    cbAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocateInfo.commandBufferCount = static_cast<uint32_t>( mCommandBuffers.size() );

    if( vkAllocateCommandBuffers( mLogicalDevice, &cbAllocateInfo, mCommandBuffers.data() ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to allocate command buffers!" );
    }

    // CommandBufferBegin + RenderPassBegin  + BindPipeline + Draw + EndRenderPass + EndCommandBuffer
    for( size_t i=0; i<mCommandBuffers.size(); ++i ) {
        VkCommandBufferBeginInfo cbBeginInfo = {};
        cbBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        // Optional (only relevant for secondary command buffers)
        //cbBeginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer( mCommandBuffers[i], &cbBeginInfo );

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = mRenderPass;
        renderPassBeginInfo.framebuffer = mFramebuffers[i];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = mSwapChainExtents;
        
        // One for the colour attachment, one for the depth/stencil (you need one clearValue for each attachment)
        VkClearValue clearValues[2] = {};
        clearValues[0].color = {0.25f, 0.25f, 0.25f, 1.f};
        clearValues[1].depthStencil = {1.f, 0};

        renderPassBeginInfo.clearValueCount = sizeof( clearValues ) / sizeof( VkClearValue );
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass( mCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
            vkCmdBindPipeline( mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline );

            VkBuffer vertexBuffers[] = {mTempMesh.GetVertexBuffer().GetBuffer()};
            VkDeviceSize offsets[] = {0};

            vkCmdBindVertexBuffers( mCommandBuffers[i], 0, 1, vertexBuffers, offsets );
            vkCmdBindIndexBuffer( mCommandBuffers[i], mTempMesh.GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32 );
            vkCmdBindDescriptorSets( mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSet, 0, nullptr );

            for (unsigned int submeshIndex = 0; submeshIndex < mTempMesh.GetSubMeshCount(); ++submeshIndex) {
                vkCmdPushConstants(mCommandBuffers[i], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), &mTempMesh.GetSubMeshData()[submeshIndex].textureIndex);
                vkCmdDrawIndexed(mCommandBuffers[i], mTempMesh.GetSubMeshData()[submeshIndex].indexCount, 1, mTempMesh.GetSubMeshData()[submeshIndex].baseIndex, 0, 0);
            }
        vkCmdEndRenderPass( mCommandBuffers[i] );

        if( vkEndCommandBuffer( mCommandBuffers[i] ) != VK_SUCCESS ) {
            throw std::runtime_error( "Failed to create command buffers!" );
        }
    }
}

void VulkanApp::CreateSemaphores() {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if( vkCreateSemaphore( mLogicalDevice, &semaphoreCreateInfo, nullptr, &mImageAvailableSemaphore ) != VK_SUCCESS  || 
        vkCreateSemaphore( mLogicalDevice, &semaphoreCreateInfo, nullptr, &mRenderFinishedSemaphore ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create semaphores!" );
    }
}

void VulkanApp::CreateUniformBuffer() {
    VkDeviceSize bufferSize = sizeof( UniformBufferObject );

    // Setup uniform buffer
    BufferDesc bufferDesc;
    bufferDesc.size = sizeof(UniformBufferObject);
    bufferDesc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferDesc.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferDesc.logicalDevice = mLogicalDevice;
    bufferDesc.physicalDevice = mPhysicalDevice;

    mUniformStagingBuffer.Create(bufferDesc);

    bufferDesc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferDesc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    mUniformBuffer.Create(bufferDesc);

    // Directional light uniform
    bufferDesc.size = sizeof(DirectionalLight);
    bufferDesc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferDesc.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    mDirectionalLightUniformStagingBuffer.Create(bufferDesc);

    bufferDesc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferDesc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    mDirectionalLightUniformBuffer.Create(bufferDesc);
}

void VulkanApp::CreateDescriptorPool() {
    VkDescriptorPoolSize descriptorPoolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 }, // mvp matrix + directional light 
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mTempMesh.GetTempMaterial().GetTextureCount() }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSizes) / sizeof(VkDescriptorPoolSize);
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
    descriptorPoolCreateInfo.maxSets = 1;

    if (vkCreateDescriptorPool(mLogicalDevice, &descriptorPoolCreateInfo, nullptr, &mDescritporPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void VulkanApp::CreateDescriptorSet() {
    VkDescriptorSetLayout layouts[] = {mDescriptorSetLayout};
    VkDescriptorSetAllocateInfo descSetAllocInfo = {};
    descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descSetAllocInfo.descriptorPool = mDescritporPool;
    descSetAllocInfo.descriptorSetCount = 1;
    descSetAllocInfo.pSetLayouts = layouts;

    if( vkAllocateDescriptorSets( mLogicalDevice, &descSetAllocInfo, &mDescriptorSet ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to allocate descriptor sets!" );
    }

    VkDescriptorBufferInfo descBufferInfo = {};
    descBufferInfo.buffer = mUniformBuffer.GetBuffer();
    descBufferInfo.offset = 0;
    descBufferInfo.range = sizeof( UniformBufferObject );

    // Directional light specific
    VkDescriptorBufferInfo directionalLightDescBufferInfo = {};
    directionalLightDescBufferInfo.buffer = mDirectionalLightUniformBuffer.GetBuffer();
    directionalLightDescBufferInfo.offset = 0;
    directionalLightDescBufferInfo.range = sizeof( DirectionalLight );
    // ---

    // texture specific
    std::vector<VkDescriptorImageInfo> descImageInfo;
    descImageInfo.resize(mTempMesh.GetTempMaterial().GetTextureCount());

    Material *mat = &(mTempMesh.GetTempMaterial());
    size_t diffuseMapCount = mat->diffuseMaps.size();
    unsigned int descIndex = 0;
    for (unsigned int i = 0; i < diffuseMapCount; ++i) {
        descImageInfo[descIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descImageInfo[descIndex].imageView = mat->diffuseMaps[i]->GetImageViewTEMP();
        descImageInfo[descIndex].sampler = mat->diffuseMaps[i]->GetSamplerTEMP();
        ++descIndex;
    }

    size_t normalMapCount = mat->normalMaps.size();
    for (unsigned int i = 0; i < normalMapCount; ++i) {
        descImageInfo[descIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descImageInfo[descIndex].imageView = mat->normalMaps[i]->GetImageViewTEMP();
        descImageInfo[descIndex].sampler = mat->normalMaps[i]->GetSamplerTEMP();
        ++descIndex;
    }

    size_t specularMapCount = mat->specularMaps.size();
    for (unsigned int i = 0; i < specularMapCount; ++i) {
        descImageInfo[descIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descImageInfo[descIndex].imageView = mat->specularMaps[i]->GetImageViewTEMP();
        descImageInfo[descIndex].sampler = mat->specularMaps[i]->GetSamplerTEMP();
        ++descIndex;
    }

    // Configure descriptor sets
    VkWriteDescriptorSet writeDescSets[5] = {};
    // ubo
    writeDescSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescSets[0].dstSet = mDescriptorSet;
    writeDescSets[0].dstBinding = 0;
    writeDescSets[0].dstArrayElement = 0;
    writeDescSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescSets[0].descriptorCount = 1;
    writeDescSets[0].pBufferInfo = &descBufferInfo;
    // diffuse maps
    writeDescSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescSets[1].dstSet = mDescriptorSet;
    writeDescSets[1].dstBinding = 1;
    writeDescSets[1].dstArrayElement = 0;
    writeDescSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescSets[1].descriptorCount = diffuseMapCount;
    writeDescSets[1].pImageInfo = &descImageInfo[0];
    // normal maps
    writeDescSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescSets[2].dstSet = mDescriptorSet;
    writeDescSets[2].dstBinding = 2;
    writeDescSets[2].dstArrayElement = 0;
    writeDescSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescSets[2].descriptorCount = normalMapCount;
    // normal maps are inserted after the diffuse maps, so we can use diffuseMapCount as a starting index for normals
    writeDescSets[2].pImageInfo = &descImageInfo[diffuseMapCount];
    // specular maps
    writeDescSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescSets[3].dstSet = mDescriptorSet;
    writeDescSets[3].dstBinding = 3;
    writeDescSets[3].dstArrayElement = 0;
    writeDescSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescSets[3].descriptorCount = specularMapCount;
    // same thing for specular, add diffuse and normal map counts to get the correct offset
    writeDescSets[3].pImageInfo = &descImageInfo[diffuseMapCount + normalMapCount];

    // directional light
    writeDescSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescSets[4].dstSet = mDescriptorSet;
    writeDescSets[4].dstBinding = 4;
    writeDescSets[4].dstArrayElement = 0;
    writeDescSets[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescSets[4].descriptorCount = 1;
    writeDescSets[4].pBufferInfo = &directionalLightDescBufferInfo;

    vkUpdateDescriptorSets( mLogicalDevice, sizeof( writeDescSets ) / sizeof( VkWriteDescriptorSet ), writeDescSets, 0, nullptr );
}

bool VulkanApp::IsPhysicalDeviceSuitable( VkPhysicalDevice *physicalDevice ) {
#if 0
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties( *physicalDevice, &props );
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures( *physicalDevice, &features );
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties( *physicalDevice, &memProps );
#endif
    QueueFamilyDesc queueFamilyDesc = FindQueueFamilies( physicalDevice );
    bool requiredExtensionsSupported = CheckDeviceExtensionSupport( physicalDevice );

    // Check swap-chain support
    bool swapChainIsSuitable = false;
    if( requiredExtensionsSupported ) {
        SwapChainDesc swapChainDesc;
        QuerySwapChainSupport( physicalDevice, swapChainDesc );
        swapChainIsSuitable = !swapChainDesc.formats.empty() && !swapChainDesc.presentationModes.empty();
    }

    return queueFamilyDesc.IsComplete() && requiredExtensionsSupported && swapChainIsSuitable;
}

bool VulkanApp::CheckDeviceExtensionSupport( VkPhysicalDevice *physicalDevice ) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties( *physicalDevice, nullptr, &extensionCount, nullptr );

    std::unique_ptr<VkExtensionProperties> supportedExtensions( new VkExtensionProperties[extensionCount] );
    vkEnumerateDeviceExtensionProperties( *physicalDevice, nullptr, &extensionCount, supportedExtensions.get() );

    std::cout << "AVAILABLE EXTENSIONS:\n=====================" << std::endl;
    for( unsigned int i=0; i<extensionCount; ++i ) {
        std::cout << supportedExtensions.get()[i].extensionName << std::endl;
    }
    std::cout << "\nREQUIRED EXTENSIONS:\n=====================" << std::endl;
    for( unsigned int i=0; i<REQUIRED_EXTENSION_COUNT; ++i ) {
        std::cout << gRequiredExtensions[i] << std::endl;
    }
    std::cout << "\n";

    for( unsigned int i=0; i<REQUIRED_EXTENSION_COUNT; ++i ) {
        bool supported = false;
        for( unsigned int j=0; j<extensionCount; ++j ) {
            if( strcmp( supportedExtensions.get()[j].extensionName, gRequiredExtensions[i] ) == 0) {
                supported = true;
            }
        }
        if( !supported ) {
            return false;
        }
    }

    return true;
}

QueueFamilyDesc VulkanApp::FindQueueFamilies( VkPhysicalDevice *physicalDevice ) {
    QueueFamilyDesc familyDesc;

    uint32_t familyCount;
    vkGetPhysicalDeviceQueueFamilyProperties( *physicalDevice, &familyCount, nullptr );

    std::unique_ptr<VkQueueFamilyProperties> familyProperties( new VkQueueFamilyProperties[familyCount] );
    vkGetPhysicalDeviceQueueFamilyProperties( *physicalDevice, &familyCount, familyProperties.get() );

    for( unsigned int i=0; i<familyCount; ++i ) {
        if( familyProperties.get()[i].queueCount > 0 && familyProperties.get()[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
            familyDesc.graphicsFamily = i;
        }

        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( *physicalDevice, i, mSurface, &presentationSupport );
        if( familyProperties.get()[i].queueCount > 0 && presentationSupport ) {
            familyDesc.presentationFamily = i;
        }

        if( familyDesc.IsComplete() ) {
            break;
        }
    }

    return familyDesc;
}

// Key things we need to know: surface capabilities, supported formats and presentation modes
void VulkanApp::QuerySwapChainSupport( VkPhysicalDevice *physicalDevice, SwapChainDesc &swapChainDesc ) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( *physicalDevice, mSurface, &swapChainDesc.capabilities );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( *physicalDevice, mSurface, &formatCount, nullptr );
    swapChainDesc.formats.resize( formatCount );
    vkGetPhysicalDeviceSurfaceFormatsKHR( *physicalDevice, mSurface, &formatCount, &swapChainDesc.formats[0] );

    uint32_t presentationModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( *physicalDevice, mSurface, &presentationModeCount, nullptr );
    swapChainDesc.presentationModes.resize( presentationModeCount );
    vkGetPhysicalDeviceSurfacePresentModesKHR( *physicalDevice, mSurface, &presentationModeCount, &swapChainDesc.presentationModes[0] );
}

// Check for surface format (colour depth), presentation mode (conditions for swapping images to the screen - arguably most important)
// and swap extent (resolution of the images in the swap chain)
VkSurfaceFormatKHR VulkanApp::SelectSwapChainFormat( const std::vector<VkSurfaceFormatKHR>& formats ) {
    // VK_FORMAT_UNDEFINED means the surface has no preferred format (best case scenerio)
    if( formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED ) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for( const auto& f : formats ) {
        if( f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
            return f;
        }
    }

    return formats[0];
}

// Will (more often than not) equal the dimensions of the window been rendered into
VkPresentModeKHR VulkanApp::SelectSwapChainPresentMode( const std::vector<VkPresentModeKHR>& presentModes ) {
    // Prefer triple-buffering
    for( const auto& presentMode : presentModes ) {
        if( presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ) {
            return presentMode;
        }
    }

    // Effectively vsync (program will have to wait if the queue is full),
    // next image is presented at vertical-sync. 
    // This mode is guaranteed to always be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Use the currentExtent member to match the width and height of the window.
// Some window managers will allow us to differ from the window dimensions however.
// This is communicated by width and height to max( uint32_t ). In this scenerio
// we'll pick the resolution that best matches the window within the bounds of
// minImageExtent and maxImageExtent.
VkExtent2D VulkanApp::SelectSwapChainSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) {
    if( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() ) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT};
    actualExtent.width = std::max( capabilities.minImageExtent.width, std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
    actualExtent.width = std::max( capabilities.minImageExtent.height, std::min( capabilities.maxImageExtent.height, actualExtent.height ) );

    return actualExtent;
}

void VulkanApp::RecreateSwapChain() {
    vkDeviceWaitIdle( mLogicalDevice );

    CreateSwapChain();
    CreateSwapChainImageViews();
    CreateRenderPass();

    CreateGraphicsPipeline();
    SetupDepthBufferingResources();
    CreateFramebuffers();

    CreateCommandBuffers();
}

void VulkanApp::InitVulkan() {
    // Directional light specific
    mDirectionalLight.colour = glm::vec4( 1.f, 1.f, 1.f, 1.f );
    mDirectionalLight.direction = glm::vec4( 0.f, -1.f, -1.f, 0.f );
    mDirectionalLight.ambientIntensity = 0.f;
    mDirectionalLight.diffuseIntensity = 0.75f;
    // -----------------------

    CreateInstance();
    SetupDebugCallback();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();

    CreateCommandPool();
    PrepSetupCommandBuffer();

    CreateSwapChain();
    CreateSwapChainImageViews();
    CreateRenderPass();
    // Uniform buffer specific
    CreateDescriptorSetLayout();
    // -----------------------

    // MODEL
    MeshDesc desc;
    desc.physicalDevice = mPhysicalDevice;
    desc.logicalDevice = mLogicalDevice;
    desc.commandBuffer = mSetupCommandBuffer;
    desc.queue = mGraphicsQueue;
    desc.fileName = "../../../Resources/barrel.obj";
    // set shaders - vert
    desc.vertexShaderConfig.logialDevice = mLogicalDevice;
    desc.vertexShaderConfig.shaderType = VK_SHADER_STAGE_VERTEX_BIT;
    desc.vertexShaderConfig.fileName = "../vert.spv";
    desc.vertexShaderConfig.mainFunctionName = "main";
    // frag
    desc.fragmentShaderConfig.logialDevice = mLogicalDevice;
    desc.fragmentShaderConfig.shaderType = VK_SHADER_STAGE_FRAGMENT_BIT;
    desc.fragmentShaderConfig.fileName = "../frag.spv";
    desc.fragmentShaderConfig.mainFunctionName = "main";
    // set texture config
    desc.textureConfig.physicalDevice = mPhysicalDevice;
    desc.textureConfig.logicalDevice = mLogicalDevice;
    desc.textureConfig.queue = mGraphicsQueue;
    desc.textureConfig.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    desc.textureConfig.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.textureConfig.tiling = VK_IMAGE_TILING_OPTIMAL;
    desc.textureConfig.format = VK_FORMAT_R8G8B8A8_UNORM;
    desc.textureConfig.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.textureConfig.commandBuffer = mSetupCommandBuffer;
    //
    mTempMesh.Load(desc);
    // -----------------------

    CreateGraphicsPipeline();
    SetupDepthBufferingResources();
    CreateFramebuffers();

    // Uniform buffer specific
    CreateUniformBuffer();
    CreateDescriptorPool();
    CreateDescriptorSet();
    // -----------------------
    CreateCommandBuffers();
    CreateSemaphores();
}

void VulkanApp::DrawFrame() {
    // Get image from swap-chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR( mLogicalDevice, mSwapChain, std::numeric_limits<uint64_t>::max(), // using uint64_t::max() for timeout disables it
                                             mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

    if( result == VK_ERROR_OUT_OF_DATE_KHR ) {
        RecreateSwapChain();
    } else if( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR ) {
        throw std::runtime_error( "Failed to acquire swap chain image!" );
    }

    // Execute the command buffer with that image as attachment in the framebuffer
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {mImageAvailableSemaphore};
    VkPipelineStageFlags pipelineWaitStageFlags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = &pipelineWaitStageFlags[0];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if( vkQueueSubmit( mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to submit draw command buffer!" );
    }

    // Return the image to the swap chain for presentation
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {mSwapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR( mPresentationQueue, &presentInfo );
    if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ) {
        RecreateSwapChain();
    } else if( result != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to present swap chain image!" );
    }

    // Count the frames
    ++fps;
    double thisTime = glfwGetTime();
    if( ( thisTime - lastTime ) >= 1.0 ) {
        //std::cout << "FPS: " << fps << std::endl;
        std::string fpsCount("Vulkan | FPS: " + std::to_string(fps));
        glfwSetWindowTitle(mWindow, fpsCount.c_str());

        fps = 0;
        lastTime = thisTime;
    }
}

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
void VulkanApp::UpdateUniformBuffer() {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>( currentTime - startTime ).count() / 1000.f;

    UniformBufferObject ubo = {};
    ubo.model = glm::translate( ubo.model, glm::vec3( 0.f, -1.75, 0.f ) );
    ubo.model *= glm::rotate( glm::mat4(), time * glm::radians( 45.f ), glm::vec3( 0.f, 1.f, 0.f ) );
    ubo.view = glm::lookAt( glm::vec3( -2.f, 2.f, 5.f ), glm::vec3( 0.f ), glm::vec3( 0.f, 1.f, 0.f ) );
    ubo.projection = glm::perspective( glm::radians( 45.f ), mSwapChainExtents.width / (float)mSwapChainExtents.height, 0.1f, 10.f );
    // glm was made for OpenGL which uses inverted Y coordinates
    ubo.projection[1][1] *= -1.f;

    // update uniforms
    mDirectionalLightUniformStagingBuffer.WriteToBufferMemory((void*)&mDirectionalLight, sizeof(DirectionalLight));
    CopyBuffer( mDirectionalLightUniformStagingBuffer, mDirectionalLightUniformBuffer, sizeof( DirectionalLight ), mSetupCommandBuffer );
    FlushSetupCommandBuffer();

    mUniformStagingBuffer.WriteToBufferMemory((void*)&ubo, sizeof(UniformBufferObject));
    CopyBuffer( mUniformStagingBuffer, mUniformBuffer, sizeof( UniformBufferObject ), mSetupCommandBuffer );
    FlushSetupCommandBuffer();
}

void VulkanApp::MainLoop() {
    fps = 0;
    lastTime = glfwGetTime();
    while( !glfwWindowShouldClose( mWindow ) ) {
        glfwPollEvents();
        UpdateUniformBuffer();
        DrawFrame();
    }
    vkDeviceWaitIdle( mLogicalDevice );
}

void VulkanApp::PrepSetupCommandBuffer() {
    // Create/allocate/setup helper/setup command buffer
    VkCommandBufferAllocateInfo cbAllocateInfo = {};
    cbAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocateInfo.commandPool = mCommandPool;
    cbAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocateInfo.commandBufferCount = 1;

    if( vkAllocateCommandBuffers( mLogicalDevice, &cbAllocateInfo, &mSetupCommandBuffer ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate the SETUP command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer( mSetupCommandBuffer, &beginInfo );
}

void VulkanApp::FlushSetupCommandBuffer() {
    // Finish recording, execute and reset
    if( vkEndCommandBuffer( mSetupCommandBuffer ) != VK_SUCCESS ) {
        throw std::runtime_error("Failed to create the SETUP command buffer!");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mSetupCommandBuffer;

    vkQueueSubmit( mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    vkQueueWaitIdle( mGraphicsQueue );

    // 0 for reset flags - hold onto memory to potentially speed-up subsequent command recording
    if (vkResetCommandBuffer(mSetupCommandBuffer, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset the SETUP command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(mSetupCommandBuffer, &beginInfo);
}
// -----------------------

// Added for depth-buffering
void VulkanApp::SetupDepthBufferingResources() {
    VkFormat depthFormat = SelectDepthImageFormat();

    ImageDesc imageDesc;
    imageDesc.physicalDevice = mPhysicalDevice;
    imageDesc.logicalDevice = mLogicalDevice;
    imageDesc.queue = mGraphicsQueue;
    //
    imageDesc.width = mSwapChainExtents.width;
    imageDesc.height = mSwapChainExtents.height;
    imageDesc.format = depthFormat;
    imageDesc.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageDesc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageDesc.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageDesc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    mDepthImageInst.Create(imageDesc);

    TransitionImageLayout( mDepthImageInst.GetImageTEMP(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, mSetupCommandBuffer );
    FlushSetupCommandBuffer();
}

VkFormat VulkanApp::SelectDepthImageFormat() {
    return FindSuitableFormat( {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

VkFormat VulkanApp::FindSuitableFormat( const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags features ) {
    for( VkFormat format : candidateFormats ) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties( mPhysicalDevice, format, &formatProperties );

        if( tiling == VK_IMAGE_TILING_LINEAR && ( formatProperties.linearTilingFeatures & features ) /* == features */) { 
            return format;
        }
        else if( tiling == VK_IMAGE_TILING_OPTIMAL && ( formatProperties.optimalTilingFeatures & features ) ) {
            return format;
        }
    }
    // Shouldn't get here
    throw std::runtime_error( "Failed to find suitable format!" );
}
// -----------------------
