#ifndef LIMONENGINE_TRANSFORMTEXTURERING_H
#define LIMONENGINE_TRANSFORMTEXTURERING_H

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Texture.h"

//queued frames still read the copy uploaded last, writing into it makes the driver wait for them, so every
//upload goes to the copy the GPU read longest ago
class TransformTextureRing {
public:
    static constexpr uint32_t COPY_COUNT = 3;

private:
    GraphicsInterface* graphicsWrapper;
    uint32_t textureUnit;
    uint32_t width;
    std::vector<std::unique_ptr<Texture>> copies;
    uint32_t currentCopyIndex = 0;

public:
    TransformTextureRing(GraphicsInterface* graphicsWrapper, uint32_t width, uint32_t height, uint32_t textureUnit)
            : graphicsWrapper(graphicsWrapper), textureUnit(textureUnit), width(width) {
        for (uint32_t copyIndex = 0; copyIndex < COPY_COUNT; ++copyIndex) {
            copies.emplace_back(std::make_unique<Texture>(graphicsWrapper, GraphicsInterface::TextureTypes::T2D,
                                                          GraphicsInterface::InternalFormatTypes::RGBA32F,
                                                          GraphicsInterface::FormatTypes::RGBA,
                                                          GraphicsInterface::DataTypes::FLOAT, width, height));
            copies.back()->setFilterMode(GraphicsInterface::FilterModes::NEAREST);//float textures can't filter on ES, and shaders texelFetch anyway
        }
        graphicsWrapper->attachTexture(copies[currentCopyIndex]->getTextureID(), textureUnit);
    }

    //texels hold whole texture rows. Only the first usedRows rows, and the first usedColumns of each, are uploaded
    void upload(const std::vector<glm::vec4>& texels, uint32_t usedColumns, uint32_t usedRows) {
        if (usedColumns == 0 || usedRows == 0) {
            return;
        }
        currentCopyIndex = (currentCopyIndex + 1) % COPY_COUNT;
        Texture* targetCopy = copies[currentCopyIndex].get();
        if (usedColumns == width) {
            targetCopy->updateRegion(0, 0, static_cast<int>(width), static_cast<int>(usedRows), texels.data());
        } else {
            for (uint32_t row = 0; row < usedRows; ++row) {
                targetCopy->updateRegion(0, static_cast<int>(row), static_cast<int>(usedColumns), 1, texels.data() + row * width);
            }
        }
        graphicsWrapper->attachTexture(targetCopy->getTextureID(), textureUnit);
    }
};

#endif //LIMONENGINE_TRANSFORMTEXTURERING_H
