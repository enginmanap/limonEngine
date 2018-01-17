//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_MODEL_H
#define LIMONENGINE_MODEL_H


#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <bullet/BulletCollision/CollisionShapes/btShapeHull.h>
#include "Utils/AssimpUtils.h"

//TODO maybe we should not have direct dependency to glm and gl
#include "glm/glm.hpp"
#include "PhysicalRenderable.h"
#include "Assets/TextureAsset.h"
#include "Material.h"
#include "Assets/ModelAsset.h"


class Model : public PhysicalRenderable {

    struct MeshMeta {
        MeshAsset* mesh;
        BoneNode* skeleton;
        GLSLProgram* program;

        MeshMeta() : mesh(NULL), skeleton(NULL), program(NULL) {}
    };

    AssetManager *assetManager;
    ModelAsset *modelAsset;
    int animationIndex = 0;
    std::string name;
    bool animated = false;
    std::string objectType;
    std::vector<glm::mat4> boneTransforms;
    std::map<uint_fast32_t, uint_fast32_t> boneIdCompoundChildMap;

    std::vector<MeshMeta *> meshes;

    btCompoundShape *compoundShape;
    const float mass;

    std::map<std::string, Material *> materialMap;
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

    void setAnimationIndex(int animationIndex) {
        this->animationIndex = animationIndex;
    }

    //TODO we need to free the texture. Destructor needed.
    ~Model() {
        delete rigidBody->getMotionState();
        delete rigidBody;
        delete compoundShape;

        for (int i = 0; i < meshes.size(); ++i) {
            delete meshes[i];
        }

    }
};

#endif //LIMONENGINE_MODEL_H
