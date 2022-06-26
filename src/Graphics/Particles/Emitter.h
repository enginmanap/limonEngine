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
public:
    struct TimedColorMultiplier {
        glm::uvec4 colorMultiplier = glm::uvec4(255,255,255,255);
        long time;
    };
    ~Emitter() {
        particleDataTexture.reset();
        assetManager->freeAsset(textureAsset->getName());
    }
private:
    std::shared_ptr<AssetManager> assetManager;
    std::vector<glm::vec4> positions;
    std::vector<glm::vec3> speeds;
    std::vector<long> creationTime;

    long worldObjectID;
    std::string name;
    glm::vec2 size;
    std::shared_ptr<Texture> texture;
    long maxCount;
    long lifeTime;
    glm::vec3 maxStartDistances;
    glm::vec3 gravity;
    glm::vec3 speedMultiplier;
    glm::vec3 speedOffset;
    std::vector<TimedColorMultiplier> timedColorMultipliers;//elements must be incremental ordered by time, first element must be time=0
    float perMsParticleCount;
    bool continuousEmit = true;//emit until reaching maximum, or emit as particles are removed;
    bool enabled = true;

    std::shared_ptr<TextureAsset> textureAsset;//it is the root asset for texture
    long currentCount = 0;
    long lastSetupTime = 0;
    long lastCreationTime = 0;
    std::shared_ptr<Texture> particleDataTexture;

    std::random_device randomDevice;
    std::default_random_engine randomFloatGenerator;
    std::uniform_real_distribution<float> randomStartingPoints;
    std::uniform_real_distribution<float> randomSpeedDistribution;
    long totalCreatedCount = 0;

    void setupVAO();

    void addRandomParticle(const glm::vec3 &startPosition, const glm::vec3 &maxStartDistances, long time);

    float calculateTimedColorShift(const long time, const long particleCreateTime);

    static bool getNameForTimedColorMultiplier(void *data, int index, const char **outText);

public:
    Emitter(long worldObjectId, std::string name, std::shared_ptr<AssetManager> assetManager,
            const std::string &textureFile, glm::vec3 startPosition, glm::vec3 maxStartDistances, glm::vec2 size, long count,
            long lifeTime, float particlePerMs = -1);

    void setupForTime(long time) override {
        if(lastSetupTime == 0) {
            lastSetupTime = time;//don't try to create massive amounts in first setup.
            lastCreationTime = time;
        }
        if(enabled && (
                    (continuousEmit && currentCount < maxCount) ||
                    (!continuousEmit && totalCreatedCount < maxCount)
                    )
            ) {
            long creationParticleCount = (time - lastCreationTime) * perMsParticleCount;
            if(creationParticleCount > 0) {
                lastCreationTime = time;
            }
            if(currentCount + creationParticleCount > maxCount) {
                creationParticleCount = maxCount - currentCount;
            }
            for (int i = 0; i < creationParticleCount; ++i) {
                addRandomParticle(this->transformation.getTranslate(), maxStartDistances, time);
            }
            currentCount += creationParticleCount;
            totalCreatedCount += creationParticleCount;
        }

        size_t removalStart, removalEnd = 0;
        bool removalSet = false;
        for (int i = 0; i < currentCount; ++i) {
            float colorShift = calculateTimedColorShift(time, creationTime[i]);
            positions[i].w = colorShift;
            positions[i] = positions[i] + glm::vec4(speeds[i], 0);
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
        std::vector<glm::vec4> temp;
        temp.insert(temp.end(), positions.rbegin(), positions.rend());
        for (int i = temp.size(); i < maxCount; ++i) {
            temp.emplace_back(glm::vec4(0,0,0, 1));
        }
        particleDataTexture->loadData(temp.data());
        lastSetupTime = time;
    }

    void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram, uint32_t lodLevel[[gnu::unused]]) override {
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

    const glm::vec3 &getMaximumStartDistances() const {
        return maxStartDistances;
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

    const std::vector<TimedColorMultiplier> &getTimedColorMultipliers() const {
        return timedColorMultipliers;
    }

    void setTimedColorMultipliers(const std::vector<TimedColorMultiplier> &timedColorMultipliers) {
        Emitter::timedColorMultipliers = timedColorMultipliers;
    }

    bool isContinuousEmit() const {
        return continuousEmit;
    }

    void setContinuousEmit(bool continuousEmit) {
        Emitter::continuousEmit = continuousEmit;
    }

    bool isEnabled() const {
        return enabled;
    }

    void setEnabled(bool enabled) {
        if(!this->enabled) {
            lastSetupTime = 0;
        }
        this->enabled = enabled;
    }

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request [[gnu::unused]]) override;

    static float packToFloat(glm::uvec4 vec) {
        uint8_t values[4];
        values[0] = vec.x;
        values[1] = vec.y;
        values[2] = vec.z;
        values[3] = vec.w;

        float result;
        std::copy(reinterpret_cast<const char*>(&values[0]),
                  reinterpret_cast<const char*>(&values[4]),
                  reinterpret_cast<char*>(&result));
        return result;
    }

    static glm::uvec4 unpackFloat(float value) {
        uint8_t *values = (uint8_t*) & value;

        glm::uvec4 result;
        result.x = values[0];
        result.y = values[1];
        result.z = values[2];
        result.w = values[3];
        return result;
    }

};
#endif //LIMONENGINE_EMITTER_H
