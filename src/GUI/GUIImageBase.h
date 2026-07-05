//
// Created by engin on 26.07.2018.
//

#ifndef LIMONENGINE_GUIIMAGEBASE_H
#define LIMONENGINE_GUIIMAGEBASE_H


#include "GUIRenderable.h"

class AssetManager;
class TextureAsset;

class GUIImageBase : public GUIRenderable {
    //Actual value is (re)computed from getMaxTextureImageUnits() each render (see renderWithProgram);
    //this default is never used for a real bind.
    int imageAttachPoint = 0;

    static std::shared_ptr<GraphicsProgram> imageRenderProgram;
protected:
    std::shared_ptr<AssetManager> assetManager;
    std::string imageFile;
    std::shared_ptr<TextureAsset> image;
public:
    GUIImageBase(GraphicsInterface* graphicsWrapper,  std::shared_ptr<AssetManager> assetManager, const std::string &imageFile);
    virtual ~GUIImageBase();

    virtual void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram, uint32_t lodLevel) override;

    void getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const override;


};


#endif //LIMONENGINE_GUIIMAGEBASE_H
