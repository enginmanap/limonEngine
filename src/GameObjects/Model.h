//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_MODEL_H
#define LIMONENGINE_MODEL_H


#include <vector>
#include <set>
#include <bullet/BulletCollision/CollisionShapes/btShapeHull.h>

#include "glm/glm.hpp"
#include "../PhysicalRenderable.h"
#include "../Assets/TextureAsset.h"
#include "../Material.h"
#include "../Assets/ModelAsset.h"
#include "../../libs/ImGui/imgui.h"
#include "GameObject.h"

#include "Sound.h"

class ActorInterface;

class Model : public PhysicalRenderable, public GameObject {
    uint32_t objectID;
    struct MeshMeta {
        std::shared_ptr<MeshAsset> mesh = nullptr;
    };
    ActorInterface *AIActor = nullptr;
    AssetManager *assetManager;
    ModelAsset *modelAsset;
private:
    std::string animationName;
    long animationTime = 0;
    bool animationLooped = true;

    std::string animationNameOld;
    long animationTimeOld = 0;
    bool animationLoopedOld = true;

    bool animationBlend = false;
    long animationBlendTime = 1000;

    bool animationLastFramePlayed = false;
    long lastSetupTime = 0;
    float animationTimeScale = 1.0f;
    std::string name;
    bool animated = false;
    bool isAIParametersDirty = true;
    bool temporary = false;
    std::vector<LimonAPI::ParameterRequest> aiParameters;
    std::string lastSelectedAIName;
    std::vector<glm::mat4> boneTransforms;
    std::map<uint_fast32_t, uint_fast32_t> boneIdCompoundChildMap;

    std::vector<MeshMeta *> meshMetaData;
    std::shared_ptr<Sound> stepOnSound = nullptr;

    btCompoundShape *compoundShape;
    std::unordered_map<std::string, std::shared_ptr<Material>> materialMap;
    int diffuseMapAttachPoint = 1;
    int ambientMapAttachPoint = 2;
    int specularMapAttachPoint = 3;
    int opacityMapAttachPoint = 4;
    int normalMapAttachPoint = 5;
    uint_fast32_t triangleCount;
    int32_t selectedBoneID = -1;
    std::map<uint32_t, Transformation*> exposedBoneTransforms;

    static ImGuiResult putAIonGUI(ActorInterface *actorInterface, std::vector<LimonAPI::ParameterRequest> &parameters,
                                  const ImGuiRequest &request, std::string &lastSelectedAIName);

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

    void activateTexturesOnly(std::shared_ptr<const Material> material);

    bool setupRenderVariables(MeshMeta *meshMetaData);

    void setupForTime(long time);

    void render();

    void renderWithProgram(GLSLProgram &program);


    void renderWithProgramInstanced(std::vector<uint32_t> &modelIndices, GLSLProgram &program);

    bool isAnimated() const { return animated;}

    float getMass() const { return mass;}

    void setAnimation(const std::string &animationName, bool looped = true) {
        this->animationName = animationName;
        this->animationTime = 0;

        this->animationLooped = looped;
        this->animationLastFramePlayed = false;
    }

    void setAnimationWithBlend(const std::string &animationName, bool looped = true, long blendTime = 100) {
        this->animationNameOld = this->animationName;
        this->animationTimeOld = this->animationTime;
        this->animationLoopedOld = this->animationLooped;

        this->animationName = animationName;
        this->animationTime = 0;
        this->animationLooped = looped;

        this->animationBlendTime = blendTime;
        this->animationBlend = true;

        this->animationLastFramePlayed = false;
    }

    std::string getAnimationName() {
        return this->animationName;
    }

    bool isAnimationFinished() {
        return animationLastFramePlayed;
    }

    float getAnimationTimeScale() const {
        return animationTimeScale;
    }

    void setAnimationTimeScale(float animationTimeScale) {
        Model::animationTimeScale = animationTimeScale;
    }

    bool isTransparent() const {
        return modelAsset->isTransparent();
    }

    ~Model();

    bool fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const;

    std::shared_ptr<Sound> &getPlayerStepOnSound() {
        return stepOnSound;
    }

    void setPlayerStepOnSound(std::shared_ptr<Sound> stepOnSound) {
        this->stepOnSound = stepOnSound;

        this->stepOnSound->changeGain(0.125f);

        if (this->stepOnSound != nullptr) {
            this->stepOnSound->setLoop(true);
        }

    }

    /************Game Object methods **************/
    uint32_t getWorldObjectID() const override {
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

    void attachAI(ActorInterface *AIActor);

    uint32_t getAIID();

    void detachAI() {
        this->AIActor = nullptr;
    }

    uint32_t getAssetID() {
        return modelAsset->getAssetID();
    }

    void convertAssetToLimon(std::set<std::vector<std::string>>& convertedAssetsSet);

    /**
     * This method allows attachment to a specific bone of the model, if a bone is selected. If no bone is selected, world transform is returned.
     * If a bone is returned, bone id is set to the parameter attachedBone. If no bone is selected and world transform is returned,
     * parameter will contain -1
     *
     * If the selected bone is not exposed, it creates a transform to expose.
     * @param attachedBone -1 is no bone is selected, bone id if bone is selected
     * @return pointer to selected bones transform or world transform of the object
     */
    Transformation* getAttachmentTransform(int32_t &attachedBone) {
        if(selectedBoneID == -1) {
            attachedBone = -1;
            return &(this->transformation);
        }
        //return bones transformation, create expose transform if there is none.
        if(exposedBoneTransforms.find(selectedBoneID) == exposedBoneTransforms.end()) {
            exposedBoneTransforms[selectedBoneID] = new Transformation();
            glm::vec3 temp1;//these are not used
            glm::vec4 temp2;
            glm::vec3 translate, scale;
            glm::quat orientation;

            glm::decompose(this->transformation.getWorldTransform() * boneTransforms[selectedBoneID], scale, orientation, translate, temp1, temp2);//update the current of thes

            exposedBoneTransforms[selectedBoneID]->setTranslate(translate);
            exposedBoneTransforms[selectedBoneID]->setScale(scale);
            exposedBoneTransforms[selectedBoneID]->setOrientation(orientation);
        }
        attachedBone = selectedBoneID;
        return exposedBoneTransforms[selectedBoneID];
    }

    /**
     * This method is only to be used by WorldLoader, and even it is not proper, and should be removed.
     * @param attachedBone
     * @return
     */
    Transformation* getAttachmentTransformForKnownBone(int32_t attachmentBoneID) {
        selectedBoneID = attachmentBoneID;
        return getAttachmentTransform(attachmentBoneID);
    }

    bool isTemporary() const {
        return temporary;
    }

    void setTemporary(bool temporary) {
        this->temporary = temporary;
    }
};

#endif //LIMONENGINE_MODEL_H
