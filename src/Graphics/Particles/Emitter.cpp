//
// Created by engin on 24.11.2020.
//

#include <Assets/TextureAsset.h>

#include <utility>
#include "ImGui/imgui.h"
#include "Emitter.h"

Emitter::Emitter(long worldObjectId, std::string name, std::shared_ptr<AssetManager> assetManager,
                 const std::string &textureFile, glm::vec3 startPosition, float startSphereR, glm::vec2 size, long count,
                 long lifeTime, float particlePerMs) :
        Renderable(assetManager->getGraphicsWrapper()), worldObjectID(worldObjectId), name(std::move(name)), size(size),
        maxCount(count), lifeTime(lifeTime), startSphereR(startSphereR),
        randomFloatGenerator(randomDevice()), randomStartingPoints(-1.0f, 1.0f),
        randomSpeedDistribution(-1.0f, 1.0f){
    this->transformation.setTranslate(startPosition);
    textureAsset.reset(assetManager->loadAsset<TextureAsset>({textureFile}));
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

void Emitter::addRandomParticle(const glm::vec3 &startPosition, float startSphereR, long time) {
    float x = randomStartingPoints(randomFloatGenerator) * startSphereR;
    float y = randomStartingPoints(randomFloatGenerator) * startSphereR;
    float z = randomStartingPoints(randomFloatGenerator) * startSphereR;
    if(x*x + y*y + z*z > startSphereR * startSphereR) {
        if(x > y && x > z) {
            x = 0;
        } else if(y > x && y > z) {
            y = 0;
        } else {
            z = 0;
        }
    }
    glm::vec4 position = glm::vec4(startPosition, 0) +
                         glm::vec4(x, y, z, 1);
    positions.emplace_back(position);
    glm::vec3 speed = glm::vec3(randomSpeedDistribution(randomFloatGenerator) * speedMultiplier.x + speedOffset.x,
                                randomSpeedDistribution(randomFloatGenerator) * speedMultiplier.y + speedOffset.y,
                                randomSpeedDistribution(randomFloatGenerator) * speedMultiplier.z + speedOffset.z);
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

    std::vector<glm::mediump_uvec3> faces;

    faces.emplace_back(glm::mediump_uvec3(0, 1, 2));
    faces.emplace_back(glm::mediump_uvec3(2, 1, 3));

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

GameObject::ImGuiResult Emitter::addImGuiEditorElements(const GameObject::ImGuiRequest &request [[gnu::unused]]) {
    float startPositionValues[3];
    startPositionValues[0] = transformation.getTranslate().x;
    startPositionValues[1] = transformation.getTranslate().y;
    startPositionValues[2] = transformation.getTranslate().z;
    if(ImGui::InputFloat3("Start Position##ParticleEmitter", startPositionValues)) {
        transformation.setTranslate(glm::vec3(startPositionValues[0], startPositionValues[1], startPositionValues[2]));
    }

    ImGui::InputFloat("Start Sphere R##ParticleEmitter", &startSphereR);

    uint32_t maxCountTemp = maxCount;
    if(ImGui::InputScalar("Maximum particle count##ParticleEmitter", ImGuiDataType_U32, &maxCountTemp)) {
        particleDataTexture = std::make_shared<Texture>(this->graphicsWrapper,
                                                        GraphicsInterface::TextureTypes::T2D,
                                                        GraphicsInterface::InternalFormatTypes::RGBA32F,
                                                        GraphicsInterface::FormatTypes::RGB,
                                                        GraphicsInterface::DataTypes::FLOAT,
                                                        maxCount, 1);

        maxCount = maxCountTemp;
        perMsParticleCount = (float) maxCount / lifeTime;
        currentCount = 0;
        positions.clear();
        speeds.clear();
        creationTime.clear();
    }

    uint32_t lifeTimeTemp = lifeTime;
    if(ImGui::InputScalar("Life time##ParticleEmitter", ImGuiDataType_U32, &lifeTimeTemp)) {
        lifeTime = lifeTimeTemp;
        lastSetupTime = 0;//sets up creation
    }

    ImGui::DragFloat("Particle per ms##ParticleEmitter", &perMsParticleCount, 0.01);

    if(ImGui::Checkbox("Continuous Emitting##ParticleEmitter", &continuousEmit)) {
        lastSetupTime = 0;//sets up creation
    }

    float* sizeValues = glm::value_ptr(size);
    if(ImGui::InputFloat2("Size##ParticleEmitter", sizeValues)) {
        size.x = sizeValues[0];
        size.y = sizeValues[1];
    }

    float speedOffsetValues[3];
    speedOffsetValues[0] = speedOffset.x;
    speedOffsetValues[1] = speedOffset.y;
    speedOffsetValues[2] = speedOffset.z;
    if(ImGui::InputFloat3("Speed Offset##ParticleEmitter", speedOffsetValues)) {
        speedOffset = glm::vec3(speedOffsetValues[0], speedOffsetValues[1], speedOffsetValues[2]);
    }

    float speedMultiplierValues[3];
    speedMultiplierValues[0] = speedMultiplier.x;
    speedMultiplierValues[1] = speedMultiplier.y;
    speedMultiplierValues[2] = speedMultiplier.z;
    if(ImGui::InputFloat3("Speed multiplier##ParticleEmitter", speedMultiplierValues)) {
        speedMultiplier = glm::vec3(speedMultiplierValues[0], speedMultiplierValues[1], speedMultiplierValues[2]);
    }

    float gravityValues[3];
    gravityValues[0] = gravity.x;
    gravityValues[1] = gravity.y;
    gravityValues[2] = gravity.z;
    if(ImGui::InputFloat3("Gravity##ParticleEmitter", gravityValues)) {
        gravity = glm::vec3(gravityValues[0], gravityValues[1], gravityValues[2]);
    }
    static int listbox_item_current = -1;
    ImGui::ListBox("ColorMultipliers##ParticleEmitter", &listbox_item_current, Emitter::getNameForTimedColorMultiplier,
                   static_cast<void *>(&this->timedColorMultipliers), this->timedColorMultipliers.size(), 10);
    
    if(listbox_item_current != -1) {
        ImGui::Indent( 16.0f );
        TimedColorMultiplier& multiplier = timedColorMultipliers[listbox_item_current];
        int colorValues[4];
        colorValues[0] = multiplier.colorMultiplier.x;
        colorValues[1] = multiplier.colorMultiplier.y;
        colorValues[2] = multiplier.colorMultiplier.z;
        colorValues[3] = multiplier.colorMultiplier.w;
        if(ImGui::DragInt4(("ColorMultiplier##" + std::to_string(listbox_item_current) + "ParticleEmitter").c_str(), colorValues, 1, 0, 255)) {
            for (int i = 0; i < 4; ++i) {
                if(colorValues[i] > 255) {
                    colorValues[i] = 255;
                } else if(colorValues[i] < 0) {
                    colorValues[i] = 0;
                }
            }
            multiplier.colorMultiplier.x = colorValues[0];
            multiplier.colorMultiplier.y = colorValues[1];
            multiplier.colorMultiplier.z = colorValues[2];
            multiplier.colorMultiplier.w = colorValues[3];
        }
        int minTime, maxTime;
        if(listbox_item_current == 0) {
            minTime = 0;
        } else {
            minTime = timedColorMultipliers[listbox_item_current-1].time +1;
        }

        if((size_t)listbox_item_current == timedColorMultipliers.size()-1) {
            maxTime = lifeTime;
        } else {
            maxTime = timedColorMultipliers[listbox_item_current+1].time -1;
        }

        //ImGui::SameLine();
        int time = multiplier.time;
        if(ImGui::DragInt(("Time##" + std::to_string(listbox_item_current) + "ParticleEmitter").c_str(), &time, 10, minTime, maxTime)) {
            if(time > maxTime) {
                time = maxTime;
            } else if(time < minTime) {
                time = minTime;
                time = minTime;
            }
            multiplier.time = time;
        }
        if(ImGui::Button(("Remove Timed Color Shift##" + std::to_string(listbox_item_current) + "ParticleEmitter").c_str())) {
            timedColorMultipliers.erase(timedColorMultipliers.begin()+listbox_item_current);
        }
        //At this point, the multiplier is invalid because of the removal, must verify
        ImGui::Unindent( 16.0f );
    }

    if(ImGui::Button(("Add Timed Color Shift##" + std::to_string(listbox_item_current) + "ParticleEmitter").c_str())) {
        TimedColorMultiplier multiplier;
        if(listbox_item_current < 0) {
            if(timedColorMultipliers.empty()) {
                multiplier.time = 0;
            } else {
                multiplier.colorMultiplier = timedColorMultipliers[timedColorMultipliers.size()-1].colorMultiplier;
                multiplier.time            = timedColorMultipliers[timedColorMultipliers.size()-1].time;
            }
            timedColorMultipliers.emplace_back(multiplier);
        } else {
            multiplier.colorMultiplier = timedColorMultipliers[listbox_item_current].colorMultiplier;
            multiplier.time            = timedColorMultipliers[listbox_item_current].time +1;
            timedColorMultipliers.insert(timedColorMultipliers.begin() + listbox_item_current+1, multiplier);
        }
    }

    ImGuiResult imGuiResult;
    if(ImGui::Button("Remove##ParticleEmitter")) {
        imGuiResult.remove = true;
    }
    return imGuiResult;
}

 bool Emitter::getNameForTimedColorMultiplier(void* data, int index, const char** outText) {
    std::vector<TimedColorMultiplier> multipliers = *static_cast<std::vector<TimedColorMultiplier>*>(data);
    if(index < 0 || (uint32_t)index >= multipliers.size()) {
        return false;
    }
    auto it = multipliers.begin();
    for (int i = 0; i < index; ++i) {
        it++;
    }
     char tempTextBuffer[128] = {0};//used for editor text buffer

    std::string timeString = std::to_string(it->time);
    std::copy(timeString.begin(), timeString.end(), tempTextBuffer);
    *outText = tempTextBuffer;
    return true;
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
