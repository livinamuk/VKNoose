#pragma once

#include "Hell/TextureTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ImageTools {
    // Offline compression
    void CreateAndExportDDS(const std::string& inputFilepath, const std::string& outputFilepath, bool createMipMaps);

    // Loading
    ImageData LoadDDS(const std::string& filepath);
    ImageData LoadUncompressedImage(const std::string& filepath);
    ImageData LoadR16UNormImage(const std::string& filepath);
    ImageData LoadEXRImage(const std::string& filepath);

    // Writing
    void SaveFlippedBitmap(const std::string& filename, const uint8_t* data, int width, int height, int channelCount);
    void SaveBitmap(const std::string& filename, const void* data, int width, int height, ImageFormat format);
    void SaveHeightMapR16UNorm(const std::string& filename, const void* data, int width, int height);
    void SaveFloatArrayTextureAsBitmap(const std::vector<float>& data, int width, int height, ImageFormat format, const std::string& filename);

    // Conversion
    void ConvertRGBA8ToR16SFloat(ImageData& imageData);
    
    // Util
    bool IsEightBitImageFormat(ImageFormat format);
    bool IsHalfFloatImageFormat(ImageFormat format);
    bool IsFullFloatImageFormat(ImageFormat format);
    float HalfToFloat(uint16_t value);
    uint16_t FloatToHalf(float value);
}
