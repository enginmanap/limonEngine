//
// Created by engin on 26.11.2018.
//

#ifndef LIMONENGINE_MODELGROUP_H
#define LIMONENGINE_MODELGROUP_H


#include "../PhysicalRenderable.h"
#include "../WorldLoader.h"


class ModelGroup : public PhysicalRenderable, public GameObject {
    uint32_t worldObjectID;
    std::string name;
public:

    ModelGroup(OpenGLGraphics* glHelper, uint32_t worldObjectID, const std::string& name)
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

    void addChild(PhysicalRenderable *renderable) override;

    bool removeChild(PhysicalRenderable* renderable) override {
        for (auto element = children.begin(); element != children.end(); ++element) {
            if((*element) == renderable) {
                children.erase(element);
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

    void renderWithProgram(std::shared_ptr<GLSLProgram> program) override;

    bool fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const override;

    static ModelGroup *deserialize(OpenGLGraphics *glHelper, AssetManager *assetManager, tinyxml2::XMLElement *ModelGroupsNode,
                                   std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds,
                                   std::map<uint32_t, ModelGroup *> &childGroups,
                                   std::vector<std::unique_ptr<WorldLoader::ObjectInformation>> &childObjects, LimonAPI *limonAPI,
                                   ModelGroup *parentGroup = nullptr);

    void setupForTime(long time) override;

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;

};


#endif //LIMONENGINE_MODELGROUP_H
