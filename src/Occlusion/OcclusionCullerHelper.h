//
// Created by engin on 03/03/2026.
//

#ifndef LIMONENGINE_OCCLUSIONCULLINGMETA_H
#define LIMONENGINE_OCCLUSIONCULLINGMETA_H
#include<string>
#include<vector>

#include "GameObjects/Model.h"
#include "snapdragon-oc/Source/app/FuzzyCulling/API/SDOCAPI.h"
#include "OccludeeMetaData.h"
#include "Utils/AABBConverter.hpp"
#include "Profiler/ProfilerMacros.h"


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
    std::vector<OcculudeeMetaData> possibleVisibleSet; // These 2 are split because that is how the sdoc wants it
    std::vector<float> possibleVisibleSetAABBs;        //

    SOC::SOCPrivate* sdocInstance =nullptr;

    const OptionsUtil::Options::Option<long> widthOption;
    const OptionsUtil::Options::Option<long> heightOption;

    glm::vec3 playerPos;
    glm::vec3 viewDir;
    glm::mat4 cameraProjectionMatrix;
public:
    int dumpFrameCount = 0;

    //the defaults newFrame creates the buffer with
    int32_t getBufferWidth() const {
        return static_cast<int32_t>(widthOption.getOrDefault(512));
    }

    int32_t getBufferHeight() const {
        return static_cast<int32_t>(heightOption.getOrDefault(256));
    }
private:
public:
    OcclusionCullerHelper(const OptionsUtil::Options::Option<long>& widthOption,
        const OptionsUtil::Options::Option<long>& heightOption) :
    widthOption(widthOption), heightOption(heightOption) {
        possibleVisibleSetAABBs.reserve(6); // if no entry, .data() call returns null.
    }

    void extractForDX(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix) {
        glm::mat3 rotation = glm::mat3(cameraMatrix);
        glm::vec3 translation = glm::vec3(cameraMatrix[3]);
        playerPos = -glm::transpose(rotation) * translation;
        viewDir = -glm::vec3(cameraMatrix[2]);
        // Map OpenGL Z [-1, 1] to DX [0, 1]
        glm::mat4 clipCorrection = glm::mat4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.5f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
        glm::mat4 dxProj = clipCorrection * projectionMatrix;

        // DX uses left handed, flip the Z-axis:
        glm::mat4 dxView = cameraMatrix;
        dxView[0][2] = -dxView[0][2];
        dxView[1][2] = -dxView[1][2];
        dxView[2][2] = -dxView[2][2];
        dxView[3][2] = -dxView[3][2];

        glm::mat4 dxViewProj = dxProj * dxView;
        // 2. Transpose for DX, it is row major
        cameraProjectionMatrix = glm::transpose(dxViewProj);
    }

    void extractForOpenGL(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix) {
        glm::mat3 rotation = glm::mat3(cameraMatrix);
        glm::vec3 translation = glm::vec3(cameraMatrix[3]);
        playerPos = -glm::transpose(rotation) * translation;
        viewDir.x = -cameraMatrix[0][2];
        viewDir.y = -cameraMatrix[1][2];
        viewDir.z = -cameraMatrix[2][2];

        viewDir = glm::normalize(viewDir);

        cameraProjectionMatrix = projectionMatrix * cameraMatrix;
    }

    void newFrame(const glm::vec3& cameraPosition[[gnu::unused]],const glm::vec3& viewDirection[[gnu::unused]],const glm::mat4& cameraMatrix, const glm::mat4& projectionMatrix) {
        PROFILE_VISIBILITY("Culling::NewFrame");
        if (!sdocInstance) {
            sdocInstance = static_cast<SOC::SOCPrivate *>(sdocInit(widthOption.getOrDefault(512), heightOption.getOrDefault(256), 1.0f));//sdoc clamps near plane to 1.0f anyway.
            // Enable occluder debugging
            //unsigned int activeOcc = 1;
            //sdocSet(sdocInstance, SDOC_DebugPrintActiveOccluder, activeOcc);
            //sdocSet(sdocInstance, SDOC_BeforeQueryTreatTrueAsCulled, 0);
            //unsigned int debugPrintActiveOccluder = 0;
            //sdocSync(sdocInstance, SDOC_SetPrintLogInGame, &debugPrintActiveOccluder); // Print to console
            sdocSet(sdocInstance, SDOC_RenderMode, SDOC_RenderMode_Full);
        }

        extractForOpenGL(cameraMatrix, projectionMatrix);

        sdocStartNewFrame(sdocInstance, glm::value_ptr(playerPos), glm::value_ptr(viewDir), glm::value_ptr(cameraProjectionMatrix));
        possibleVisibleSetAABBs.clear();
        possibleVisibleSet.clear();
    }

    void endFrame() {
        if (sdocInstance) {
            sdocSet(sdocInstance, SDOC_FlushSubmittedOccluder, 1);
        }
    }

    void debugLibraryState() {
        if (!sdocInstance) {
            return;
        }

        unsigned int memoryUsed = 0;
        sdocSync(sdocInstance, SDOC_Get_MemoryUsed, &memoryUsed);
        std::cout << "SDOC Memory Used: " << memoryUsed << " bytes" << std::endl;
        char logBuffer[256];
        if (sdocSync(sdocInstance, SDOC_Get_Log, logBuffer)) {
            std::cout << "SDOC Log: " << logBuffer << std::endl;
        }
    }

    void renderOccluder(const Model* model, uint32_t requestedLodLevel) {
        for (size_t i = 0; i < model->getMeshMetaData().size(); ++i) {
            renderOccluder(model->getMeshMetaData()[i], model->getTransformation()->getWorldTransform(), requestedLodLevel);
        }
    }

    void renderOccluder(const Model::MeshMeta* meshMeta, const glm::mat4 &modelMatrix, uint32_t requestedLodLevel) {
        const std::vector<uint16_t, AlignedAllocator<uint16_t, 64>> &bakedOccluder = meshMeta->mesh->getBakedOccluder(requestedLodLevel);
        if (!bakedOccluder.empty()) {
            //SDOC rasterizes at flush, so the bake and the matrix have to outlive this call. The matrix is the model's own
            sdocRenderBakedOccluder(sdocInstance, const_cast<unsigned short *>(bakedOccluder.data()), glm::value_ptr(modelMatrix));
            return;
        }
        uint32_t lodLevel = meshMeta->mesh->getSimplestLodLevel(requestedLodLevel);
        const uint32_t *triangleCounts = meshMeta->mesh->getTriangleCount();
        const uint32_t *offsets = meshMeta->mesh->getOffsets();
        size_t vertCount = meshMeta->mesh->getVertices().size();//lod indices still address the full vertex buffer
        size_t idxCount = triangleCounts[lodLevel] * 3;

        sdocRenderOccluder(sdocInstance,
            &(meshMeta->mesh->getVertices().data()->x),
            //offsets are in index units, so cast before adding
            (const unsigned short *)&(meshMeta->mesh->getFaces().data()->x) + offsets[lodLevel],
            vertCount,
            idxCount,
            glm::value_ptr(modelMatrix),
            true);
    }

    void addOccludee(const Model* model, const glm::vec3& minWorldAABB, const glm::vec3& maxWorldAABB, uint32_t lod, float averageDepth, RenderList* renderList) {
        possibleVisibleSet.push_back(OcculudeeMetaData(model, lod, averageDepth, renderList));
        possibleVisibleSetAABBs.push_back(minWorldAABB.x);
        possibleVisibleSetAABBs.push_back(minWorldAABB.y);
        possibleVisibleSetAABBs.push_back(minWorldAABB.z);
        possibleVisibleSetAABBs.push_back(maxWorldAABB.x);
        possibleVisibleSetAABBs.push_back(maxWorldAABB.y);
        possibleVisibleSetAABBs.push_back(maxWorldAABB.z);

    }

    std::vector<OcculudeeMetaData*> getNonOccluded() {
        PROFILE_VISIBILITY("Culling::QueryOccludees");//SDOC rasterizes the occluders here, at flush, not when they are submitted
        std::vector<OcculudeeMetaData*> returnList;
        size_t occludeeCount = possibleVisibleSet.size();
        returnList.reserve(occludeeCount);
        bool* results = new bool[occludeeCount];
        // Ensure AABB data size matches occludee count (6 floats per occludee)
        if (possibleVisibleSetAABBs.size() != occludeeCount * 6) {
            std::cerr << "Inconsistent AABB data size: expected " << occludeeCount * 6 << " floats, got " << possibleVisibleSetAABBs.size() << " floats, can't calculate" << std::endl;
            delete[] results;
            return returnList;
        }
        sdocQueryOccludees(sdocInstance, possibleVisibleSetAABBs.data(), occludeeCount, results);
        for (size_t i = 0; i < occludeeCount; ++i) {
            if (results[i]) {
                returnList.push_back(&possibleVisibleSet[i]);
            }
        }
        delete[] results;
        return returnList;
    }

    //same query as getNonOccluded, for occlusion_debugReverse which needs the removed ones too
    void splitOccludees(std::vector<OcculudeeMetaData*>& nonOccluded, std::vector<OcculudeeMetaData*>& occluded) {
        PROFILE_VISIBILITY("Culling::QueryOccludeesDebugReverse");
        size_t occludeeCount = possibleVisibleSet.size();
        nonOccluded.reserve(occludeeCount);
        if (possibleVisibleSetAABBs.size() != occludeeCount * 6) {
            std::cerr << "Inconsistent AABB data size: expected " << occludeeCount * 6 << " floats, got " << possibleVisibleSetAABBs.size() << " floats, can't calculate" << std::endl;
            return;
        }
        bool* results = new bool[occludeeCount];
        sdocQueryOccludees(sdocInstance, possibleVisibleSetAABBs.data(), occludeeCount, results);
        for (size_t i = 0; i < occludeeCount; ++i) {
            if (results[i]) {
                nonOccluded.push_back(&possibleVisibleSet[i]);
            } else {
                occluded.push_back(&possibleVisibleSet[i]);
            }
        }
        delete[] results;
    }

    void dumpDepth(const std::string& fileName) {
        if (!sdocInstance) return;

        std::string pathStr = fileName + ".ppm";
        const char* path = pathStr.c_str();
        sdocSync(sdocInstance, SDOC_Save_DepthMapPath, (void*)path);

        unsigned int dimensions[2];
        sdocSync(sdocInstance, SDOC_Get_DepthBufferWidthHeight, dimensions);
        std::cout << "Depth buffer dimensions: " << dimensions[0] << "x" << dimensions[1] << std::endl;

        size_t bufferSize = dimensions[0] * dimensions[1] * 4; // 4 bytes per pixel
        unsigned char* depthData = new unsigned char[bufferSize];

        bool success = sdocSync(sdocInstance, SDOC_Get_DepthMap, depthData);
        std::cout << "Get depth map success: " << success << std::endl;

        success = sdocSync(sdocInstance, SDOC_Save_DepthMap, depthData);
        std::cout << "Save depth map success: " << success << std::endl;
        delete[] depthData;
    }

    bool querySingleOccludee(const glm::vec3& minAABB, const glm::vec3& maxAABB,const glm::mat4& modelMatrix) const {
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
    }
};


#endif //LIMONENGINE_OCCLUSIONCULLINGMETA_H
