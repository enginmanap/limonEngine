//
// Created by engin on 14.09.2016.
//

#include "MeshAsset.h"


MeshAsset::MeshAsset(AssetManager *assetManager, const aiMesh *currentMesh) : material(NULL) {
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

}