//
// Created by engin on 24.11.2020.
//

#include <Assets/TextureAsset.h>

#include <utility>
#include <vector>
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include "ImGui/imgui.h"
#include "Emitter.h"
#include "../../XMLHelper.h"
#include "../../ImGuiHelper.h"
#include "../../limonAPI/util/Logger.h"
#include "../../Utils/GLMConverter.h"
#include <LinearMath/btConvexHullComputer.h>

#include <glm/gtc/type_ptr.hpp>

Emitter::Emitter(long worldObjectId, std::string name, std::shared_ptr<AssetManager> assetManager,
                 const std::string &textureFile, glm::vec3 startPosition, glm::vec3 maxStartDistances, glm::vec2 size, long count,
                 long lifeTime, float particlePerMs) :
        Renderable(assetManager->getGraphicsWrapper()),
        assetManager(assetManager),
        worldObjectID(worldObjectId),
        name(std::move(name)),
        size(size),
        maxCount(count),
        lifeTime(lifeTime),
        maxStartDistances(maxStartDistances),
        randomFloatGenerator(randomDevice()),
        randomStartingPoints(-1.0f, 1.0f),
        randomSpeedDistribution(-1.0f, 1.0f)
        {
    this->transformation.setTranslate(startPosition);
    textureAsset = assetManager->loadAsset<TextureAsset>({textureFile});
    this->texture = textureAsset->getTexture();

    setupVAO();

    particleDataTexture = std::make_shared<Texture>(this->graphicsWrapper,
                                                    GraphicsInterface::TextureTypes::T2D,
                                                    GraphicsInterface::InternalFormatTypes::RGBA32F,
                                                    GraphicsInterface::FormatTypes::RGBA,
                                                    GraphicsInterface::DataTypes::FLOAT,
                                                    maxCount, 1);
    if(particlePerMs > 0) {
        this->perMsParticleCount = particlePerMs;
    } else {
        this->perMsParticleCount = (float) maxCount / lifeTime;
    }
}

Emitter::Emitter(const Emitter& other, uint32_t newObjectID) :
        Emitter(newObjectID, other.name, other.assetManager, other.textureAsset->getName().front(),
                other.transformation.getTranslate(), other.maxStartDistances, other.size, other.maxCount,
                other.lifeTime, other.perMsParticleCount) {
    this->gravity = other.gravity;
    this->speedMultiplier = other.speedMultiplier;
    this->speedOffset = other.speedOffset;
    this->timedColorMultipliers = other.timedColorMultipliers;
    this->continuousEmit = other.continuousEmit;
    this->enabled = other.enabled;
    this->transformation.setTransformationsNotPropagate(
            other.transformation.getTranslate(), other.transformation.getOrientation(), other.transformation.getScale());
}

Attachable* Emitter::clone(uint32_t newObjectID, LimonAPI* limonAPI [[gnu::unused]],
                           const std::unordered_map<uint32_t, uint32_t>& idRemap [[gnu::unused]]) const {
    return new Emitter(*this, newObjectID);
}

void Emitter::addRandomParticle(const glm::vec3 &startPosition, const glm::vec3 &maxStartDistances, long time) {
    float x = randomStartingPoints(randomFloatGenerator) * maxStartDistances.x;
    float y = randomStartingPoints(randomFloatGenerator) * maxStartDistances.y;
    float z = randomStartingPoints(randomFloatGenerator) * maxStartDistances.z;
    /* Leaving here if I want to add a toggle for sphere
    if(x*x + y*y + z*z > startSphereR * startSphereR) {
        if(x > y && x > z) {
            x = 0;
        } else if(y > x && y > z) {
            y = 0;
        } else {
            z = 0;
        }
    }
    */
    glm::vec4 position = glm::vec4(startPosition, 0) +
                         glm::vec4(x, y, z, 1);
    positions.emplace_back(position);
    //meters/second in, divided down here since speeds[] itself is meters/tick
    glm::vec3 speed = glm::vec3(randomSpeedDistribution(randomFloatGenerator) * speedMultiplier.x + speedOffset.x,
                                randomSpeedDistribution(randomFloatGenerator) * speedMultiplier.y + speedOffset.y,
                                randomSpeedDistribution(randomFloatGenerator) * speedMultiplier.z + speedOffset.z)
                      / (float) TICK_PER_SECOND;
    speeds.emplace_back(speed);
    creationTime.emplace_back(time);
    //std::cout << "Add particle with position " << position.x << ", " <<position.y << ", " <<position.z << std::endl;
}

void Emitter::setupVAO() {
    std::vector<glm::vec3> vertices;

    vertices.emplace_back(glm::vec3(-1.0f, 1.0f, 0.0f));
    vertices.emplace_back(glm::vec3(-1.0f, -1.0f, 0.0f));
    vertices.emplace_back(glm::vec3(1.0f, 1.0f, 0.0f));
    vertices.emplace_back(glm::vec3(1.0f, -1.0f, 0.0f));

    std::vector<glm::u16vec3> faces;

    faces.emplace_back(glm::uvec3(0, 1, 2));
    faces.emplace_back(glm::uvec3(2, 1, 3));

    std::vector<glm::vec2> textureCoordinates;

    textureCoordinates.emplace_back(0.0f, 1.0f);
    textureCoordinates.emplace_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.emplace_back(glm::vec2(1.0f, 1.0f));
    textureCoordinates.emplace_back(glm::vec2(1.0f, 0.0f));


    uint32_t vbo;
    graphicsWrapper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    graphicsWrapper->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 3);
    bufferObjects.push_back(vbo);
}

ImGuiResult Emitter::addImGuiEditorElements(const ImGuiRequest &request) {

    //Allow transformation editing.
    if(transformation.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix, false, parentObject != nullptr)) {
        //true means transformation changed, activate rigid body
    }

    if(parentObject != nullptr) {
        const glm::vec3 worldPosition(this->transformation.getWorldTransform()[3]);
        ImGui::Text("World Position X: %.3f", worldPosition.x);
        ImGui::Text("World Position Y: %.3f", worldPosition.y);
        ImGui::Text("World Position Z: %.3f", worldPosition.z);
        ImGui::NewLine();
    }

    if(ImGui::Checkbox("Enabled##ParticleEmitter", &enabled)) {
        lastSetupTime = 0;//sets up creation
    }

    ImGui::Text("Active Particles: %ld / %ld", currentCount, maxCount);

    float startDistanceValues[3];
    startDistanceValues[0] = maxStartDistances.x;
    startDistanceValues[1] = maxStartDistances.y;
    startDistanceValues[2] = maxStartDistances.z;
    if(ImGui::DragFloat3("Maximum Start Distances##ParticleEmitter", startDistanceValues, 0.05f, 0.0f, FLT_MAX)) {
        maxStartDistances.x = startDistanceValues[0];
        maxStartDistances.y = startDistanceValues[1];
        maxStartDistances.z = startDistanceValues[2];
    }
    ImGui::SameLine();
    ImGuiHelper::ShowHelpMarker("Half-extent of the box particles randomly spawn within, centered on the emitter.");

    int maxCountTemp = (int) maxCount;
    if(ImGui::DragInt("Maximum particle count##ParticleEmitter", &maxCountTemp, 1, 1, INT_MAX)) {
        maxCount = maxCountTemp;
        particleDataTexture = std::make_shared<Texture>(this->graphicsWrapper,
                                                        GraphicsInterface::TextureTypes::T2D,
                                                        GraphicsInterface::InternalFormatTypes::RGBA32F,
                                                        GraphicsInterface::FormatTypes::RGBA,
                                                        GraphicsInterface::DataTypes::FLOAT,
                                                        maxCount, 1);
        perMsParticleCount = (float) maxCount / lifeTime;
        currentCount = 0;
        positions.clear();
        speeds.clear();
        creationTime.clear();
    }

    int lifeTimeTemp = (int) lifeTime;
    if(ImGui::DragInt("Life time##ParticleEmitter", &lifeTimeTemp, 10, 1, INT_MAX)) {
        lifeTime = lifeTimeTemp;
        perMsParticleCount = (float) maxCount / lifeTime;
        lastSetupTime = 0;//sets up creation
    }

    ImGui::Text("Particles per ms: %.3f (Maximum particle count / Life time)", perMsParticleCount);

    if(ImGui::Checkbox("Continuous Emitting##ParticleEmitter", &continuousEmit)) {
        lastSetupTime = 0;//sets up creation
    }

    ImGui::DragFloat2("Size##ParticleEmitter", glm::value_ptr(size), 0.01f, 0.01f, FLT_MAX);

    float speedOffsetValues[3];
    speedOffsetValues[0] = speedOffset.x;
    speedOffsetValues[1] = speedOffset.y;
    speedOffsetValues[2] = speedOffset.z;
    if(ImGui::DragFloat3("Speed Offset##ParticleEmitter", speedOffsetValues, 0.01f)) {
        speedOffset = glm::vec3(speedOffsetValues[0], speedOffsetValues[1], speedOffsetValues[2]);
    }
    ImGui::SameLine();
    ImGuiHelper::ShowHelpMarker("Constant per-axis launch velocity in meters/second (1 world unit = 1 meter), added on top of Speed Multiplier's random spread. See the green arrows in the viewport.");

    float speedMultiplierValues[3];
    speedMultiplierValues[0] = speedMultiplier.x;
    speedMultiplierValues[1] = speedMultiplier.y;
    speedMultiplierValues[2] = speedMultiplier.z;
    if(ImGui::DragFloat3("Speed multiplier##ParticleEmitter", speedMultiplierValues, 0.01f)) {
        speedMultiplier = glm::vec3(speedMultiplierValues[0], speedMultiplierValues[1], speedMultiplierValues[2]);
    }
    ImGui::SameLine();
    ImGuiHelper::ShowHelpMarker("Scales a random per-axis launch velocity in [-1, 1] meters/second before Speed Offset is added -- this is the spread between the min/max green arrows in the viewport.");

    float gravityValues[3];
    gravityValues[0] = gravity.x;
    gravityValues[1] = gravity.y;
    gravityValues[2] = gravity.z;
    if(ImGui::DragFloat3("Gravity##ParticleEmitter", gravityValues, 0.01f)) {
        gravity = glm::vec3(gravityValues[0], gravityValues[1], gravityValues[2]);
    }
    ImGui::SameLine();
    ImGuiHelper::ShowHelpMarker("Per-axis acceleration in meters/second^2 (e.g. -9.8 for Earth-like gravity on Y).");

    ImGui::NewLine();
    ImGui::Text("Color Multipliers Over Particle Age");
    ImGui::SameLine();
    ImGuiHelper::ShowHelpMarker("Multiplies the sprite's color over each particle's own lifetime -- every particle replays this timeline from its own spawn, not the emitter's age or world clock.");

    static int selectedColorMultiplierIndex = -1;
    if(selectedColorMultiplierIndex >= (int)timedColorMultipliers.size()) {
        selectedColorMultiplierIndex = -1;
    }
    for (size_t i = 0; i < timedColorMultipliers.size(); ++i) {
        const TimedColorMultiplier& multiplier = timedColorMultipliers[i];
        ImVec4 previewColor(multiplier.colorMultiplier.x / 255.0f, multiplier.colorMultiplier.y / 255.0f,
                            multiplier.colorMultiplier.z / 255.0f, multiplier.colorMultiplier.w / 255.0f);
        ImGui::ColorButton(("##ColorPreview" + std::to_string(i) + "ParticleEmitter").c_str(), previewColor,
                           ImGuiColorEditFlags_NoTooltip, ImVec2(20.0f, 20.0f));
        ImGui::SameLine();
        std::string label = "Age: " + std::to_string(multiplier.time) + " ms##ParticleEmitter" + std::to_string(i);
        if(ImGui::Selectable(label.c_str(), selectedColorMultiplierIndex == (int)i)) {
            selectedColorMultiplierIndex = (int)i;
        }
    }

    if(selectedColorMultiplierIndex != -1) {
        ImGui::Indent( 16.0f );
        TimedColorMultiplier& multiplier = timedColorMultipliers[selectedColorMultiplierIndex];
        float colorValues[4];
        colorValues[0] = multiplier.colorMultiplier.x / 255.0f;
        colorValues[1] = multiplier.colorMultiplier.y / 255.0f;
        colorValues[2] = multiplier.colorMultiplier.z / 255.0f;
        colorValues[3] = multiplier.colorMultiplier.w / 255.0f;
        if(ImGui::ColorEdit4(("Color##" + std::to_string(selectedColorMultiplierIndex) + "ParticleEmitter").c_str(), colorValues)) {
            multiplier.colorMultiplier.x = (unsigned int) std::lround(colorValues[0] * 255.0f);
            multiplier.colorMultiplier.y = (unsigned int) std::lround(colorValues[1] * 255.0f);
            multiplier.colorMultiplier.z = (unsigned int) std::lround(colorValues[2] * 255.0f);
            multiplier.colorMultiplier.w = (unsigned int) std::lround(colorValues[3] * 255.0f);
        }
        int minTime, maxTime;
        if(selectedColorMultiplierIndex == 0) {
            minTime = 0;
        } else {
            minTime = timedColorMultipliers[selectedColorMultiplierIndex-1].time +1;
        }

        if((size_t)selectedColorMultiplierIndex == timedColorMultipliers.size()-1) {
            maxTime = lifeTime;
        } else {
            maxTime = timedColorMultipliers[selectedColorMultiplierIndex+1].time -1;
        }

        int time = multiplier.time;
        if(ImGui::DragInt(("Particle Age (ms)##" + std::to_string(selectedColorMultiplierIndex) + "ParticleEmitter").c_str(), &time, 10, minTime, maxTime)) {
            if(time > maxTime) {
                time = maxTime;
            } else if(time < minTime) {
                time = minTime;
            }
            multiplier.time = time;
        }
        ImGui::SameLine();
        ImGuiHelper::ShowHelpMarker("Milliseconds since this particle was created -- each particle replays this timeline from its own spawn, not the emitter's age or world clock.");

        if(ImGui::Button(("Remove Timed Color Shift##" + std::to_string(selectedColorMultiplierIndex) + "ParticleEmitter").c_str())) {
            timedColorMultipliers.erase(timedColorMultipliers.begin()+selectedColorMultiplierIndex);
            selectedColorMultiplierIndex = -1;
        }
        ImGui::Unindent( 16.0f );
    }

    if(ImGui::Button(("Add Timed Color Shift##" + std::to_string(selectedColorMultiplierIndex) + "ParticleEmitter").c_str())) {
        TimedColorMultiplier multiplier;
        if(selectedColorMultiplierIndex < 0) {
            if(timedColorMultipliers.empty()) {
                multiplier.time = 0;
            } else {
                multiplier.colorMultiplier = timedColorMultipliers[timedColorMultipliers.size()-1].colorMultiplier;
                multiplier.time            = timedColorMultipliers[timedColorMultipliers.size()-1].time;
            }
            timedColorMultipliers.emplace_back(multiplier);
        } else {
            multiplier.colorMultiplier = timedColorMultipliers[selectedColorMultiplierIndex].colorMultiplier;
            multiplier.time            = timedColorMultipliers[selectedColorMultiplierIndex].time +1;
            timedColorMultipliers.insert(timedColorMultipliers.begin() + selectedColorMultiplierIndex+1, multiplier);
        }
    }

    ImGuiResult imGuiResult;
    if(ImGui::Button("Remove##ParticleEmitter")) {
        imGuiResult.remove = true;
    }
    return imGuiResult;
}

void Emitter::drawDebugLine(Logger *logger, uint32_t &bufferId, const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color) const {
    //first segment creates the buffer; every later segment appends to the same one.
    if(bufferId == 0) {
        bufferId = logger->drawLine(from, to, color, color, true);
    } else {
        logger->drawLine(bufferId, from, to, color, color, true);
    }
}

void Emitter::drawDebugBox(Logger *logger, uint32_t &bufferId, const glm::vec3 &boxMin, const glm::vec3 &boxMax, const glm::vec3 &color) const {
    glm::vec3 corners[8];
    for (int i = 0; i < 8; ++i) {
        corners[i] = glm::vec3((i & 1) ? boxMax.x : boxMin.x,
                               (i & 2) ? boxMax.y : boxMin.y,
                               (i & 4) ? boxMax.z : boxMin.z);
    }
    //edges connect corners that differ in exactly one bit
    static const int edgePairs[12][2] = {{0,1},{0,2},{0,4},{1,3},{1,5},{2,3},
                                         {2,6},{3,7},{4,5},{4,6},{5,7},{6,7}};
    for (int edgeIndex = 0; edgeIndex < 12; ++edgeIndex) {
        drawDebugLine(logger, bufferId, corners[edgePairs[edgeIndex][0]], corners[edgePairs[edgeIndex][1]], color);
    }
}

void Emitter::drawDebugArrow(Logger *logger, uint32_t &bufferId, const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color) const {
    drawDebugLine(logger, bufferId, from, to, color);
    glm::vec3 direction = to - from;
    float length = glm::length(direction);
    if(length < 1e-5f) {
        return;//zero vector, nothing to put an arrowhead on
    }
    direction /= length;
    glm::vec3 helperAxis = (std::abs(direction.y) < 0.99f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
    glm::vec3 perpendicular = glm::normalize(glm::cross(direction, helperAxis));
    const float headLength = glm::min(length * 0.25f, 0.3f);
    const float headWidth = headLength * 0.5f;
    glm::vec3 headBase = to - direction * headLength;
    drawDebugLine(logger, bufferId, to, headBase + perpendicular * headWidth, color);
    drawDebugLine(logger, bufferId, to, headBase - perpendicular * headWidth, color);
}

float Emitter::trajectoryDisplacement(float v0, float gravityAxis, float k, float ticksPerSecond) {
    return v0 * k / ticksPerSecond + gravityAxis * k * (k - 1.0f) / (2.0f * ticksPerSecond * ticksPerSecond);
}

void Emitter::trajectoryRangeAtAge(float v0Min, float v0Max, float gravityAxis, float k, float ticksPerSecond, float &outMin, float &outMax) {
    float atV0Min = trajectoryDisplacement(v0Min, gravityAxis, k, ticksPerSecond);
    float atV0Max = trajectoryDisplacement(v0Max, gravityAxis, k, ticksPerSecond);
    outMin = glm::min(atV0Min, atV0Max);
    outMax = glm::max(atV0Min, atV0Max);
}

float Emitter::expectedTrajectorySpeed(float k, float ticksPerSecond) const {
    return glm::length(speedOffset + gravity * (k / ticksPerSecond));
}

float Emitter::ageAtArcLength(const std::vector<float> &cumulativeArcLength, int integrationSamples, float stepCount, float targetArcLength) {
    int i = 0;
    while(i < integrationSamples && cumulativeArcLength[i + 1] < targetArcLength) {
        ++i;
    }
    float segmentStart = cumulativeArcLength[i];
    float segmentEnd = cumulativeArcLength[i + 1];
    float t = (segmentEnd > segmentStart) ? (targetArcLength - segmentStart) / (segmentEnd - segmentStart) : 0.0f;
    float kStart = stepCount * (float) i / (float) integrationSamples;
    float kEnd = stepCount * (float) (i + 1) / (float) integrationSamples;
    return kStart + t * (kEnd - kStart);
}

void Emitter::renderDebugVisualization(Logger *logger, uint32_t &bufferId) const {
    if(bufferId != 0) {
        logger->clearLineBuffer(bufferId);
        bufferId = 0;
    }

    const glm::vec3 worldPosition(this->transformation.getWorldTransform()[3]);
    const glm::vec3 positionMarkerColor(1.0f, 1.0f, 0.0f);//yellow
    const glm::vec3 spawnBoxColor(0.0f, 1.0f, 1.0f);//cyan
    const glm::vec3 velocityColor(0.0f, 1.0f, 0.0f);//green
    const glm::vec3 trajectoryHullColor(1.0f, 0.5f, 0.0f);//orange

    //1) where the emitter is
    const float markerHalfSize = 0.15f;
    drawDebugLine(logger, bufferId, worldPosition - glm::vec3(markerHalfSize, 0, 0), worldPosition + glm::vec3(markerHalfSize, 0, 0), positionMarkerColor);
    drawDebugLine(logger, bufferId, worldPosition - glm::vec3(0, markerHalfSize, 0), worldPosition + glm::vec3(0, markerHalfSize, 0), positionMarkerColor);
    drawDebugLine(logger, bufferId, worldPosition - glm::vec3(0, 0, markerHalfSize), worldPosition + glm::vec3(0, 0, markerHalfSize), positionMarkerColor);

    //2) where particles spawn. addRandomParticle never rotates this offset by the emitter's orientation
    const glm::vec3 spawnBoxMin = worldPosition - maxStartDistances;
    const glm::vec3 spawnBoxMax = worldPosition + maxStartDistances;
    drawDebugBox(logger, bufferId, spawnBoxMin, spawnBoxMax, spawnBoxColor);

    //3) which way they start moving: min/average/max. 1 world unit = 1 meter, so the raw vector already reads as meters covered in one second
    const glm::vec3 velocityMin = speedOffset - speedMultiplier;
    const glm::vec3 velocityAvg = speedOffset;
    const glm::vec3 velocityMax = speedOffset + speedMultiplier;
    drawDebugArrow(logger, bufferId, worldPosition, worldPosition + velocityMin, velocityColor);
    drawDebugArrow(logger, bufferId, worldPosition, worldPosition + velocityAvg, velocityColor);
    drawDebugArrow(logger, bufferId, worldPosition, worldPosition + velocityMax, velocityColor);

    //4) volume particles sweep through after spawn: convex hull of sampled per-age boxes, not a single AABB,
    //since a particle can't sit at every axis' extreme at once. N assumes World::play's locked timestep.
    const float ticksPerSecond = (float) TICK_PER_SECOND;
    const float stepCount = std::round((float) lifeTime * ticksPerSecond / 1000.0f);
    if(stepCount > 0.0f) {
        //cumulative arc length of the mean trajectory, used only to bias slice placement toward where it moves fastest
        const int integrationSamples = (int) glm::clamp(stepCount, 1.0f, 512.0f);
        std::vector<float> cumulativeArcLength(integrationSamples + 1, 0.0f);
        for (int i = 1; i <= integrationSamples; ++i) {
            const float kPrev = stepCount * (float) (i - 1) / (float) integrationSamples;
            const float kCurr = stepCount * (float) i / (float) integrationSamples;
            cumulativeArcLength[i] = cumulativeArcLength[i - 1] + expectedTrajectorySpeed((kPrev + kCurr) * 0.5f, ticksPerSecond) * (kCurr - kPrev) / ticksPerSecond;
        }
        const float totalArcLength = cumulativeArcLength[integrationSamples];

        const int hullSliceCount = 8;
        std::vector<float> sliceAges;
        sliceAges.reserve(hullSliceCount + 3);
        for (int i = 0; i < hullSliceCount; ++i) {
            if(totalArcLength > 1e-6f) {
                const float targetArcLength = totalArcLength * (float) i / (float) (hullSliceCount - 1);
                sliceAges.push_back(ageAtArcLength(cumulativeArcLength, integrationSamples, stepCount, targetArcLength));
            } else {
                //expected trajectory barely moves (near-zero speedOffset and gravity): fall back to even spacing
                sliceAges.push_back(stepCount * (float) i / (float) (hullSliceCount - 1));
            }
        }

        //only forced in if the turning point actually falls within the particle's life
        for (int axis = 0; axis < 3; ++axis) {
            const float gravityAxis = gravity[axis];
            if(std::abs(gravityAxis) > 1e-6f) {
                const float turningAge = 0.5f - speedOffset[axis] * ticksPerSecond / gravityAxis;
                if(turningAge > 0.0f && turningAge < stepCount) {
                    sliceAges.push_back(turningAge);
                }
            }
        }
        std::sort(sliceAges.begin(), sliceAges.end());

        //merges ages within 1% of stepCount of each other (e.g. a forced turning point landing next to a sample)
        const float ageMergeTolerance = stepCount * 0.01f;
        std::vector<float> dedupedSliceAges;
        dedupedSliceAges.reserve(sliceAges.size());
        for (size_t i = 0; i < sliceAges.size(); ++i) {
            if(dedupedSliceAges.empty() || (sliceAges[i] - dedupedSliceAges.back()) >= ageMergeTolerance) {
                dedupedSliceAges.push_back(sliceAges[i]);
            }
        }

        //8 corners per slice age, the exact extremes of that age's box, all fed into one hull
        std::vector<glm::vec3> hullPoints;
        hullPoints.reserve(dedupedSliceAges.size() * 8);
        for (size_t sliceIndex = 0; sliceIndex < dedupedSliceAges.size(); ++sliceIndex) {
            const float age = dedupedSliceAges[sliceIndex];
            glm::vec3 sliceMin, sliceMax;
            for (int axis = 0; axis < 3; ++axis) {
                float rangeMin, rangeMax;
                trajectoryRangeAtAge(velocityMin[axis], velocityMax[axis], gravity[axis], age, ticksPerSecond, rangeMin, rangeMax);
                sliceMin[axis] = spawnBoxMin[axis] + rangeMin;
                sliceMax[axis] = spawnBoxMax[axis] + rangeMax;
            }
            for (int corner = 0; corner < 8; ++corner) {
                hullPoints.emplace_back((corner & 1) ? sliceMax.x : sliceMin.x,
                                        (corner & 2) ? sliceMax.y : sliceMin.y,
                                        (corner & 4) ? sliceMax.z : sliceMin.z);
            }
        }

        btConvexHullComputer hullComputer;
        hullComputer.compute(glm::value_ptr(hullPoints[0]), sizeof(glm::vec3), (int) hullPoints.size(), 0.0f, 0.0f);
        const btConvexHullComputer::Edge* edgeArrayBase = &hullComputer.edges[0];
        for (int i = 0; i < hullComputer.edges.size(); ++i) {
            const btConvexHullComputer::Edge &edge = hullComputer.edges[i];
            const int reverseIndex = (int) (edge.getReverseEdge() - edgeArrayBase);
            if(i < reverseIndex) {//draw each undirected edge exactly once
                const glm::vec3 from = GLMConverter::BltToGLM(hullComputer.vertices[edge.getSourceVertex()]);
                const glm::vec3 to = GLMConverter::BltToGLM(hullComputer.vertices[edge.getTargetVertex()]);
                drawDebugLine(logger, bufferId, from, to, trajectoryHullColor);
            }
        }
    }
}

float Emitter::calculateTimedColorShift(const long time, const long particleCreateTime) {
    if(timedColorMultipliers.empty()) {
        return packToFloat(glm::uvec4 (255,255,255,255));
    }
    if(timedColorMultipliers.size() == 1) {
        return packToFloat(timedColorMultipliers[0].colorMultiplier);
    }
    long spendTime = time - particleCreateTime;

    if(timedColorMultipliers[timedColorMultipliers.size() - 1].time <= spendTime) {
        //last element, no need to interpolate
        return packToFloat(timedColorMultipliers[timedColorMultipliers.size() - 1].colorMultiplier);
    }

    size_t timedColorMultiplierIndex = 1;
    while(timedColorMultiplierIndex < timedColorMultipliers.size() - 1 &&
          spendTime > timedColorMultipliers[timedColorMultiplierIndex].time ){
        timedColorMultiplierIndex++;
    }

    const TimedColorMultiplier& from = timedColorMultipliers[timedColorMultiplierIndex - 1];
    const TimedColorMultiplier& to   = timedColorMultipliers[timedColorMultiplierIndex];
    //spend time interpolation factor
    float factor = (float)(spendTime - from.time) / (float)(to.time - from.time);
    glm::uvec4 result = glm::mix(from.colorMultiplier, to.colorMultiplier, factor);
    return packToFloat(result);
}

void Emitter::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *emittersNode) const {
    tinyxml2::XMLElement *emitterElement = document.NewElement("Emitter");
    emittersNode->InsertEndChild(emitterElement);

    XMLHelper::writeElement(document, emitterElement, "ID", std::to_string(worldObjectID));
    XMLHelper::writeElement(document, emitterElement, "Name", name);

    if(getParentBoneID() != -1) {
        transformation.serializeLocal(document, emitterElement);
    } else {
        transformation.serialize(document, emitterElement);
    }

    tinyxml2::XMLElement *gravityNode = document.NewElement("Gravity");
    XMLHelper::writeVec3(document, gravityNode, gravity);
    emitterElement->InsertEndChild(gravityNode);

    tinyxml2::XMLElement *speedMultiplierNode = document.NewElement("SpeedMultiplier");
    XMLHelper::writeVec3(document, speedMultiplierNode, speedMultiplier);
    emitterElement->InsertEndChild(speedMultiplierNode);

    tinyxml2::XMLElement *speedOffsetNode = document.NewElement("SpeedOffset");
    XMLHelper::writeVec3(document, speedOffsetNode, speedOffset);
    emitterElement->InsertEndChild(speedOffsetNode);

    tinyxml2::XMLElement *sizeNode = document.NewElement("Size");
    XMLHelper::writeElement(document, sizeNode, "X", size.x);
    XMLHelper::writeElement(document, sizeNode, "Y", size.y);
    emitterElement->InsertEndChild(sizeNode);

    XMLHelper::writeElement(document, emitterElement, "MaxCount", std::to_string(maxCount));
    XMLHelper::writeElement(document, emitterElement, "LifeTime", std::to_string(lifeTime));
    XMLHelper::writeElement(document, emitterElement, "ContinuousEmitting", continuousEmit ? "True" : "False");
    XMLHelper::writeElement(document, emitterElement, "Enabled", enabled ? "True" : "False");

    tinyxml2::XMLElement *maxStartDistancesNode = document.NewElement("MaximumStartDistances");
    XMLHelper::writeVec3(document, maxStartDistancesNode, maxStartDistances);
    emitterElement->InsertEndChild(maxStartDistancesNode);

    XMLHelper::writeElement(document, emitterElement, "Texture", texture->getName());

    tinyxml2::XMLElement *timedColorMultipliersNode = document.NewElement("TimedColorMultipliers");
    for (const TimedColorMultiplier& multiplier : timedColorMultipliers) {
        tinyxml2::XMLElement *timedColorElement = document.NewElement("TimedColorMultiplier");
        XMLHelper::writeElement(document, timedColorElement, "R", multiplier.colorMultiplier.x);
        XMLHelper::writeElement(document, timedColorElement, "G", multiplier.colorMultiplier.y);
        XMLHelper::writeElement(document, timedColorElement, "B", multiplier.colorMultiplier.z);
        XMLHelper::writeElement(document, timedColorElement, "A", multiplier.colorMultiplier.w);
        XMLHelper::writeElement(document, timedColorElement, "Time", std::to_string(multiplier.time));
        timedColorMultipliersNode->InsertEndChild(timedColorElement);
    }
    emitterElement->InsertEndChild(timedColorMultipliersNode);

    if(getParentObject() != nullptr) {
        const GameObject* parentGO = dynamic_cast<const GameObject*>(getParentObject());
        if(parentGO != nullptr) {
            XMLHelper::writeElement(document, emitterElement, "ParentID", parentGO->getWorldObjectID());
        }
        if(getParentBoneID() != -1) {
            XMLHelper::writeElement(document, emitterElement, "ParentBoneID", getParentBoneID());
        }
    }
}

Emitter *Emitter::deserialize(tinyxml2::XMLElement *emitterNode, std::shared_ptr<AssetManager> assetManager,
                               bool &hasParent, uint32_t &parentID, int32_t &parentBoneID) {
    hasParent = false;

    std::string maxCountStr;
    if(!XMLHelper::readRequiredText(emitterNode, "MaxCount", maxCountStr, "Particle emitter must have a maximum particle count.")) {
        return nullptr;
    }
    long maxCount = std::stoul(maxCountStr);

    std::string lifeTimeStr;
    if(!XMLHelper::readRequiredText(emitterNode, "LifeTime", lifeTimeStr, "Particle emitter must have a life time.")) {
        return nullptr;
    }
    long lifeTime = std::stoul(lifeTimeStr);

    std::string textureFile;
    if(!XMLHelper::readRequiredText(emitterNode, "Texture", textureFile, "Particle emitter must have a Texture.")) {
        return nullptr;
    }

    std::string idStr;
    if(!XMLHelper::readRequiredText(emitterNode, "ID", idStr, "Particle emitter does not have ID. This is invalid!")) {
        return nullptr;
    }
    long id = std::stoul(idStr);

    bool continuousEmit = true;
    std::string continuousEmitStr;
    if(!XMLHelper::readOptionalText(emitterNode, "ContinuousEmitting", continuousEmitStr)) {
        std::cerr << "Particle emitter does not have Continuous emitting set. Assuming true" << std::endl;
    } else if(continuousEmitStr == "False") {
        continuousEmit = false;
    } else if(continuousEmitStr != "True") {
        std::cerr << "Continuous emit setting unknown, assuming true " << std::endl;
    }

    bool enabled = true;
    std::string enabledStr;
    if(!XMLHelper::readOptionalText(emitterNode, "Enabled", enabledStr)) {
        std::cerr << "Particle emitter does not have Enabled set. Assuming true" << std::endl;
    } else if(enabledStr == "False") {
        enabled = false;
    } else if(enabledStr != "True") {
        std::cerr << "Enabled setting unknown, assuming true " << std::endl;
    }

    std::string name;
    if(!XMLHelper::readRequiredText(emitterNode, "Name", name, "Particle emitter does not have Name. This is invalid!")) {
        return nullptr;
    }

    glm::vec3 startPosition;
    tinyxml2::XMLElement* emitterTransformationElement = emitterNode->FirstChildElement("Transformation");
    if(emitterTransformationElement != nullptr) {
        tinyxml2::XMLElement* translateEl = emitterTransformationElement->FirstChildElement("Translate");
        if(translateEl == nullptr || !XMLHelper::readVec3(translateEl, startPosition)) {
            std::cerr << "Emitter Transformation is missing Translate." << std::endl;
            return nullptr;
        }
    } else {
        tinyxml2::XMLElement* startPositionEl = emitterNode->FirstChildElement("StartPosition");
        if (startPositionEl == nullptr) {
            std::cerr << "Particle Emitter must have a position/direction." << std::endl;
            return nullptr;
        }
        if(!XMLHelper::readVec3(startPositionEl, startPosition)) {
            return nullptr;
        }
    }

    glm::vec3 maxStartDistances;
    tinyxml2::XMLElement* maxStartDistancesEl = emitterNode->FirstChildElement("MaximumStartDistances");
    if(maxStartDistancesEl == nullptr || !XMLHelper::readVec3(maxStartDistancesEl, maxStartDistances)) {
        std::cerr << "Particle Emitter must have a Maximum Start distance." << std::endl;
        return nullptr;
    }

    glm::vec2 size(1.0f, 1.0f);
    tinyxml2::XMLElement* sizeEl = emitterNode->FirstChildElement("Size");
    size.x = XMLHelper::readFloatOrDefault(sizeEl, "X", 1.0f);
    size.y = XMLHelper::readFloatOrDefault(sizeEl, "Y", 1.0f);

    glm::vec3 gravity(0,0,0);
    tinyxml2::XMLElement* gravityEl = emitterNode->FirstChildElement("Gravity");
    if(gravityEl == nullptr) {
        std::cout << "Particle Emitter has no gravity." << std::endl;
    }
    gravity.x = XMLHelper::readFloatOrDefault(gravityEl, "X", gravity.x);
    gravity.y = XMLHelper::readFloatOrDefault(gravityEl, "Y", gravity.y);
    gravity.z = XMLHelper::readFloatOrDefault(gravityEl, "Z", gravity.z);

    glm::vec3 speedMultiplier(1,1,1);
    tinyxml2::XMLElement* speedMultiplierEl = emitterNode->FirstChildElement("SpeedMultiplier");
    if(speedMultiplierEl == nullptr) {
        std::cout << "Particle Emitter has no speedMultiplier." << std::endl;
    }
    speedMultiplier.x = XMLHelper::readFloatOrDefault(speedMultiplierEl, "X", speedMultiplier.x);
    speedMultiplier.y = XMLHelper::readFloatOrDefault(speedMultiplierEl, "Y", speedMultiplier.y);
    speedMultiplier.z = XMLHelper::readFloatOrDefault(speedMultiplierEl, "Z", speedMultiplier.z);

    glm::vec3 speedOffset(0,0,0);
    tinyxml2::XMLElement* speedOffsetEl = emitterNode->FirstChildElement("SpeedOffset");
    if(speedOffsetEl == nullptr) {
        std::cout << "Particle Emitter has no gravity." << std::endl;
    }
    speedOffset.x = XMLHelper::readFloatOrDefault(speedOffsetEl, "X", speedOffset.x);
    speedOffset.y = XMLHelper::readFloatOrDefault(speedOffsetEl, "Y", speedOffset.y);
    speedOffset.z = XMLHelper::readFloatOrDefault(speedOffsetEl, "Z", speedOffset.z);

    std::vector<TimedColorMultiplier> multipliers;
    tinyxml2::XMLElement* timedColorMultipliersEl = emitterNode->FirstChildElement("TimedColorMultipliers");
    if (timedColorMultipliersEl == nullptr) {
        std::cout << "Particle Emitter has no Timed color shift." << std::endl;
    } else {
        tinyxml2::XMLElement* timedColorMultiplierElement = timedColorMultipliersEl->FirstChildElement("TimedColorMultiplier");
        while(timedColorMultiplierElement != nullptr) {
            TimedColorMultiplier timedColorMultiplier;
            std::string timeStr;
            if(!XMLHelper::readRequiredText(timedColorMultiplierElement, "Time", timeStr, "time can't be found for timed color multiplier, skipping!")) {
                timedColorMultiplierElement = timedColorMultiplierElement->NextSiblingElement("TimedColorMultiplier");
                continue;
            }
            timedColorMultiplier.time = std::atol(timeStr.c_str());
            timedColorMultiplier.colorMultiplier.r = static_cast<unsigned>(XMLHelper::readFloatOrDefault(timedColorMultiplierElement, "R", 255.0f));
            timedColorMultiplier.colorMultiplier.g = static_cast<unsigned>(XMLHelper::readFloatOrDefault(timedColorMultiplierElement, "G", 255.0f));
            timedColorMultiplier.colorMultiplier.b = static_cast<unsigned>(XMLHelper::readFloatOrDefault(timedColorMultiplierElement, "B", 255.0f));
            timedColorMultiplier.colorMultiplier.a = static_cast<unsigned>(XMLHelper::readFloatOrDefault(timedColorMultiplierElement, "A", 255.0f));
            multipliers.emplace_back(timedColorMultiplier);
            timedColorMultiplierElement = timedColorMultiplierElement->NextSiblingElement("TimedColorMultiplier");
        }
    }

    Emitter* emitter = new Emitter(id, name, assetManager, textureFile, startPosition, maxStartDistances, size, maxCount, lifeTime);
    if(emitterTransformationElement != nullptr) {
        emitter->getTransformation()->deserialize(emitterTransformationElement);
    }
    emitter->setGravity(gravity);
    emitter->setSpeedMultiplier(speedMultiplier);
    emitter->setSpeedOffset(speedOffset);
    emitter->setTimedColorMultipliers(multipliers);
    emitter->setContinuousEmit(continuousEmit);
    emitter->setEnabled(enabled);

    tinyxml2::XMLElement* parentEl = emitterNode->FirstChildElement("ParentID");
    if(parentEl != nullptr && parentEl->GetText() != nullptr) {
        parentID = std::stoul(parentEl->GetText());
        parentBoneID = -1;
        tinyxml2::XMLElement* parentBoneIDEl = emitterNode->FirstChildElement("ParentBoneID");
        if(parentBoneIDEl != nullptr && parentBoneIDEl->GetText() != nullptr) {
            parentBoneID = std::stoi(parentBoneIDEl->GetText());
        }
        hasParent = true;
    }

    return emitter;
}
