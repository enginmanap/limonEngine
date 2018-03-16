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


class Model : public PhysicalRenderable, public GameObject {

    struct MeshMeta {
        MeshAsset* mesh;
        BoneNode* skeleton;
        GLSLProgram* program;

        MeshMeta() : mesh(nullptr), skeleton(nullptr), program(nullptr) {}
    };
    AssetManager *assetManager;
    ModelAsset *modelAsset;
    std::string animationName;
    std::string name;
    bool animated = false;
    std::vector<glm::mat4> boneTransforms;
    std::map<uint_fast32_t, uint_fast32_t> boneIdCompoundChildMap;

    std::vector<MeshMeta *> meshes;

    btCompoundShape *compoundShape;
    const float mass;

    std::unordered_map<std::string, Material *> materialMap;
    int diffuseMapAttachPoint = 1;
    int ambientMapAttachPoint = 2;
    int specularMapAttachPoint = 3;
    int opacityMapAttachPoint = 4;
    uint_fast32_t triangleCount;


public:
    Model(AssetManager *assetManager, const std::string &modelFile) : Model(assetManager, 0, modelFile) {};

    Model(AssetManager *assetManager, const float mass, const std::string &modelFile);

    void activateMaterial(const Material *material, GLSLProgram *program);

    bool setupRenderVariables(GLSLProgram *program);

    void setupForTime(long time);

    void render();

    void renderWithProgram(GLSLProgram &program);

    bool isAnimated() const { return animated;}

    float getMass() const { return mass;}

    void setAnimation(const std::string& animationName) {
        this->animationName = animationName;
    }

    //TODO we need to free the texture. Destructor needed.
    ~Model() {
        delete rigidBody->getMotionState();
        delete rigidBody;
        delete compoundShape;

        for (unsigned int i = 0; i < meshes.size(); ++i) {
            delete meshes[i];
        }

    }

    /************Game Object methods **************/
    ObjectTypes getTypeID() const {
        return GameObject::MODEL;
    };

    std::string getName() const {
        return name;
    };

    void addImGuiEditorElements() {
        bool updated = false;
        ImGui::Text("%s",getName().c_str());                           // Some text (you can use a format string too)
        updated = ImGui::SliderFloat("Position X", &(this->translate.x), -100.0f, 100.0f)   || updated;
        updated = ImGui::SliderFloat("Position Y", &(this->translate.y), -100.0f, 100.0f)   || updated;
        updated = ImGui::SliderFloat("Position Z", &(this->translate.z), -100.0f, 100.0f)   || updated;

        updated = ImGui::SliderFloat("Scale X", &(this->scale.x), 0.01f, 10.0f)             || updated;
        updated = ImGui::SliderFloat("Scale Y", &(this->scale.y), 0.01f, 10.0f)             || updated;
        updated = ImGui::SliderFloat("Scale Z", &(this->scale.z), 0.01f, 10.0f)             || updated;

        if(updated) {
            this->setTranslate(translate);
            this->setScale(scale);
            this->rigidBody->activate();
        }
    };
    /************Game Object methods **************/
};

#endif //LIMONENGINE_MODEL_H
