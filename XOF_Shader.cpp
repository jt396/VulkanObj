/*
===============================================================================

    XOF
    ===
    File    :    XOF_Shader.cpp
    Desc    :    Represents a SPIR-V-based shader.

===============================================================================
*/
#include "XOF_Shader.hpp"
#include <fstream>


Shader::Shader() { mIsLoaded = false; }
Shader::~Shader() {}

bool Shader::Load(const ShaderDesc& shaderDesc) {
    if (mIsLoaded) {
        return mIsLoaded;
    }

    // Read in shader source
    std::ifstream file(shaderDesc.fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file!");
        return mIsLoaded;
    }

    size_t fileSize = file.tellg();
    std::vector<char> shaderSourceBuffer;
    shaderSourceBuffer.resize(fileSize);

    file.seekg(0);
    file.read(shaderSourceBuffer.data(), fileSize);
    file.close();

    // Create shader module
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pCode = (const uint32_t*)(shaderSourceBuffer.data());
    shaderModuleCreateInfo.codeSize = shaderSourceBuffer.size();

    mShaderModule.Set(shaderDesc.logialDevice, vkDestroyShaderModule);
    if (vkCreateShaderModule(shaderDesc.logialDevice, &shaderModuleCreateInfo, nullptr, &mShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
        return mIsLoaded;
    }

    // Create pipeline shader stage creation info
    mPipelineCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    mPipelineCreationInfo.stage = shaderDesc.shaderType;
    mPipelineCreationInfo.module = mShaderModule;
    mPipelineCreationInfo.pName = shaderDesc.mainFunctionName;

    return (mIsLoaded = true);
}
