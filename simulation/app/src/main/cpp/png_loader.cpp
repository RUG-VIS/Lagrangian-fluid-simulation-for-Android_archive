#include <sys/endian.h>
#include "png_loader.h"

#define LOG_TAG "native-lib"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

ImageData loadSimpleTGA(AAssetManager* assetManager, const char* filename) {
    AAsset* asset = AAssetManager_open(assetManager, filename, AASSET_MODE_BUFFER);
    if (!asset) {
        throw std::runtime_error("Could not open the TGA file");
    }

    // Skip the header we don't need
    AAsset_seek(asset, 12, SEEK_SET);

    // Read the width and height
    int16_t width, height;
    AAsset_read(asset, &width, sizeof(width));
    AAsset_read(asset, &height, sizeof(height));
    width = le16toh(width); // Convert to host endianness if needed
    height = le16toh(height);

    // Skip image descriptor
    AAsset_seek(asset, 2, SEEK_CUR);

    // Allocate memory for the image data
    std::vector<unsigned char> imageData(static_cast<size_t>(width) * height * 4); // Assuming 32-bit RGBA

    // Read the image data
    if (AAsset_read(asset, imageData.data(), imageData.size()) != static_cast<int>(imageData.size())) {
        AAsset_close(asset);
        throw std::runtime_error("Failed to read image data");
    }

    AAsset_close(asset);
    return {width, height, std::move(imageData)};
}