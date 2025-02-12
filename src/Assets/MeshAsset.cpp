//
// Created by engin on 14.09.2016.
//

#include "MeshAsset.h"

#include <Utils/GLMConverter.h>

#include "API/Graphics/GraphicsInterface.h"
#include "../../libs/meshoptimizer/src/meshoptimizer.h"

MeshAsset::MeshAsset(const aiMesh *currentMesh, std::string name, std::shared_ptr<const BoneNode> meshSkeleton,
                     const glm::mat4 &parentTransform, const bool isPartOfAnimated)
        : name(name), parentTransform(parentTransform), isPartOfAnimated(isPartOfAnimated) {
    if (!currentMesh->HasPositions()) {
        throw "No position found"; //Not going to process if mesh is empty
    }

    vertexCount = currentMesh->mNumVertices;
    if(!setTriangles(currentMesh)) {
        return;
    }

    //If model is animated, but mesh has no bones, it is most likely we need to attach to the nearest parent.
    this->minAABB = parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mAABB.mMin), 1.0f);
    this->maxAABB = parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mAABB.mMax), 1.0f);
    //loadBoneInformation
    if (currentMesh->HasBones()) {
        this->bones = true;

        assert(meshSkeleton != nullptr);
        this->skeleton = meshSkeleton;
        //we will create a bone* vector so we can load the weight information to GPU. The same vector should be
        //copied and filled with animation information on Model class
        //an a name->bone* map, for animation.
        fillBoneMap(this->skeleton);

        boneIDs.resize(vertices.size());
        boneWeights.resize(vertices.size());
        for (unsigned int j = 0; j < currentMesh->mNumBones; ++j) {
            uint32_t boneID = boneIdMap[currentMesh->mBones[j]->mName.C_Str()];
            /*
             * Assimp has a bone array with weight lists for vertices,
             * we need a vertex array with weight list for bones.
             * This loop should generate that
             *
             * ADDITION
             * I want to split BulletCollision meshes to move them with real mesh, for that
             * I will use this information
             */
            for (uint32_t k = 0; k < currentMesh->mBones[j]->mNumWeights; ++k) {
                if(currentMesh->mBones[j]->mWeights[k].mWeight > 0.0f) {
                    addWeightToVertex(boneID, currentMesh->mBones[j]->mWeights[k].mVertexId,
                                      currentMesh->mBones[j]->mWeights[k].mWeight);
                    boneAttachedMeshes[boneID].push_back(currentMesh->mBones[j]->mWeights[k].mVertexId);
                }
            }


        }
        //std::cout << "Animation added for mesh" << std::endl;
    } else {
        if(isPartOfAnimated) {
            //what to do now? now we assign bone id of the node, and weight of 1.0

            this->bones = true;

            assert(meshSkeleton != nullptr);
            this->skeleton = meshSkeleton;
            //we will create a bone* vector so we can load the weight information to GPU. The same vector should be
            //copied and filled with animation information on Model class
            //an a name->bone* map, for animation.
            fillBoneMap(this->skeleton);

            boneIDs.resize(vertices.size());
            boneWeights.resize(vertices.size());
            uint32_t boneID = boneIdMap[name];
            /*
             * Assimp has a bone array with weight lists for vertices,
             * we need a vertex array with weight list for bones.
             * This loop should generate that
             *
             * ADDITION
             * I want to split BulletCollision meshes to move them with real mesh, for that
             * I will use this information
             */
            for (uint32_t k = 0; k < currentMesh->mNumVertices; ++k) {
                addWeightToVertex(boneID, k, 1.0f);
                boneAttachedMeshes[boneID].push_back(k);
            }

            //std::cout << "Animation added for mesh" << std::endl;
        } else {
            this->bones = false;
        }
    }
    buildBulletMesh();
}


void MeshAsset::loadGPUPart(AssetManager *assetManager) {
    /*** things should be set by serialize */
    //triangleCount
    //vertexCount
    // isPartOfAnimated
    //vertices
    //faces
    //normals
    //textureCoordinates
    //bones
    //skeleton
    // boneIDs
    // boneWeights
    // boneAttachedMeshes;
    // boneIDMap


    uint32_t vbo;
    assetManager->getGraphicsWrapper()->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    assetManager->getGraphicsWrapper()->bufferNormalData(normals, vao, vbo, 4);
    bufferObjects.push_back(vbo);

    if (!textureCoordinates.empty()) {
        assetManager->getGraphicsWrapper()->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 3);
        bufferObjects.push_back(vbo);
    }

    if (this->bones) {
        assetManager->getGraphicsWrapper()->bufferExtraVertexData(boneIDs, vao, vbo, 5);
        bufferObjects.push_back(vbo);

        assetManager->getGraphicsWrapper()->bufferExtraVertexData(boneWeights, vao, vbo, 6);
        bufferObjects.push_back(vbo);
    }
}

bool MeshAsset::setTriangles(const aiMesh *currentMesh) {
    //In this part, the "if"s can be put in for, but then we will check them for each iteration. I am
    // not sure if that creates enough performance difference, it can be checked.
    if(isPartOfAnimated) {
        if (currentMesh->HasTextureCoords(0)) {
            for (unsigned int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
                normals.push_back(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]));
                glm::vec2 vectorsTextureCoordinates(currentMesh->mTextureCoords[0][j].x, currentMesh->mTextureCoords[0][j].y);
                normalizeTextureCoordinates(vectorsTextureCoordinates);
                textureCoordinates.push_back(vectorsTextureCoordinates);

            }
        } else {
            for (unsigned int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
                normals.push_back(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]));
            }
        }
    } else {

        if (currentMesh->HasTextureCoords(0)) {
            for (unsigned int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]), 1.0f)));
                normals.push_back(glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]), 1.0f)));
                glm::vec2 vectorsTextureCoordinates(currentMesh->mTextureCoords[0][j].x, currentMesh->mTextureCoords[0][j].y);
                normalizeTextureCoordinates(vectorsTextureCoordinates);
                textureCoordinates.push_back(vectorsTextureCoordinates);

            }
        } else {
            for (unsigned int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]), 1.0f)));
                normals.push_back(glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]), 1.0f)));
            }
        }
    }

    for (unsigned int j = 0; j < currentMesh->mNumFaces; ++j) {
        if(currentMesh->mFaces[j].mNumIndices == 3) {
            faces.push_back(glm::vec3(currentMesh->mFaces[j].mIndices[0],
                                      currentMesh->mFaces[j].mIndices[1],
                                      currentMesh->mFaces[j].mIndices[2]));
        }
    }
    triangleCount[0] = faces.size();
    offsets[0] = 0;
    if(faces.empty()) {
        return false;
    }

    //lets try to simplify
    float threshold = 0.2f;
    size_t target_index_count = size_t(faces.size()*3 * threshold);
    float target_error = 0.001f;

    std::vector<unsigned int> lod(faces.size()*3);
    lod.resize(meshopt_simplify(&lod[0], &(faces[0].x), faces.size()*3, &vertices[0].x, vertices.size(), sizeof(glm::vec3),
                                target_index_count, target_error));
    //now we have new faces. lets assign.
    for (size_t i = 0; i <lod.size(); i = i+3) {
        faces.push_back(glm::vec3(lod[i + 0],
                                  lod[i + 1],
                                  lod[i + 2]));
    }
    triangleCount[1] = lod.size()/3;
    offsets[1] = triangleCount[0]*3;
    //std::cerr << "simplification1 result: \t" << triangleCount[0] << "\t->\t" << triangleCount[1] << std::endl;

    //lets try to simplify
    float threshold2 = 0.1f;
    size_t target_index_count2 = size_t(triangleCount[0]*3 * threshold2);
    float target_error2 = 0.01f;

    std::vector<unsigned int> lod2(triangleCount[0]*3);
    lod2.resize(meshopt_simplify(&lod2[0], &(faces[0].x), triangleCount[0]*3, &vertices[0].x, vertices.size(), sizeof(glm::vec3),
                                target_index_count2, target_error2));
    //now we have new faces. lets assign.
    for (size_t i = 0; i <lod2.size(); i = i+3) {
        faces.push_back(glm::vec3(lod2[i + 0],
                                  lod2[i + 1],
                                  lod2[i + 2]));
    }
    triangleCount[2] = lod2.size()/3;
    offsets[2] = (triangleCount[1]*3) + offsets[1];
    //std::cerr << "simplification2 result: \t" << triangleCount[1] << "\t->\t" << triangleCount[2] << std::endl;


    //lets try to simplify
    float threshold3 = 0.05f;
    size_t target_index_count3 = size_t(triangleCount[0]*3 * threshold3);
    float target_error3 = 0.5f;

    std::vector<unsigned int> lod3(triangleCount[0]*3);
    lod3.resize(meshopt_simplify(&lod3[0], &(faces[0].x), triangleCount[0]*3, &vertices[0].x, vertices.size(), sizeof(glm::vec3),
                                 target_index_count3, target_error3));
    //now we have new faces. lets assign.
    for (size_t i = 0; i <lod3.size(); i = i+3) {
        faces.push_back(glm::vec3(lod3[i + 0],
                                  lod3[i + 1],
                                  lod3[i + 2]));
    }
    triangleCount[3] = lod3.size()/3;

    offsets[3] = (triangleCount[2]*3) + offsets[2];
    //std::cerr << "simplification3 result: \t" << triangleCount[2] << "\t->\t" << triangleCount[3] << std::endl;

    //std::cerr << "after simplification triangle counts: \t" << triangleCount[0] << ", " << triangleCount[1] << ", " << triangleCount[2] << ", " << triangleCount[3] << std::endl;
    //std::cerr << "after simplification offsets: \t" << offsets[0] << ", " << offsets[1] << ", " << offsets[2] << ", " << offsets[3] << std::endl;
    return true;
}

void MeshAsset::normalizeTextureCoordinates(glm::vec2 &textureCoordinates) const {
    float fractionPart = textureCoordinates.x;
    if(fabs(textureCoordinates.x) > 1) {
        double integerPart;
        fractionPart = modf (textureCoordinates.x , &integerPart);
    }
    if(textureCoordinates.x < 0 ) {
        fractionPart = fractionPart + 1;
    }
    textureCoordinates.x = fractionPart;

    fractionPart = textureCoordinates.y;
    if(fabs(textureCoordinates.y) > 1) {
        double integerPart;
        fractionPart = modf (textureCoordinates.y , &integerPart);
    }
    if(textureCoordinates.y < 0 ) {
        fractionPart = fractionPart + 1;
    }
    textureCoordinates.y = fractionPart;
}

bool MeshAsset::addWeightToVertex(uint32_t boneID, unsigned int vertex, float weight) {
    //the weights are suppose to be ordered,
    for (int i = 0; i < 4; ++i) { //we allow only 4 bones per vertex
        if (boneWeights[vertex][i] < weight) {
            //shift to open slot
            if(boneWeights[vertex][i] > 0.0f) {
                for (int j = 3; j > i; --j) {
                    boneWeights[vertex][j] = boneWeights[vertex][j - 1];
                    boneIDs[vertex][j] = boneIDs[vertex][j - 1];
                }
            }
            boneWeights[vertex][i] = weight;
            boneIDs[vertex][i] = boneID;
            return true;
        }
    }
    return false;
}

/**
 * this should return either a map of boneid<->hull or if no animation btTriangleMesh.
 * this way, we can use animation transforms to move the individual hulls with the bones.
 * @param compoundShape
 * @return
 */
btTriangleMesh *MeshAsset::getBulletMesh(std::map<uint32_t, btConvexHullShape *> *hullMap,
                                         std::map<uint32_t, btTransform> *parentTransformMap) {
    //Turns out bullet shapes does not copy meshes, so we should return a copy, not the original;
    btTriangleMesh *copyMesh = nullptr;
    if(!isPartOfAnimated) {
        copyMesh = new btTriangleMesh(bulletMesh);
        shapeCopies.push_back(copyMesh);
    } else {
        //in this case, we don't use faces directly, instead we use per bone vertex information.
        std::map<uint32_t, std::vector<uint32_t>>::iterator it;
        for (it = boneAttachedMeshes.begin(); it != boneAttachedMeshes.end(); it++) {
            (*hullMap)[it->first] = bulletHullMap[it->first];
            (*parentTransformMap)[it->first] = bulletParentTransformMap[it->first];
        }

    }
    return copyMesh;
}

/**
 * this should return either a map of boneid<->hull or if no animation btTriangleMesh.
 * this way, we can use animation transforms to move the individual hulls with the bones.
 * @param compoundShape
 * @return
 */
void MeshAsset::buildBulletMesh() {
    if(!isPartOfAnimated) {
        //if not part of an animation, than we don't need to split based on bones
        for (unsigned int j = 0; j < faces.size(); ++j) {
            bulletMesh.addTriangle(GLMConverter::GLMToBlt(vertices[faces[j][0]]),
                                   GLMConverter::GLMToBlt(vertices[faces[j][1]]),
                                   GLMConverter::GLMToBlt(vertices[faces[j][2]]));
        }
        //shapeCopies.push_back(copyMesh);
    } else {
        //in this case, we don't use faces directly, instead we use per bone vertex information.
        std::map<uint32_t, std::vector<uint32_t>>::iterator it;
        for (it = boneAttachedMeshes.begin(); it != boneAttachedMeshes.end(); it++) {
            btConvexHullShape *hullshape = new btConvexHullShape();
            for (unsigned int index = 0; index < it->second.size(); index++) {
                hullshape->addPoint(GLMConverter::GLMToBlt(vertices[it->second[index]]));
            }
            bulletHull = new btShapeHull(hullshape);
            btScalar margin = hullshape->getMargin();
            bulletHull->buildHull(margin);
            delete hullshape;
            hullshape = nullptr;

            hullshape = new btConvexHullShape((const btScalar *) bulletHull->getVertexPointer(),
                                              bulletHull->numVertices());
            //FIXME clear memory leak here, no one deletes this shapes.
            bulletHullMap[it->first] = hullshape;
            bulletParentTransformMap[it->first].setFromOpenGLMatrix(glm::value_ptr(parentTransform));
        }

    }
}

bool MeshAsset::hasBones() const {
    return bones;
}

void MeshAsset::fillBoneMap(std::shared_ptr<const BoneNode> boneNode) {
    if (boneNode == nullptr) {
        return;
    }
    boneIdMap[boneNode->name] = boneNode->boneID;
    for (unsigned int i = 0; i < boneNode->children.size(); ++i) {
        fillBoneMap(boneNode->children[i]);
    }
}