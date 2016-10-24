//
// Created by engin on 14.09.2016.
//

#include "MeshAsset.h"


MeshAsset::MeshAsset(AssetManager *assetManager, const aiMesh *currentMesh, const Material *material,
                     const BoneNode *meshSkeleton) : material(
        material) {
    triangleCount = currentMesh->mNumFaces;
    bulletMesh = new btTriangleMesh();
    if (!currentMesh->HasPositions()) {
        throw "No position found"; //Not going to process if mesh is empty
        return;
    }

    vertexCount = currentMesh->mNumVertices;
    if (currentMesh->HasTextureCoords(0)) {
        for (int j = 0; j < currentMesh->mNumVertices; ++j) {
            vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
            normals.push_back(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]));
            textureCoordinates.push_back(
                    glm::vec2(currentMesh->mTextureCoords[0][j].x, currentMesh->mTextureCoords[0][j].y));

        }
    } else {
        for (int j = 0; j < currentMesh->mNumVertices; ++j) {
            vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
            normals.push_back(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]));
        }
    }

    for (int j = 0; j < currentMesh->mNumFaces; ++j) {
        faces.push_back(glm::vec3(currentMesh->mFaces[j].mIndices[0],
                                  currentMesh->mFaces[j].mIndices[1],
                                  currentMesh->mFaces[j].mIndices[2]));
        bulletMesh->addTriangle(GLMConverter::GLMToBlt(vertices[currentMesh->mFaces[j].mIndices[0]]),
                                GLMConverter::GLMToBlt(vertices[currentMesh->mFaces[j].mIndices[1]]),
                                GLMConverter::GLMToBlt(vertices[currentMesh->mFaces[j].mIndices[2]]));
    }
    if (vertexCount <= 24) {
        for (int j = 0; j < currentMesh->mNumFaces; ++j) {
            bulletMeshFaces.push_back(glm::mediump_uvec3(currentMesh->mFaces[j].mIndices[0],
                                                         currentMesh->mFaces[j].mIndices[1],
                                                         currentMesh->mFaces[j].mIndices[2]));
        }
    }

    convexShape = new btConvexTriangleMeshShape(bulletMesh);
    if (vertexCount > 24) { // hull shape has 24 vertices, if we already has less, don't generate hull.
        //hull approximation

        hull = new btShapeHull(convexShape);
        btScalar margin = convexShape->getMargin();
        hull->buildHull(margin);
        delete convexShape;

        simplifiedConvexShape = new btConvexHullShape((const btScalar *) hull->getVertexPointer(),
                                                      hull->numVertices());

        bulletMeshFaces.clear();
        for (int k = 0; k < hull->numIndices(); k = k + 3) {
            bulletMeshFaces.push_back(glm::mediump_uvec3(hull->getIndexPointer()[k],
                                                         hull->getIndexPointer()[k + 1],
                                                         hull->getIndexPointer()[k + 2]));
        }
        bulletTriangleCount = hull->numTriangles();
        vertexCount = hull->numVertices();
    } else {
        hull = NULL;
        //direct use of the object
        simplifiedConvexShape = convexShape;
        bulletTriangleCount = triangleCount;
    }
    convexShape = NULL; //since simplified Convex shape has taken over, we don't need the pointer anymore

    uint_fast32_t vbo;
    assetManager->getGlHelper()->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    assetManager->getGlHelper()->bufferNormalData(normals, vao, vbo, 4);
    bufferObjects.push_back(vbo);

    if (!textureCoordinates.empty()) {
        assetManager->getGlHelper()->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 3);
        bufferObjects.push_back(vbo);
    }

    //loadBoneInformation
    if (currentMesh->HasBones()) {
        this->bones = true;

        assert(meshSkeleton != NULL);
        this->skeleton = meshSkeleton;
        //we will create a bone* vector so we can load the weight information to GPU. The same vector should be
        //copied and filled with animation information on Model class
        //an a name->bone* map, for animation.
        fillBoneMap(this->skeleton);

        boneIDs.resize(vertices.size());
        boneWeights.resize(vertices.size());
        for (unsigned int j = 0; j < currentMesh->mNumBones; ++j) {
            uint_fast32_t boneID = boneIdMap[currentMesh->mBones[j]->mName.C_Str()];
            /*
             * Assimp has a bone array with weight lists for vertices,
             * we need a vertex array with weight list for bones.
             * This loop should generate that
             */
            for (uint_fast32_t k = 0; k < currentMesh->mBones[j]->mNumWeights; ++k) {
                addWeightToVertex(boneID, currentMesh->mBones[j]->mWeights[k].mVertexId,
                                  currentMesh->mBones[j]->mWeights[k].mWeight);
            }


        }
        std::cout << "Animation added for mesh" << std::endl;

        assetManager->getGlHelper()->bufferExtraVertexData(boneIDs, vao, vbo, 5);
        bufferObjects.push_back(vbo);

        assetManager->getGlHelper()->bufferExtraVertexData(boneWeights, vao, vbo, 6);
        bufferObjects.push_back(vbo);
    } else {
        this->bones = false;
    }
}

bool MeshAsset::addWeightToVertex(uint_fast32_t boneID, unsigned int vertex, float weight) {
    glm::lowp_uvec4 *vertexBoneIDs = &boneIDs[vertex];//this allows us to change bone ids
    glm::vec4 *vertexWeights = &boneWeights[vertex];
    //the weights are suppose to be ordered,
    for (int i = 0; i < 4; ++i) { //we allow only 4 bones per vertex
        if ((*vertexWeights)[i] < weight) {
            //shift to open slot
            for (int j = 3; j > i; --j) {
                (*vertexWeights)[j] = (*vertexWeights)[j - 1];
                (*vertexBoneIDs)[j] = (*vertexBoneIDs)[j - 1];
            }
            (*vertexWeights)[i] = weight;
            (*vertexBoneIDs)[i] = boneID;
            return true;
        }
    }
    return false;
}

btConvexShape *MeshAsset::getCollisionShape() {
    {
        btTriangleMesh *copyMesh = new btTriangleMesh();
        if (hull == NULL) {
            for (int i = 0; i < bulletTriangleCount; ++i) {
                copyMesh->addTriangle(GLMConverter::GLMToBlt(vertices.at(bulletMeshFaces[i].x)),
                                      GLMConverter::GLMToBlt(vertices.at(bulletMeshFaces[i].y)),
                                      GLMConverter::GLMToBlt(vertices.at(bulletMeshFaces[i].z)));
            }
        } else {
            for (int i = 0; i < bulletTriangleCount; ++i) {
                copyMesh->addTriangle(hull->getVertexPointer()[bulletMeshFaces[i].x],
                                      hull->getVertexPointer()[bulletMeshFaces[i].y],
                                      hull->getVertexPointer()[bulletMeshFaces[i].z]);
            }
        }
        btConvexShape *copyShape = new btConvexTriangleMeshShape(copyMesh);
        shapeCopies.push_back(copyShape);
        return copyShape;
    }
}

bool MeshAsset::hasBones() const {
    return bones;
}

void MeshAsset::fillBoneMap(const BoneNode *boneNode) {
    if (boneNode == NULL) {
        return;
    }
    boneIdMap[boneNode->name] = boneNode->boneID;
    for (int i = 0; i < boneNode->children.size(); ++i) {
        fillBoneMap(boneNode->children[i]);
    }
}


