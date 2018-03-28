//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_MODEL_H
#define LIMONENGINE_MODEL_H


#include <vector>
#include <bullet/BulletCollision/CollisionShapes/btShapeHull.h>

#include "glm/glm.hpp"
#include "../PhysicalRenderable.h"
#include "../Assets/TextureAsset.h"
#include "../Material.h"
#include "../Assets/ModelAsset.h"
#include "../../libs/ImGui/imgui.h"

class Actor;

class Model : public PhysicalRenderable, public GameObject {

    uint32_t objectID;
    struct MeshMeta {
        MeshAsset* mesh;
        BoneNode* skeleton;
        GLSLProgram* program;

        MeshMeta() : mesh(nullptr), skeleton(nullptr), program(nullptr) {}
    };
    Actor *AIActor = nullptr;
    AssetManager *assetManager;
    ModelAsset *modelAsset;
    std::string animationName;
    long animationTime = 0;
    long lastSetupTime = 0;
    float animationTimeScale = 1.0f;
    std::string name;
    bool animated = false;
    std::vector<glm::mat4> boneTransforms;
    std::map<uint_fast32_t, uint_fast32_t> boneIdCompoundChildMap;

    std::vector<MeshMeta *> meshMetaData;

    btCompoundShape *compoundShape;
    std::unordered_map<std::string, Material *> materialMap;
    int diffuseMapAttachPoint = 1;
    int ambientMapAttachPoint = 2;
    int specularMapAttachPoint = 3;
    int opacityMapAttachPoint = 4;
    uint_fast32_t triangleCount;

    void generateWorldTransform() {
        this->oldWorldTransform = this->worldTransform;
        //if animated, then the transform information will be updated according to bone transforms. Then ve apply current center offset
            this->worldTransform = glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation) *
                                   glm::scale(glm::mat4(1.0f), scale) * glm::translate(glm::mat4(1.0f), -1.0f * centerOffset);
        isDirty = false;
    }

public:
    Model(uint32_t objectID, AssetManager *assetManager, const std::string &modelFile) : Model(objectID, assetManager,
                                                                                               0, modelFile) {};

    Model(uint32_t objectID, AssetManager *assetManager, const float mass, const std::string &modelFile);

    void activateMaterial(const Material *material, GLSLProgram *program);

    bool setupRenderVariables(GLSLProgram *program);

    void setupForTime(long time);

    void render();

    void renderWithProgram(GLSLProgram &program);

    bool isAnimated() const { return animated;}

    float getMass() const { return mass;}

    void setAnimation(const std::string& animationName) {
        this->animationName = animationName;
        this->animationTime = 0;
    }

    ~Model();

    void fillObjects(tinyxml2::XMLDocument& document, tinyxml2::XMLElement * objectsNode) const;

    /************Game Object methods **************/
    uint32_t getWorldObjectID() {
        return objectID;
    }
    ObjectTypes getTypeID() const {
        return GameObject::MODEL;
    };

    std::string getName() const {
        return name;
    };

    void addImGuiEditorElements() {
        bool updated = false;
        bool crudeUpdated = false;
        static glm::vec3 preciseTranslatePoint = this->translate;
        ImGui::Text("%s",getName().c_str());                           // Some text (you can use a format string too)
        updated = ImGui::SliderFloat("Precise Position X", &(this->translate.x), preciseTranslatePoint.x - 5.0f, preciseTranslatePoint.x + 5.0f)   || updated;
        updated = ImGui::SliderFloat("Precise Position Y", &(this->translate.y), preciseTranslatePoint.y - 5.0f, preciseTranslatePoint.y + 5.0f)   || updated;
        updated = ImGui::SliderFloat("Precise Position Z", &(this->translate.z), preciseTranslatePoint.z - 5.0f, preciseTranslatePoint.z + 5.0f)   || updated;
        ImGui::NewLine();
        crudeUpdated = ImGui::SliderFloat("Crude Position X", &(this->translate.x), -100.0f, 100.0f)   || crudeUpdated;
        crudeUpdated = ImGui::SliderFloat("Crude Position Y", &(this->translate.y), -100.0f, 100.0f)   || crudeUpdated;
        crudeUpdated = ImGui::SliderFloat("Crude Position Z", &(this->translate.z), -100.0f, 100.0f)   || crudeUpdated;
        ImGui::NewLine();
        glm::vec3 tempScale(this->scale);
        updated = ImGui::SliderFloat("Scale X", &(tempScale.x), 0.01f, 10.0f)             || updated;
        updated = ImGui::SliderFloat("Scale Y", &(tempScale.y), 0.01f, 10.0f)             || updated;
        updated = ImGui::SliderFloat("Scale Z", &(tempScale.z), 0.01f, 10.0f)             || updated;
        ImGui::NewLine();
        updated = ImGui::SliderFloat("Rotate X", &(this->orientation.x), -1.0f, 1.0f)             || updated;
        updated = ImGui::SliderFloat("Rotate Y", &(this->orientation.y), -1.0f, 1.0f)             || updated;
        updated = ImGui::SliderFloat("Rotate Z", &(this->orientation.z), -1.0f, 1.0f)             || updated;
        updated = ImGui::SliderFloat("Rotate W", &(this->orientation.w), -1.0f, 1.0f)             || updated;

        if(updated || crudeUpdated) {
            this->setTranslate(translate);
            this->setScale(tempScale);
            this->setOrientation(orientation);
            this->rigidBody->activate();
        }
        if(crudeUpdated) {
            preciseTranslatePoint = this->translate;
        }

        if(isAnimated()) {
            ImGui::Text("Animation properties");                           // Some text (you can use a format string too)
            if (ImGui::BeginCombo("Animation Name", animationName.c_str())) {
                //ImGui::Combo();
                for (std::unordered_map<std::string, AnimationSet *>::const_iterator it = modelAsset->getAnimations().begin();
                     it != modelAsset->getAnimations().end(); it++) {
                    if(ImGui::Selectable(it->first.c_str())) {
                        setAnimation(it->first);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SliderFloat("Animation time scale", &(this->animationTimeScale), 0.01f, 2.0f);
        }
    };
    /************Game Object methods **************/
    void attachAI(Actor *AIActor) {
        //after this, clearing the AI is job of the model.
        this->AIActor = AIActor;
    }

    uint32_t getAIID();

    void detachAI() {
        this->AIActor = nullptr;
    }


};

#endif //LIMONENGINE_MODEL_H
