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
#include "GameObject.h"

#include "Sound.h"

class Actor;

class Model : public PhysicalRenderable, public GameObject {
    uint32_t objectID;
    struct MeshMeta {
        MeshAsset* mesh = nullptr;
        GLSLProgram* program = nullptr;
    };
    Actor *AIActor = nullptr;
    AssetManager *assetManager;
    ModelAsset *modelAsset;
    std::string animationName;
    long animationTime = 0;
    bool animationLooped = true;
    bool animationLastFramePlayed = false;
    long lastSetupTime = 0;
    float animationTimeScale = 1.0f;
    std::string name;
    bool animated = false;
    std::vector<glm::mat4> boneTransforms;
    std::map<uint_fast32_t, uint_fast32_t> boneIdCompoundChildMap;

    std::vector<MeshMeta *> meshMetaData;
    std::shared_ptr<Sound> stepOnSound = nullptr;
    char stepOnSoundNameBuffer[128] = {};

    btCompoundShape *compoundShape;
    std::unordered_map<std::string, Material *> materialMap;
    int diffuseMapAttachPoint = 1;
    int ambientMapAttachPoint = 2;
    int specularMapAttachPoint = 3;
    int opacityMapAttachPoint = 4;
    uint_fast32_t triangleCount;

public:
    Model(uint32_t objectID, AssetManager *assetManager, const std::string &modelFile) : Model(objectID, assetManager,
                                                                                               0, modelFile, false) {};

    Model(uint32_t objectID, AssetManager *assetManager, const float mass, const std::string &modelFile,
              bool disconnected);

    Model(const Model& otherModel, uint32_t objectID); //kind of copy constructor, except ID

    void transformChangeCallback() {
        PhysicalRenderable::updatePhysicsFromTransform();
        glHelper->setModel(this->getWorldObjectID(), this->transformation.getWorldTransform());
    }

    void updateTransformFromPhysics() override {
        PhysicalRenderable::updateTransformFromPhysics();
        glHelper->setModel(this->getWorldObjectID(), this->transformation.getWorldTransform());
    }

    void setSamplersAndUBOs(GLSLProgram *program);
    void activateTexturesOnly(const Material *material);

    bool setupRenderVariables(MeshMeta *meshMetaData);

    void setupForTime(long time);

    void render();

    void renderWithProgram(GLSLProgram &program);

    void renderInstanced(std::vector<uint32_t> &modelIndices);

    void renderWithProgramInstanced(std::vector<uint32_t> &modelIndices, GLSLProgram &program);

    bool isAnimated() const { return animated;}

    float getMass() const { return mass;}

    void setAnimation(const std::string &animationName, bool looped = true) {
        this->animationName = animationName;
        this->animationTime = 0;

        this->animationLooped = looped;
        this->animationLastFramePlayed = false;
    }

    bool isAnimationFinished() {
        return animationLastFramePlayed;
    }

    ~Model();

    void fillObjects(tinyxml2::XMLDocument& document, tinyxml2::XMLElement * objectsNode) const;

    std::shared_ptr<Sound> &getPlayerStepOnSound() {
        return stepOnSound;
    }

    void setPlayerStepOnSound(std::shared_ptr<Sound> stepOnSound) {
        this->stepOnSound = stepOnSound;

        if (this->stepOnSound != nullptr) {
            this->stepOnSound->setLoop(true);
            if (this->stepOnSound->getName().length() < 128) {
                strcpy(stepOnSoundNameBuffer, this->stepOnSound->getName().c_str());
            } else {
                strncpy(stepOnSoundNameBuffer, this->stepOnSound->getName().c_str(), 127);
            }
        }

    }

    /************Game Object methods **************/
    uint32_t getWorldObjectID() {
        return objectID;
    }
    ObjectTypes getTypeID() const {
        return GameObject::MODEL;
    };

    std::string getName() const {
        return name + "_" + std::to_string(objectID);
    };

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request);
    /************Game Object methods **************/

    void attachAI(Actor *AIActor) {
        //after this, clearing the AI is job of the model.
        this->AIActor = AIActor;
    }

    uint32_t getAIID();

    void detachAI() {
        this->AIActor = nullptr;
    }

    uint32_t getAssetID() {
        return modelAsset->getAssetID();
    }

};

#endif //LIMONENGINE_MODEL_H
