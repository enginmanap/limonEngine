//
// Created by engin on 26.07.2018.
//

#include "GUIImage.h"
#include "../Assets/AssetManager.h"
#include "../Assets/TextureAsset.h"

GLSLProgram* GUIImage::imageRenderProgram = nullptr;


GUIImage::GUIImage(GLHelper *glHelper, AssetManager *assetManager, const std::string &imageFile) : GUIRenderable(glHelper), assetManager(assetManager), imageFile(imageFile) {
    image = assetManager->loadAsset<TextureAsset>({imageFile});
    if(imageRenderProgram == nullptr) {
        imageRenderProgram = new GLSLProgram(glHelper, "./Data/Shaders/GUI/vertexImage.glsl", "./Data/Shaders/GUI/fragmentImage.glsl", false);
    }
    this->renderProgram = imageRenderProgram;

    this->setScale(image->getHeight() /2.0f,image->getWidth() /2.0f);// split in half, because the quad is -1 to 1, meaning it is 2 units long.
}

GUIImage::~GUIImage() {
    assetManager->freeAsset({imageFile});
}

void GUIImage::render() {

    renderProgram->setUniform("orthogonalProjectionMatrix", glHelper->getOrthogonalProjectionMatrix());

        if (!renderProgram->setUniform("worldTransformMatrix", this->getTransformation()->getWorldTransform())) {
            std::cerr << "failed to set uniform \"worldTransformMatrix\"" << std::endl;
        }

        if (!renderProgram->setUniform("GUISampler", imageAttachPoint)) {
            std::cerr << "failed to set uniform \"GUISampler\"" << std::endl;
        }
        glHelper->attachTexture(image->getID(), imageAttachPoint);
        glHelper->render(renderProgram->getID(), vao, ebo, (const GLuint) (faces.size() * 3));
}

void GUIImage::getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const {
    Transformation temp = transformation;
    glm::vec4 upRight   = (temp.getWorldTransform() * glm::vec4( 1.0f / 2.0f,  1.0f/ 2.0f, 0.0f, 1.0f));
    glm::vec4 downLeft  = (temp.getWorldTransform() * glm::vec4(-1.0f / 2.0f, -1.0f/ 2.0f, 0.0f, 1.0f));

    //it is possible with rotation the up value to be lower, right value to be more left then left. assign by check;
    aabbMin.x = std::min(downLeft.x, upRight.x);
    aabbMin.y = std::min(downLeft.y, upRight.y);

    aabbMax.x = std::max(downLeft.x, upRight.x);
    aabbMax.y = std::max(downLeft.y, upRight.y);
}
