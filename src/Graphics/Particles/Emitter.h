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
    std::vector<glm::vec4> positions;
    std::vector<glm::vec3> speeds;
    std::vector<long> creationTime;

    long worldObjectID;
    std::string name;
    glm::vec2 size;
    std::shared_ptr<Texture> texture;
    long maxCount;
    long lifeTime;
    float startSphereR;
    glm::vec3 gravity;
    glm::vec3 speedMultiplier;
    glm::vec3 speedOffset;

    std::shared_ptr<TextureAsset> textureAsset;//it is the root asset for texture
    long currentCount = 0;
    long lastSetupTime = 0;
    long lastCreationTime = 0;
    std::shared_ptr<Texture> particleDataTexture;
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
        if(lastSetupTime == 0) {
            lastSetupTime = time;//don't try to create massive amounts in first setup.
            lastCreationTime = time;
        }
        float alphaChange = (float)(time - lastSetupTime) / (float)this->lifeTime;
        if(alphaChange > 1 ) {
            alphaChange = 1;
        }
        size_t removalStart, removalEnd = 0;
        bool removalSet = false;
        for (int i = 0; i < currentCount; ++i) {
            positions[i] = positions[i] + glm::vec4(speeds[i], -1 * alphaChange);
            speeds[i] +=(gravity/60.0);
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
        }
        if(currentCount < maxCount) {
            long creationParticleCount = (time - lastCreationTime) * perMsParticleCount;
            if(creationParticleCount > 0) {
                lastCreationTime = time;
            }
            for (int i = 0; i < creationParticleCount; ++i) {
                addRandomParticle(this->transformation.getTranslate(), startSphereR, time);
            }
            currentCount += creationParticleCount;
        }
        std::vector<glm::vec4> temp;
        temp.insert(temp.end(), positions.rbegin(), positions.rend());
        for (int i = temp.size(); i < maxCount; ++i) {
            temp.emplace_back(glm::vec4(0,0,0, 0));
        }
        particleDataTexture->loadData(temp.data());
        lastSetupTime = time;
    }

    void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram) override {
        renderProgram->setUniform("sprite", 6);
        graphicsWrapper->attachTexture((int) texture->getTextureID(), 6);
        renderProgram->setUniform("positions", 7);
        graphicsWrapper->attachTexture((int) particleDataTexture->getTextureID(), 7);
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

    float getStartSphereR() const {
        return startSphereR;
    }

    const glm::vec3 &getGravity() const {
        return gravity;
    }

    void setGravity(const glm::vec3 &gravity) {
        Emitter::gravity = gravity;
    }

    const glm::vec3 &getSpeedMultiplier() const {
        return speedMultiplier;
    }

    void setSpeedMultiplier(const glm::vec3 &speedMultiplier) {
        Emitter::speedMultiplier = speedMultiplier;
    }

    const glm::vec3 &getSpeedOffset() const {
        return speedOffset;
    }

    void setSpeedOffset(const glm::vec3 &speedOffset) {
        Emitter::speedOffset = speedOffset;
    }

    void addRandomParticle(const glm::vec3 &startPosition, float startSphereR, long time);

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request [[gnu::unused]]) override;

};
#endif //LIMONENGINE_EMITTER_H
