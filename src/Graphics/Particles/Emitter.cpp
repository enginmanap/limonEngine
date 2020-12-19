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
         maxCount(count), lifeTime(lifeTime), startPosition(startPosition), startSphereR(startSphereR),
        randomFloatGenerator(randomDevice()), randomFloats(-1 * startSphereR, startSphereR){ // generates random floats between - startSphereR and startSphereR

    textureAsset.reset(assetManager->loadAsset<TextureAsset>({textureFile}));
    this->texture = textureAsset->getTexture();

    setupVAO();

    positionTexture = std::make_shared<Texture>(this->graphicsWrapper,
                                                GraphicsInterface::TextureTypes::T2D,
                                                GraphicsInterface::InternalFormatTypes::RGB32F,
                                                GraphicsInterface::FormatTypes::RGB,
                                                GraphicsInterface::DataTypes::FLOAT,
                                                count, 1);
    this->perMsParticleCount = (float) count / lifeTime;
}

void Emitter::addRandomParticle(const glm::vec3 &startPosition, float startSphereR, long time) {
    float x = randomFloats(randomFloatGenerator);
    float y = randomFloats(randomFloatGenerator);
    float z = startSphereR * startSphereR - (x * x + y * y);
    if (z > 0) {
        z = sqrt(z);
    } else {
        z = -1 * sqrt(-1 * z);
    }
    glm::vec3 position = startPosition +
                         glm::vec3(x, y, z);
    positions.emplace_back(position);
    glm::vec3 speed = glm::vec3(randomFloats(randomFloatGenerator), randomFloats(randomFloatGenerator), randomFloats(
            randomFloatGenerator));
    speed.y += startSphereR + startSphereR * 1.f;
    speeds.emplace_back(speed);
    creationTime.emplace_back(time);
    std::cout << "Add particle with position " << position.x << ", " <<position.y << ", " <<position.z << std::endl;
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

GameObject::ImGuiResult Emitter::addImGuiEditorElements(const GameObject::ImGuiRequest &request) {
    ImGui::InputFloat3("startPosition##ParticleEmitter", glm::value_ptr(startPosition));
    ImGuiResult imGuiResult;
    if(ImGui::Button("Remove##ParticleEmitter")) {
        imGuiResult.remove = true;
    }
    return imGuiResult;
}
