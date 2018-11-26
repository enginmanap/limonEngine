//
// Created by engin on 26.11.2018.
//

#ifndef LIMONENGINE_MODELGROUP_H
#define LIMONENGINE_MODELGROUP_H


#include "../PhysicalRenderable.h"

class ModelGroup : public PhysicalRenderable, public GameObject {
    std::vector<PhysicalRenderable*> renderables;
    uint32_t worldObjectID;
    std::string name;
public:

    ModelGroup(GLHelper* glHelper, uint32_t worldObjectID, const std::string& name)
    : PhysicalRenderable(glHelper, 0, true), worldObjectID(worldObjectID), name(name) {
        transformation.setUpdateCallback(nullptr);
    }

    ObjectTypes getTypeID() const override;

    std::string getName() const override;

    uint32_t getWorldObjectID() override;

    void addToGroup(PhysicalRenderable* renderable) {

        glm::vec3 translate, scale;
        glm::quat orientation;
        this->transformation.getDifference(*renderable->getTransformation(), translate, scale,orientation);
        //renderable->getTransformation()->getDifference(this->transformation, translate, scale,orientation);

        renderable->getTransformation()->setTranslate(translate);
        renderable->getTransformation()->setScale(scale);
        renderable->getTransformation()->setOrientation(orientation);
        renderable->getTransformation()->setParentTransform(&this->transformation);


        renderables.push_back(renderable);
    }

    bool removeFromGroup(PhysicalRenderable* renderable) {
        for (auto element = renderables.begin(); element != renderables.end(); ++element) {
            if((*element) == renderable) {
                renderables.erase(element);
                renderable->getTransformation()->removeParentTransform();
                renderable->getTransformation()->addTranslate(this->getTransformation()->getTranslateSingle());
                renderable->getTransformation()->addScale(this->getTransformation()->getScale());
                renderable->getTransformation()->addOrientation(this->getTransformation()->getOrientation());
                return true;
            }
        }
        return false;
    }

    void renderWithProgram(GLSLProgram &program) override;

    void fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const override;

    void render() override;

    void setupForTime(long time) override;

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;
};


#endif //LIMONENGINE_MODELGROUP_H
