/*
===============================================================================

    XOF
    ===
    File    :    XOF_Texture.hpp
    Desc    :    Represents a texture; loaded in using the stb header-based library.

===============================================================================
*/
#ifndef XOF_TEXTURE_HPP
#define XOF_TEXTURE_HPP


#include "XOF_Image.hpp"


class Texture : public Image {
public:
                                Texture();
                                Texture(ImageDesc& imageDesc);
                                ~Texture();

    bool                        Create(ImageDesc& imageDesc) override;
    inline bool                 IsLoaded() const;

    inline VkSampler            GetSamplerTEMP();

private:
                                // Move out into separate file/class?
    VulkanDeleter<VkSampler>    mTempSampler;

    bool                        mIsLoaded;

    bool                        CreateTextureImage(ImageDesc& imageDesc);
    bool                        CreateTextureImageView(const ImageDesc& imageDesc);
    bool                        CreateTextureSampler(const ImageDesc& imageDesc);
};


bool Texture::IsLoaded() const {
    return mIsLoaded;
}

VkSampler Texture::GetSamplerTEMP() {
    return mTempSampler;
}


#endif // XOF_TEXTURE_HPP
