#include "SRGBTextureAsset.h"

#include <SDL3_image/SDL_image.h>

bool SRGBTextureAsset::convertToSupportedFormat() {
    //3 byte surfaces take the RGBA path of the base, the conversion fills alpha with 255 so they don't count as transparent
    if (SDL_BYTESPERPIXEL(cpuSurface->format) == 3 || SDL_BYTESPERPIXEL(cpuSurface->format) == 6) {
        SDL_Surface *expandedSurface = SDL_ConvertSurface(cpuSurface, SDL_PIXELFORMAT_ABGR8888);
        if (expandedSurface == nullptr) {
            return false;
        }
        SDL_DestroySurface(cpuSurface);
        cpuSurface = expandedSurface;
    }
    if (!TextureAsset::convertToSupportedFormat()) {
        return false;
    }
    textureMetaData.internalFormatType = GraphicsInterface::InternalFormatTypes::SRGB8_ALPHA8;
    return true;
}
