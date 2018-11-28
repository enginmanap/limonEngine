//
// Created by engin on 26.11.2018.
//

#ifndef LIMONENGINE_MODELGROUP_H
#define LIMONENGINE_MODELGROUP_H


#include "../PhysicalRenderable.h"
#include "../WorldLoader.h"

class ModelGroup : public PhysicalRenderable, public GameObject {
    std::vector<PhysicalRenderable*> renderables;
    uint32_t worldObjectID;
    std::string name;
public:

    ModelGroup(GLHelper* glHelper, uint32_t worldObjectID, const std::string& name)
    : PhysicalRenderable(glHelper, 0, true), worldObjectID(worldObjectID), name(name) {
        transformation.setUpdateCallback(nullptr);
    }

    GameObject::ObjectTypes getTypeID() const {
        return MODEL_GROUP;
    }

    std::string getName() const {
        return name;
    }

    uint32_t getWorldObjectID() const override {
        return worldObjectID;
    }

    const std::vector<PhysicalRenderable *> &getRenderables() const {
        return renderables;
    }

    void addToGroup(PhysicalRenderable* renderable) {
        glm::vec3 averageTranslateDifference(0.0f, 0.0f, 0.0f);
        if(renderables.size() != 0) {
            for (auto iterator = renderables.begin(); iterator != renderables.end(); ++iterator) {
                averageTranslateDifference += (*iterator)->getTransformation()->getTranslateSingle();//single because we are the parent
            }
        }

        averageTranslateDifference += renderable->getTransformation()->getTranslateSingle() - this->transformation.getTranslate();//if already had parent, don't use it

        averageTranslateDifference = averageTranslateDifference * (1.0f / (renderables.size()+1)); //+1 because new renderable is not put in yet
        //at this point, we know where the model group imguizmo should be, now move it, and update old children

        glm::vec3 difference = averageTranslateDifference;
        for (auto iterator = renderables.begin(); iterator != renderables.end(); ++iterator) {
            (*iterator)->getTransformation()->setTransformationsNotPropagate((*iterator)->getTransformation()->getTranslateSingle() - difference);
        }

        this->getTransformation()->addTranslate(averageTranslateDifference);
        glm::vec3 translate, scale;
        glm::quat orientation;
        this->transformation.getDifference(*renderable->getTransformation(), translate, scale,orientation);
        //renderable->getTransformation()->getDifference(this->transformation, translate, scale,orientation);

        renderable->getTransformation()->setTranslate(translate);
        renderable->getTransformation()->setScale(scale);
        renderable->getTransformation()->setOrientation(orientation);
        renderable->getTransformation()->setParentTransform(&this->transformation);
        renderable->setParentObject(this);

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
                renderable->setParentObject(nullptr);
                return true;
            }
        }
        return false;
    }

    void renderWithProgram(GLSLProgram &program) override;

    void fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const override;

    static ModelGroup *deserialize(GLHelper *glHelper, AssetManager *assetManager, tinyxml2::XMLElement *ModelGroupsNode,
                                       std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds,
                                       std::map<uint32_t, ModelGroup *> &childGroups,
                                       std::vector<std::unique_ptr<WorldLoader::ObjectInformation>> &childObjects, LimonAPI *limonAPI,
                                       ModelGroup *parentGroup = nullptr);

    void render() override;

    void setupForTime(long time) override;

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;

};


#endif //LIMONENGINE_MODELGROUP_H
