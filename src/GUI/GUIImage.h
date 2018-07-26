//
// Created by engin on 26.07.2018.
//

#ifndef LIMONENGINE_GUIIMAGE_H
#define LIMONENGINE_GUIIMAGE_H


#include "GUIRenderable.h"

class AssetManager;
class TextureAsset;

class GUIImage : public GUIRenderable {
    AssetManager* assetManager;
    std::string imageFile;
    TextureAsset *image;
    int imageAttachPoint = 1;

    static GLSLProgram* imageRenderProgram;

public:
    GUIImage(GLHelper *glHelper, AssetManager *assetManager, const std::string &imageFile);
    virtual ~GUIImage();

    void render() override;

    void getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const override;


};


#endif //LIMONENGINE_GUIIMAGE_H
