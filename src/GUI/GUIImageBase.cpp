//
// Created by engin on 26.07.2018.
//

#include "GUIImageBase.h"
#include "../Assets/AssetManager.h"
#include "../Assets/TextureAsset.h"

std::shared_ptr<GLSLProgram> GUIImageBase::imageRenderProgram = nullptr;


GUIImageBase::GUIImageBase(OpenGLGraphics *glHelper, AssetManager *assetManager, const std::string &imageFile) : GUIRenderable(glHelper), assetManager(assetManager), imageFile(imageFile) {
    image = assetManager->loadAsset<TextureAsset>({imageFile});
    this->setScale(image->getHeight() /2.0f,image->getWidth() /2.0f);// split in half, because the quad is -1 to 1, meaning it is 2 units long.
}

GUIImageBase::~GUIImageBase() {
    assetManager->freeAsset({imageFile});
    //delete renderProgram;// since the program is shared, don't remove
}

void GUIImageBase::renderWithProgram(std::shared_ptr<GLSLProgram> renderProgram){

    renderProgram->setUniform("orthogonalProjectionMatrix", glHelper->getOrthogonalProjectionMatrix());

    if (!renderProgram->setUniform("worldTransformMatrix", this->getTransformation()->getWorldTransform())) {//translate.z is alpha channel, we are sending it too!
        std::cerr << "failed to set uniform \"worldTransformMatrix\"" << std::endl;
    }

    if (!renderProgram->setUniform("GUISampler", imageAttachPoint)) {
        std::cerr << "failed to set uniform \"GUISampler\"" << std::endl;
    }
    glHelper->attachTexture(image->getID(), imageAttachPoint);
    glHelper->render(renderProgram->getID(), vao, ebo, (GLuint) (faces.size() * 3));
}

void GUIImageBase::getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const {
    Transformation temp = transformation;
    glm::vec4 upRight   = (temp.getWorldTransform() * glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f));
    glm::vec4 downLeft  = (temp.getWorldTransform() * glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f));

    //it is possible with rotation the up value to be lower, right value to be more left then left. assign by check;
    aabbMin.x = std::min(downLeft.x, upRight.x);
    aabbMin.y = std::min(downLeft.y, upRight.y);

    aabbMax.x = std::max(downLeft.x, upRight.x);
    aabbMax.y = std::max(downLeft.y, upRight.y);
}
