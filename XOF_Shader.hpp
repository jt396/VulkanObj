/*
===============================================================================

    XOF
    ===
    File    :    XOF_Shader.hpp
    Desc    :    Represents a SPIR-V-based shader.

===============================================================================
*/
#ifndef XOF_SHADER_HPP
#define XOF_SHADER_HPP


#include "VulkanHelpers.hpp"
#include <vulkan/vulkan.h>


struct ShaderDesc {
    VkDevice                logialDevice;
    VkShaderStageFlagBits   shaderType;
    const char            * fileName;
    const char            * mainFunctionName;
};


class Shader {
public:
                                            Shader();
                                            ~Shader();

    bool                                    Load(const ShaderDesc& shaderDesc);
    inline bool                             IsLoaded() const;

    inline VkPipelineShaderStageCreateInfo  GetPipelineCreationInfo() const;

private:
    VulkanDeleter<VkShaderModule>           mShaderModule;
    VkPipelineShaderStageCreateInfo         mPipelineCreationInfo;

    bool                                    mIsLoaded;
};


bool Shader::IsLoaded() const {
    return mIsLoaded;
}

VkPipelineShaderStageCreateInfo Shader::GetPipelineCreationInfo() const {
    return mPipelineCreationInfo;
}


#endif // XOF_SHADER_HPP
