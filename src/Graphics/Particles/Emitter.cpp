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

    if(ImGui::Checkbox("Enabled##ParticleEmitter", &enabled)) {
        lastSetupTime = 0;//sets up creation
    }
    float startPositionValues[3];
    startPositionValues[0] = transformation.getTranslate().x;
    startPositionValues[1] = transformation.getTranslate().y;
    startPositionValues[2] = transformation.getTranslate().z;
    if(ImGui::InputFloat3("Start Position##ParticleEmitter", startPositionValues)) {
        transformation.setTranslate(glm::vec3(startPositionValues[0], startPositionValues[1], startPositionValues[2]));
    }

    float startDistanceValues[3];
    startDistanceValues[0] = maxStartDistances.x;
    startDistanceValues[1] = maxStartDistances.y;
    startDistanceValues[2] = maxStartDistances.z;
    if(ImGui::InputFloat3("Maximum Start Distances##ParticleEmitter", startDistanceValues)) {
        maxStartDistances.x = startDistanceValues[0];
        maxStartDistances.y = startDistanceValues[1];
        maxStartDistances.z = startDistanceValues[2];
    }

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
