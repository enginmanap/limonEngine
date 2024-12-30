//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_MODEL_H
#define LIMONENGINE_MODEL_H


#include <vector>
#include <set>

#include "../PhysicalRenderable.h"
#include "../Assets/TextureAsset.h"
#include "../Material.h"
#include "../Assets/ModelAsset.h"
#include "../../libs/ImGui/imgui.h"
#include "GameObject.h"

#include "Sound.h"

class ActorInterface;
class RenderList;
class Model : public PhysicalRenderable, public GameObject {
public:
    struct MeshMeta {
        std::shared_ptr<MeshAsset> mesh = nullptr;
        std::shared_ptr<const Material> material = nullptr;
    };
private:
    uint32_t objectID;
    uint32_t rigID = 0; //initialize as 0, because thats the value for non animated.

    ActorInterface *AIActor = nullptr;
    std::shared_ptr<AssetManager> assetManager;
    std::shared_ptr<ModelAsset> modelAsset;
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
    std::vector<LimonTypes::GenericParameter> aiParameters;
    std::string lastSelectedAIName;
    std::vector<glm::mat4> boneTransforms;
    std::map<uint32_t, uint32_t> boneIdCompoundChildMap;

    std::vector<MeshMeta *> meshMetaData;
    std::shared_ptr<Sound> stepOnSound = nullptr;

    btCompoundShape *compoundShape;
    btDefaultMotionState *motionState;
    std::vector<btCollisionShape *> childrenPhysicsShapes;

    uint32_t triangleCount;
    int32_t selectedBoneID = -1;
    std::map<uint32_t, Transformation*> exposedBoneTransforms;


    static ImGuiResult putAIonGUI(ActorInterface *actorInterface, std::vector<LimonTypes::GenericParameter> &parameters,
                                  const ImGuiRequest &request, std::string &lastSelectedAIName);

    /**
     * This method returns a list of MeshAsset->Material pairs, that has been customized specifically for this model, that is not shared with the
     * ModelAsset. Model asset has a default material, and world has overrides for that asset. This method returns overrides for not the asset level,
     * but the model level.
     *
     * This method will check one by one all meshes and compare with the asset. That is because we don't want to make the Model object use more memory
     * for something thats expected to apply only a very small portion of the models.
     * Consider this a slow method.
     *
     * @return list of meshes that had different material than asset
     */
    std::vector<std::pair<std::string, std::shared_ptr<const Material>>> getNewMeshMaterials() const;

public:
    void loadOverriddenMeshMaterial(std::vector<std::pair<std::string, std::shared_ptr<Material>>> & customisedMeshMaterialList);

    void setRigId(uint32_t rigId) {
        this->rigID = rigId;
    }

    uint32_t getRigId() const {
        return rigID;
    }

    Model(uint32_t objectID,  std::shared_ptr<AssetManager> assetManager, const std::string &modelFile) : Model(objectID, assetManager,
                                                                                                                0, modelFile, false) {};

    Model(uint32_t objectID,  std::shared_ptr<AssetManager> assetManager, const float mass, const std::string &modelFile,
              bool disconnected);

    Model(const Model& otherModel, uint32_t objectID); //kind of copy constructor, except ID

    void transformChangeCallback() {
        PhysicalRenderable::updatePhysicsFromTransform();
        graphicsWrapper->setModel(this->getWorldObjectID(), this->transformation.getWorldTransform());
    }

    void updateTransformFromPhysics() override {
        PhysicalRenderable::updateTransformFromPhysics();
        graphicsWrapper->setModel(this->getWorldObjectID(), this->transformation.getWorldTransform());
    }

    void activateTexturesOnly(std::shared_ptr<const Material> material) const;

    bool setupRenderVariables(MeshMeta *meshMetaData);

    const std::vector<MeshMeta *> &getMeshMetaData() const { return meshMetaData; }

    void setupForTime(long time) override;

    void renderWithProgram(std::shared_ptr<GraphicsProgram> program, uint32_t lodLevel) override;

    RenderList convertToRenderList(uint32_t lodLevel, float depth) const;
    void renderWithProgramInstanced(const std::vector<glm::uvec4> & modelIndices, GraphicsProgram &program, uint32_t lodLevel);

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

    ~Model() override;

    bool fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const override;

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
    ObjectTypes getTypeID() const override {
        return GameObject::MODEL;
    }

     const std::vector<glm::mat4>* getBoneTransforms() const {return &boneTransforms;}

    std::string getName() const override {
        return name + "_" + std::to_string(objectID);
    }

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;
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

    std::vector<std::shared_ptr<Material>> getMaterials() const override {
        std::vector<std::shared_ptr<Material>> materials;
        for(const auto& element:this->modelAsset->getMaterialMap()){
            materials.emplace_back(element.second);
        }
        return materials;
    }

    void addTag(const std::string& text) override {
        this->dirtyForFrustum = true;
        GameObject::addTag(text);
    }

    void removeTag(const std::string& text) override {
        this->dirtyForFrustum = true;
        GameObject::removeTag(text);
    }


};

#endif //LIMONENGINE_MODEL_H
