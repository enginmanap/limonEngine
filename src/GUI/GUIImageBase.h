//
// Created by engin on 26.07.2018.
//

#ifndef LIMONENGINE_GUIIMAGEBASE_H
#define LIMONENGINE_GUIIMAGEBASE_H


#include "GUIRenderable.h"

class AssetManager;
class TextureAsset;

class GUIImageBase : public GUIRenderable {
    int imageAttachPoint = 0;//read from the program each render, the pipeline assigns it

    static std::shared_ptr<GraphicsProgram> imageRenderProgram;
protected:
    std::shared_ptr<AssetManager> assetManager;
    std::string imageFile;
    std::shared_ptr<TextureAsset> image;

    //the shader treats every GUI image as color, a Data one gets a warning
    void putImageColorSpace(const std::vector<std::string> &imageName, const std::string &idSuffix) const;
public:
    GUIImageBase(GraphicsInterface* graphicsWrapper,  std::shared_ptr<AssetManager> assetManager, const std::string &imageFile);
    virtual ~GUIImageBase();

    virtual void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram, uint32_t lodLevel) override;

    void getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const override;

    virtual std::vector<std::string> getImageFiles() const {
        return {imageFile};
    }


};


#endif //LIMONENGINE_GUIIMAGEBASE_H
