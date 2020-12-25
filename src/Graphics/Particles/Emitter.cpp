//
// Created by engin on 24.11.2020.
//

#include <Assets/TextureAsset.h>

#include <utility>
#include "ImGui/imgui.h"
#include "Emitter.h"

Emitter::Emitter(long worldObjectId, std::string name, std::shared_ptr<AssetManager> assetManager,
                 const std::string &textureFile, glm::vec3 startPosition, float startSphereR, glm::vec2 size, long count,
                 long lifeTime) :
        Renderable(assetManager->getGraphicsWrapper()), worldObjectID(worldObjectId), name(std::move(name)), size(size),
        maxCount(count), lifeTime(lifeTime), startSphereR(startSphereR),
        randomFloatGenerator(randomDevice()), randomStartingPoints(-1, 1),
        randomSpeedDistribution(-0.1f, 0.1f){ // generates random floats between - startSphereR and startSphereR
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
    this->perMsParticleCount = (float) maxCount / lifeTime;
}

void Emitter::addRandomParticle(const glm::vec3 &startPosition, float startSphereR, long time) {
    float x = randomStartingPoints(randomFloatGenerator) * startSphereR;
    float y = randomStartingPoints(randomFloatGenerator) * startSphereR;
    float z = startSphereR * startSphereR - (x * x + y * y);
    if (z > 0) {
        z = sqrt(z);
    } else {
        z = -1 * sqrt(-1 * z);
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

    ImGuiResult imGuiResult;
    if(ImGui::Button("Remove##ParticleEmitter")) {
        imGuiResult.remove = true;
    }
    return imGuiResult;
}
