//
// Created by engin on 24.11.2020.
//

#ifndef LIMONENGINE_GPUPARTICLEEMITTER_H
#define LIMONENGINE_GPUPARTICLEEMITTER_H


#include <Graphics/Texture.h>
#include <API/Graphics/GraphicsProgram.h>
#include <random>
#include <utility>
#include <Renderable.h>
#include "../../Assets/TextureAsset.h"

// This is a partial implementation I did just to check the performance difference, it is not ready to use
class GPUParticleEmitter : public Renderable, public GameObject {
public:
    struct TimedColorMultiplier {
        glm::uvec4 colorMultiplier = glm::uvec4(255,255,255,255);
        long time;
    };
private:
    struct ParticleData {
        glm::vec3 position;
        long creationTime;
        glm::vec3 speed;
        long destroyTime;
        ParticleData() = default;
    };
    std::shared_ptr<AssetManager> assetManager;
    std::vector<ParticleData> particles;

    long worldObjectID;
    std::string name;
    glm::vec2 size;
    std::shared_ptr<Texture> texture;
    long maxCount;
    long lifeTime;
    glm::vec3 maxStartDistances = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 speedMultiplier = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 speedOffset = glm::vec3(0.0f, 0.0f, 0.0f);
    std::vector<TimedColorMultiplier> timedColorMultipliers;//elements must be incremental ordered by time, first element must be time=0
    float perMsParticleCount;
    bool continuousEmit = true;//emit until reaching maximum, or emit as particles are removed;
    bool enabled = true;
    bool dirty;

    std::shared_ptr<TextureAsset> textureAsset;//it is the root asset for texture
    std::shared_ptr<Texture> particleDataTexture;

    std::random_device randomDevice;
    std::default_random_engine randomFloatGenerator;
    std::uniform_real_distribution<float> randomStartingPoints;
    std::uniform_real_distribution<float> randomSpeedDistribution;
    long totalCreatedCount = 0;

    void setupVAO();

    ParticleData addRandomParticle(const glm::vec3 &startPosition, const glm::vec3 &maxStartDistances, long creationTime);

    float calculateTimedColorShift(const long time, const long particleCreateTime);

    static bool getNameForTimedColorMultiplier(void *data, int index, const char **outText);

public:
    GPUParticleEmitter(long worldObjectId, std::string name, std::shared_ptr<AssetManager> assetManager,
            const std::string &textureFile, glm::vec3 startPosition, glm::vec3 maxStartDistances, glm::vec2 size, long count,
            long lifeTime, long startTime, float particlePerMs = -1);

    ~GPUParticleEmitter() override;

    void setupForTime(long time) override {
        if(dirty) {
            setupParticles(time);
            dirty = false;
        }
    }

    void setupParticles(long time) {
        std::vector<glm::vec4> temp;
        float particleBeforeTimeIncrease = perMsParticleCount;
        float timeLeftBeforeNextParticle = 0;
        long startingTime = time;
        for(long currentCount = 0; currentCount < maxCount; ++ currentCount) {
            //there are 2 ways to reach max count, if we create 1 or more particles per ms, or not
            if(perMsParticleCount > 1.0) {
                ParticleData particleData = addRandomParticle(this->transformation.getTranslate(), maxStartDistances,
                                                              startingTime);
                particleBeforeTimeIncrease--;
                if (particleBeforeTimeIncrease <= 0) {
                    //rest of the particles should start after now.
                    startingTime++;

                    particleBeforeTimeIncrease += perMsParticleCount;
                }
                particles.emplace_back(particleData);
            } else {
                //we don't want to create 1 particle per second, so we will go with time
                ParticleData particleData = addRandomParticle(this->transformation.getTranslate(), maxStartDistances,
                                                              startingTime);
                particles.emplace_back(particleData);
                float timeShift = 1.0f/ perMsParticleCount;
                timeShift = timeShift + timeLeftBeforeNextParticle;
                startingTime += floor(timeShift);
                timeLeftBeforeNextParticle = timeShift - floor(timeShift);

            }
        }
        for(long currentCount = 0; currentCount < maxCount; ++ currentCount) {
            temp.emplace_back(glm::vec4(particles[currentCount].position, particles[currentCount].creationTime));
        }
        for(long currentCount = 0; currentCount < maxCount; ++ currentCount) {
            temp.emplace_back(glm::vec4(particles[currentCount].speed, particles[currentCount].destroyTime));
        }
        particleDataTexture->loadData(temp.data());

    }

    void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram, uint32_t lodLevel[[gnu::unused]]) override {
        renderProgram->setUniform("sprite", 6);
        graphicsWrapper->attachTexture((int) texture->getTextureID(), 6);
        renderProgram->setUniform("positions", 7);
        graphicsWrapper->attachTexture((int) particleDataTexture->getTextureID(), 7);
        renderProgram->setUniform("size", size.x);
        renderProgram->setUniform("gravity", gravity);
        renderProgram->setUniform("continuousEmit", continuousEmit);
        renderProgram->setUniform("lifeTime", (float) lifeTime);
        graphicsWrapper->renderInstanced(renderProgram->getID(), vao, ebo, 3 * 2, maxCount);
    }

    ObjectTypes getTypeID() const override {
        return ObjectTypes::GPU_PARTICLE_EMITTER;
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
        if(gravity != this->gravity) {
            dirty = true;
        }
        GPUParticleEmitter::gravity = gravity;
    }

    const glm::vec3 &getSpeedMultiplier() const {
        return speedMultiplier;
    }

    void setSpeedMultiplier(const glm::vec3 &speedMultiplier) {
        if(speedMultiplier != this->speedMultiplier) {
            dirty = true;
        }
        GPUParticleEmitter::speedMultiplier = speedMultiplier;
    }

    const glm::vec3 &getSpeedOffset() const {
        return speedOffset;
    }

    void setSpeedOffset(const glm::vec3 &speedOffset) {
        if(speedOffset != this->speedOffset) {
            dirty = true;
        }
        GPUParticleEmitter::speedOffset = speedOffset;
    }

    const std::vector<TimedColorMultiplier> &getTimedColorMultipliers() const {
        return timedColorMultipliers;
    }

    void setTimedColorMultipliers(const std::vector<TimedColorMultiplier> &timedColorMultipliers) {
        GPUParticleEmitter::timedColorMultipliers = timedColorMultipliers;
    }

    bool isContinuousEmit() const {
        return continuousEmit;
    }

    void setContinuousEmit(bool continuousEmit) {
        if(continuousEmit != this->continuousEmit) {
            dirty = true;
        }
        GPUParticleEmitter::continuousEmit = continuousEmit;
    }

    bool isEnabled() const {
        return enabled;
    }

    void setEnabled(bool enabled) {
        if(this->enabled != enabled) {
            dirty = true;
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
#endif //LIMONENGINE_GPUPARTICLEEMITTER_H
