//
// Created by engin on 03/03/2026.
//

#ifndef LIMONENGINE_OCCLUSIONCULLINGMETA_H
#define LIMONENGINE_OCCLUSIONCULLINGMETA_H
#include<vector>

#include "GameObjects/Model.h"
#include "snapdragon-oc/Source/app/FuzzyCulling/API/SDOCAPI.h"
#include "OccludeeMetaData.h"


namespace SOC {
    class SOCPrivate;
}

class OcclusionCullerHelper {
    struct SDOCAABB {
        float minx;
        float miny;
        float minz;
        float maxx;
        float maxy;
        float maxz;
    };
    std::vector<OcculudeeMetaData> possibleVisibleSetMesh; // These 2 are split because that is how the sdoc wants it
    //std::vector<SDOCAABB> possibleVisibleSetMeshAABBs;     //
    std::vector<float> possibleVisibleSetMeshAABBFs;     //

    SOC::SOCPrivate* sdocInstance =nullptr;

    glm::vec3 playerPos;
    glm::vec3 viewDir;
    glm::mat4 finalDXMatrix;
public:
    OcclusionCullerHelper() {
        //sdocInstance = sdocInit(1024, 256, 0.0001f);
        possibleVisibleSetMeshAABBFs.reserve(6);
    }

    void extract_for_dx(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix) {
        glm::mat3 rotation = glm::mat3(cameraMatrix);
        glm::vec3 translation = glm::vec3(cameraMatrix[3]);
        playerPos = -glm::transpose(rotation) * translation;
        viewDir = -glm::vec3(cameraMatrix[2]);
        // --- Process Projection Matrix ---
        // Map OpenGL Z [-1, 1] to DirectX [0, 1]
        glm::mat4 clipCorrection = glm::mat4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.5f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
        glm::mat4 dxProj = clipCorrection * projectionMatrix;

        // --- Process View Matrix ---
        // If the library requires Left-Handed (standard for DX), flip the Z-axis:
        glm::mat4 dxView = cameraMatrix;
        dxView[0][2] = -dxView[0][2];
        dxView[1][2] = -dxView[1][2];
        dxView[2][2] = -dxView[2][2];
        dxView[3][2] = -dxView[3][2];
        // 1. Combine (Projection * View)
        glm::mat4 dxViewProj = dxProj * dxView;

        // 2. Transpose for DirectX Row-Major memory layout
        // This is what you pass to your library/shader
        finalDXMatrix = glm::transpose(dxViewProj);
    }

    void extract_for_ogl(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix) {
        glm::mat3 rotation = glm::mat3(cameraMatrix);
        glm::vec3 translation = glm::vec3(cameraMatrix[3]);
        playerPos = -glm::transpose(rotation) * translation;
        viewDir.x = -cameraMatrix[0][2];
        viewDir.y = -cameraMatrix[1][2];
        viewDir.z = -cameraMatrix[2][2];

        viewDir = glm::normalize(viewDir);

        finalDXMatrix = projectionMatrix * cameraMatrix;
    }

    void newFrame(const glm::vec3& cameraPosition[[gnu::unused]],const glm::vec3& viewDirection[[gnu::unused]],const glm::mat4& cameraMatrix, const glm::mat4& projectionMatrix) {
        if (!sdocInstance) {
            sdocInstance = static_cast<SOC::SOCPrivate *>(sdocInit(2560 / 1, 1440 / 1, 0.10f));
            // Enable occluder debugging
            unsigned int activeOcc = 1;
            sdocSet(sdocInstance, SDOC_DebugPrintActiveOccluder, activeOcc);
            //sdocSet(sdocInstance, SDOC_BeforeQueryTreatTrueAsCulled, 0);
            unsigned int debugPrintActiveOccluder = 0;
            sdocSync(sdocInstance, SDOC_SetPrintLogInGame, &debugPrintActiveOccluder); // Print to console
            sdocSync(sdocInstance, SDOC_RenderMode, SDOC_RenderMode_Full);
        }

        extract_for_ogl(cameraMatrix, projectionMatrix);

        //sdocStartNewFrame(sdocInstance, glm::value_ptr(cameraPosition), glm::value_ptr(viewDirection), glm::value_ptr(dxVP));
        sdocStartNewFrame(sdocInstance, glm::value_ptr(playerPos), glm::value_ptr(viewDir), glm::value_ptr(finalDXMatrix));
        //possibleVisibleSetMeshAABBs.clear();
        possibleVisibleSetMeshAABBFs.clear();
        possibleVisibleSetMesh.clear();
    }

    void endFrame() {
        if (sdocInstance) {
            sdocSet(sdocInstance, SDOC_FlushSubmittedOccluder, 1);
        }
    }

    void debugLibraryState() {
        if (!sdocInstance) return;

        // Get memory usage
        unsigned int memoryUsed = 0;
        sdocSync(sdocInstance, SDOC_Get_MemoryUsed, &memoryUsed);
        std::cout << "SDOC Memory Used: " << memoryUsed << " bytes" << std::endl;
        // Get logs
        char logBuffer[256];
        if (sdocSync(sdocInstance, SDOC_Get_Log, logBuffer)) {
            std::cout << "SDOC Log: " << logBuffer << std::endl;
        }
    }

    void renderOccluder(const Model* model) {
        for (size_t i = 0; i < model->getMeshMetaData().size(); ++i) {
            renderOccluder(model->getMeshMetaData()[i], model->getTransformation()->getWorldTransform());
        }
    }

    void renderOccluder(const Model::MeshMeta* meshMeta, const glm::mat4 &modelMatrix) {
        const uint32_t *triangleCounts = meshMeta->mesh->getTriangleCount();
        size_t vertCount = meshMeta->mesh->getVertices().size() * 3;  // Convert vec3 count to float count
        size_t idxCount = triangleCounts[0] * 3;//first LOD

        // std::cout << "DEBUG: Rendering occluder - vertices: " << vertCount
        //           << ", faces: " << faceCount
        //           << ", indices: " << idxCount << std::endl;
/*
        // Convert to flat arrays to ensure proper memory layout
        std::vector<float> flatVertices;
        flatVertices.reserve(vertCount);
        for (const auto& vert : meshMeta->mesh->getVertices()) {
            flatVertices.push_back(vert.x);
            flatVertices.push_back(vert.y);
            flatVertices.push_back(vert.z);
        }

        std::vector<unsigned short> flatIndices;
        flatIndices.reserve(idxCount);
        for (const auto& face : meshMeta->mesh->getFaces()) {
            flatIndices.push_back(face.x);
            flatIndices.push_back(face.y);
            flatIndices.push_back(face.z);
        }

        std::cout << "=== SUBMITTING OCCLUDER ===" << std::endl;
        std::cout << "Mesh: " << meshMeta->mesh->getName() << std::endl;
        std::cout << "Vertices: " << vertCount << ", Faces: " << faceCount << ", Indices: " << idxCount << std::endl;
*/
        sdocRenderOccluder(sdocInstance,
            &(meshMeta->mesh->getVertices().data()->x),
            &(meshMeta->mesh->getFaces().data()->x),
            vertCount,
            idxCount,
            glm::value_ptr(modelMatrix),
            false);
    }

    void addOccludee(const Model* model, uint32_t lod, float averageDepth, RenderList* renderList) {
        for (size_t i = 0; i < model->getMeshMetaData().size(); ++i) {
            addOccludee(model->getMeshMetaData()[i], model, lod, averageDepth, renderList);
        }
    }

    void addOccludee(const Model::MeshMeta* meshMeta, const Model* model, uint32_t lod, float averageDepth, RenderList* renderList) {
        // glm::vec4 minWorldAABBt = model->getTransformation()->getWorldTransform() * meshMeta->mesh->getAabbMin();
        // glm::vec4 maxWorldAABBt = model->getTransformation()->getWorldTransform() * meshMeta->mesh->getAabbMax();
        // ASSUMING THE MESH IS SAME AS THE MODEL, FOR THIS GIVEN INSTANCE
        glm::vec3 minWorldAABBt = model->getAabbMin();
        glm::vec3 maxWorldAABBt = model->getAabbMax();
        glm::vec3 minWorldAABB;
        glm::vec3 maxWorldAABB;
        minWorldAABB.x = std::min(minWorldAABBt.x, maxWorldAABBt.x);
        minWorldAABB.y = std::min(minWorldAABBt.y, maxWorldAABBt.y);
        minWorldAABB.z = std::min(minWorldAABBt.z, maxWorldAABBt.z);
        maxWorldAABB.x = std::max(minWorldAABBt.x, maxWorldAABBt.x);
        maxWorldAABB.y = std::max(minWorldAABBt.y, maxWorldAABBt.y);
        maxWorldAABB.z = std::max(minWorldAABBt.z, maxWorldAABBt.z);

        // SDOCAABB aabb;
        // aabb.minx = minWorldAABB.x;
        // aabb.miny = minWorldAABB.y;
        // aabb.minz = minWorldAABB.z;
        // aabb.maxx = maxWorldAABB.x;
        // aabb.maxy = maxWorldAABB.y;
        // aabb.maxz = maxWorldAABB.z;
        possibleVisibleSetMesh.push_back(OcculudeeMetaData(meshMeta, model, lod, averageDepth, renderList));
        //possibleVisibleSetMeshAABBs.push_back(aabb);
        possibleVisibleSetMeshAABBFs.push_back(minWorldAABB.x);
        possibleVisibleSetMeshAABBFs.push_back(minWorldAABB.y);
        possibleVisibleSetMeshAABBFs.push_back(minWorldAABB.z);
        possibleVisibleSetMeshAABBFs.push_back(maxWorldAABB.x);
        possibleVisibleSetMeshAABBFs.push_back(maxWorldAABB.y);
        possibleVisibleSetMeshAABBFs.push_back(maxWorldAABB.z);

    }

    std::vector<OcculudeeMetaData*> getNonOccludedMeshMeta() {
        std::vector<OcculudeeMetaData*> returnList;
        size_t meshCount = possibleVisibleSetMesh.size();
        returnList.reserve(meshCount);
        bool* results = new bool[meshCount];
        // Ensure AABB data size matches mesh count (6 floats per mesh)
        if (possibleVisibleSetMeshAABBFs.size() != meshCount * 6) {
            std::cout << "Inconsistent AABB data size: expected " << meshCount * 6 << " floats, got " << possibleVisibleSetMeshAABBFs.size() << " floats" << std::endl;
            // Data inconsistency - return empty list to prevent crash
            return returnList;
        }
        //sdocSet(sdocInstance, SDOC_Set_UsePrevDepthBuffer, 1);




        sdocQueryOccludees(sdocInstance, possibleVisibleSetMeshAABBFs.data(), meshCount, results);
        for (size_t i = 0; i < meshCount; ++i) {
            if (results[i]) {
                returnList.push_back(&possibleVisibleSetMesh[i]);
            } else {
                //std::cout << " model " << possibleVisibleSetMesh[i].model->getName() << ":" << possibleVisibleSetMesh[i].meshMeta->mesh->getName() << " is occluded" << std::endl;
            }
        }
        delete[] results;
        return returnList;
    }

    std::vector<OcculudeeMetaData*> getNonOccludedMeshMeta2() {
        std::vector<OcculudeeMetaData*> returnList;
        size_t meshCount = possibleVisibleSetMesh.size();
        returnList.reserve(meshCount);
        bool* results = new bool[meshCount];
        // Ensure AABB data size matches mesh count (6 floats per mesh)
        if (possibleVisibleSetMeshAABBFs.size() != meshCount * 6) {
            std::cout << "Inconsistent AABB data size: expected " << meshCount * 6 << " floats, got " << possibleVisibleSetMeshAABBFs.size() << " floats" << std::endl;
            // Data inconsistency - return empty list to prevent crash
            return returnList;
        }
        //sdocSet(sdocInstance, SDOC_Set_UsePrevDepthBuffer, 1);
        for (size_t i = 0; i < meshCount; ++i) {

            float* aabb = possibleVisibleSetMeshAABBFs.data() + i * 6;
            sdocQueryOccludees(sdocInstance, aabb, 1, results + i);
        }
        //sdocQueryOccludees(sdocInstance, possibleVisibleSetMeshAABBFs.data(), meshCount, results);
        for (size_t i = 0; i < meshCount; ++i) {
            if (results[i]) {
                returnList.push_back(&possibleVisibleSetMesh[i]);
            } else {
                //std::cout << " model " << possibleVisibleSetMesh[i].model->getName() << ":" << possibleVisibleSetMesh[i].meshMeta->mesh->getName() << " is occluded" << std::endl;
            }
        }
        //delete[] results;
        return returnList;
    }

    void dumpDepth() {
        if (!sdocInstance) return;

        // Set save path first
        const char* path = "SDOCdepthMap.ppm";
        sdocSync(sdocInstance, SDOC_Save_DepthMapPath, (void*)path);

        // Get buffer dimensions first
        unsigned int dimensions[2];
        sdocSync(sdocInstance, SDOC_Get_DepthBufferWidthHeight, dimensions);
        std::cout << "Depth buffer dimensions: " << dimensions[0] << "x" << dimensions[1] << std::endl;

        // Allocate buffer for depth data
        size_t bufferSize = dimensions[0] * dimensions[1] * 4; // 4 bytes per pixel
        unsigned char* depthData = new unsigned char[bufferSize];

        // Get depth map
        bool success = sdocSync(sdocInstance, SDOC_Get_DepthMap, depthData);
        std::cout << "Get depth map success: " << success << std::endl;

        // Save depth map
        success = sdocSync(sdocInstance, SDOC_Save_DepthMap, depthData);
        std::cout << "Save depth map success: " << success << std::endl;

        delete[] depthData;
    }

    bool queryOccluder(const Model::MeshMeta* meshMeta[[gnu::unused]], const glm::vec3& minAABB, const glm::vec3& maxAABB,const glm::mat4& modelMatrix) const {

        glm::vec4 minWorldAABB = modelMatrix * glm::vec4(minAABB, 1.0);
        glm::vec4 maxWorldAABB = modelMatrix * glm::vec4(maxAABB, 1.0);
        float aabb[6];
        aabb[0] = minWorldAABB.x;
        aabb[1] = minWorldAABB.y;
        aabb[2] = minWorldAABB.z;
        aabb[3] = maxWorldAABB.x;
        aabb[4] = maxWorldAABB.y;
        aabb[5] = maxWorldAABB.z;
        bool result[1];
        sdocQueryOccludees(sdocInstance, aabb, 1, result);
        return result[0];
        // return sdocQueryOccludeeMesh(sdocInstance,
        // reinterpret_cast<const float*>(meshMeta->mesh->getVertices().data()),
        // reinterpret_cast<const unsigned short*>(meshMeta->mesh->getFaces().data()),
        // meshMeta->mesh->getVertices().size()*3,
        // meshMeta->mesh->getFaces().size()*3,
        // glm::value_ptr(modelMatrix),
        // true,
        // aabb);
    }
};


#endif //LIMONENGINE_OCCLUSIONCULLINGMETA_H