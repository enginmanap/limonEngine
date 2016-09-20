//
// Created by Engin Manap on 13.02.2016.
//

#ifndef UBERGAME_MODEL_H
#define UBERGAME_MODEL_H


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

    AssetManager *assetManager;
    ModelAsset *modelAsset;

    btCompoundShape *compoundShape;

    std::map<std::string, Material *> materialMap;
    int diffuseMapAttachPoint = 1;
    uint_fast32_t triangleCount;


public:
    Model(AssetManager *assetManager, const std::string &modelFile) : Model(assetManager, 0, modelFile) {};

    Model(AssetManager *assetManager, const float mass, const std::string &modelFile);

    void activateMaterial(const Material *material);

    bool setupRenderVariables();

    void render();

    void renderWithProgram(GLSLProgram &program);


    //TODO we need to free the texture. Destructor needed.
    ~Model() {
        delete rigidBody->getMotionState();
        delete rigidBody;

/*
        if (simplifiedConvexShape != NULL) {
            delete simplifiedConvexShape;
        }
        delete convexShape;
*/
    }


};

#endif //UBERGAME_MODEL_H
