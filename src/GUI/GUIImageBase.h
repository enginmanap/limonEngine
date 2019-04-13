//
// Created by engin on 26.07.2018.
//

#ifndef LIMONENGINE_GUIIMAGEBASE_H
#define LIMONENGINE_GUIIMAGEBASE_H


#include "GUIRenderable.h"

class AssetManager;
class TextureAsset;

class GUIImageBase : public GUIRenderable {
    int imageAttachPoint = 1;

    static std::shared_ptr<GLSLProgram> imageRenderProgram;
protected:
    AssetManager* assetManager;
    std::string imageFile;
    TextureAsset *image;
public:
    GUIImageBase(GLHelper *glHelper, AssetManager *assetManager, const std::string &imageFile);
    virtual ~GUIImageBase();

    virtual void render() override;

    void getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const override;


};


#endif //LIMONENGINE_GUIIMAGEBASE_H
