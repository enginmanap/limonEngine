//
// Created by engin on 24.11.2020.
//

#ifndef LIMONENGINE_EMITTER_H
#define LIMONENGINE_EMITTER_H


#include <Graphics/Texture.h>
#include <API/Graphics/GraphicsProgram.h>
#include <random>
#include <utility>
#include <Renderable.h>
#include "../../Assets/TextureAsset.h"

class Emitter : public Renderable, public GameObject {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> speeds;
    std::vector<long> creationTime;

    long worldObjectID;
    std::string name;
    glm::vec2 size;
    std::shared_ptr<Texture> texture;
    long maxCount;
    long lifeTime;
    glm::vec3 startPosition;
    float startSphereR;

    std::shared_ptr<TextureAsset> textureAsset;//it is the root asset for texture
    long currentCount = 0;
    long lastSetupTime = 0;
    long lastCreationTime = 0;
    std::shared_ptr<Texture> positionTexture;
    float perMsParticleCount;

    std::random_device randomDevice;
    std::default_random_engine randomFloatGenerator;
    std::uniform_real_distribution<float> randomStartingPoints;
    std::uniform_real_distribution<float> randomSpeedDistribution;

    void setupVAO();

public:
    Emitter(long worldObjectId, std::string name, std::shared_ptr<AssetManager> assetManager,
            const std::string &textureFile, glm::vec3 startPosition, float startSphereR, glm::vec2 size, long count,
            long lifeTime);

    void setupForTime(long time) override {
        std::cout << "Emitter rendering " << __LINE__ << std::endl;
        if(lastSetupTime == 0) {
            lastSetupTime = time;//don't try to create massive amounts in first setup.
            lastCreationTime = time;
        }
        size_t removalStart, removalEnd = 0;
        bool removalSet = false;
        for (int i = 0; i < currentCount; ++i) {
            positions[i] = positions[i] + (speeds[i] * 1.0f);
            if(!removalSet && ((time - this->creationTime[i]) > lifeTime )) {
                removalStart = i;
                removalSet = true;
            }
            if(removalSet && ((time - this->creationTime[i]) > lifeTime )) {
                removalEnd = i+1;
            }
        }
        if(removalSet) {
            if(removalEnd == 0 || removalEnd > (size_t)currentCount) {
                removalEnd = currentCount;
            }
            //now remove
            positions.erase(positions.begin() + removalStart, positions.begin() + removalEnd);
            speeds.erase(speeds.begin() + removalStart, speeds.begin() + removalEnd);
            creationTime.erase(creationTime.begin() + removalStart, creationTime.begin() + removalEnd);

            currentCount = currentCount - (removalEnd - removalStart);
            //std::cout << "removing " << (removalEnd - removalStart) << "particles" << std::endl;
        }
        std::cout << "Emitter rendering " << __LINE__ << std::endl;
        if(currentCount < maxCount) {
            long creationParticleCount = (time - lastCreationTime) * perMsParticleCount;
            if(creationParticleCount > 0) {
                lastCreationTime = time;
            }
            for (int i = 0; i < creationParticleCount; ++i) {
                addRandomParticle(startPosition, startSphereR, time);
            }
            currentCount += creationParticleCount;
        }
        std::cout << "Emitter rendering " << __LINE__ << std::endl;
        std::vector<glm::vec3> temp;
        temp.insert(temp.end(), positions.begin(), positions.end());
        for (int i = temp.size(); i < maxCount; ++i) {
            temp.emplace_back(glm::vec3(0,0,0));
        }
        positionTexture->loadData(temp.data());
        std::cout << "Emitter rendering " << __LINE__ << std::endl;
        lastSetupTime = time;
    }

    void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram) override {
        renderProgram->setUniform("sprite", 6);
        graphicsWrapper->attachTexture((int) texture->getTextureID(), 6);
        renderProgram->setUniform("positions", 7);
        graphicsWrapper->attachTexture((int) positionTexture->getTextureID(), 7);
        renderProgram->setUniform("size", size.x);
        graphicsWrapper->renderInstanced(renderProgram->getID(), vao, ebo, 3 * 2, currentCount);
    }

    ObjectTypes getTypeID() const override {
        return PARTICLE_EMITTER;
    }

    std::string getName() const override {
        return name;
    }

    uint32_t getWorldObjectID() const override {
        return worldObjectID;
    }

    const glm::vec2 &getSize() const {
        return size;
    }

    const std::shared_ptr<Texture> &getTexture() const {
        return texture;
    }

    long getMaxCount() const {
        return maxCount;
    }

    long getLifeTime() const {
        return lifeTime;
    }

    const glm::vec3 &getStartPosition() const {
        return startPosition;
    }

    float getStartSphereR() const {
        return startSphereR;
    }

    void addRandomParticle(const glm::vec3 &startPosition, float startSphereR, long time);

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request [[gnu::unused]]) override;

};
#endif //LIMONENGINE_EMITTER_H
