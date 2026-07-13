//
// Created for the water render method tutorial.
//

#include <random>
#include <cmath>
#include "WaterRenderMethod.h"

std::vector<LimonTypes::GenericParameter> WaterRenderMethod::getParameters() const {
    auto makeParameter = [](const std::string &description, double defaultValue) {
        LimonTypes::GenericParameter parameter;
        parameter.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
        parameter.valueType = LimonTypes::GenericParameter::ValueTypes::DOUBLE;
        parameter.description = description;
        parameter.value.doubleValue = defaultValue;
        parameter.isSet = true;//optional: editor allows saving with the default
        return parameter;
    };

    return std::vector<LimonTypes::GenericParameter>{
            makeParameter("Start X", -50.0),
            makeParameter("Start Z", -50.0),
            makeParameter("End X", 50.0),
            makeParameter("End Z", 50.0),
            makeParameter("Height", 0.0),
            makeParameter("Triangle size", 2.0),
    };
}

void WaterRenderMethod::generateMesh(double startX, double startZ, double endX, double endZ, double height, double triangleSize) {
    if (triangleSize <= 0.0) {
        triangleSize = 1.0;
    }
    double width = endX - startX;
    double depth = endZ - startZ;
    if (width <= 0.0 || depth <= 0.0) {
        std::cerr << "WaterRenderMethod: end coordinates must be greater than start coordinates, using a 1x1 fallback grid." << std::endl;
        width = depth = triangleSize;
        endX = startX + width;
        endZ = startZ + depth;
    }

    uint32_t cols = static_cast<uint32_t>(std::floor(width / triangleSize)) + 1;
    uint32_t rows = static_cast<uint32_t>(std::floor(depth / triangleSize)) + 1;
    if (cols < 2) { cols = 2; }
    if (rows < 2) { rows = 2; }

    // Mesh indices are 16-bit (glm::u16vec3), so the grid can't exceed this many vertices.
    if (static_cast<uint64_t>(cols) * static_cast<uint64_t>(rows) > MAX_GRID_VERTICES) {
        double minTriangleSize = std::sqrt((width * depth) / static_cast<double>(MAX_GRID_VERTICES - 1));
        std::cerr << "WaterRenderMethod: requested grid (" << cols << "x" << rows << ") exceeds the 16-bit index limit ("
                   << MAX_GRID_VERTICES << " vertices); clamping triangleSize from " << triangleSize << " to " << minTriangleSize << std::endl;
        triangleSize = minTriangleSize;
        cols = static_cast<uint32_t>(std::floor(width / triangleSize)) + 1;
        rows = static_cast<uint32_t>(std::floor(depth / triangleSize)) + 1;
        if (cols < 2) { cols = 2; }
        if (rows < 2) { rows = 2; }
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    vertices.reserve(cols * rows);
    uvs.reserve(cols * rows);

    for (uint32_t row = 0; row < rows; ++row) {
        double z = startZ + row * triangleSize;
        for (uint32_t col = 0; col < cols; ++col) {
            double x = startX + col * triangleSize;
            vertices.emplace_back(static_cast<float>(x), static_cast<float>(height), static_cast<float>(z));
            //world-unit UVs so the noise texture repeats at a consistent scale regardless of triangleSize
            uvs.emplace_back(static_cast<float>(x), static_cast<float>(z));
        }
    }

    std::vector<glm::u16vec3> faces;
    faces.reserve((cols - 1) * (rows - 1) * 2);
    for (uint32_t row = 0; row < rows - 1; ++row) {
        for (uint32_t col = 0; col < cols - 1; ++col) {
            uint16_t topLeft = static_cast<uint16_t>(row * cols + col);
            uint16_t topRight = static_cast<uint16_t>(topLeft + 1);
            uint16_t bottomLeft = static_cast<uint16_t>((row + 1) * cols + col);
            uint16_t bottomRight = static_cast<uint16_t>(bottomLeft + 1);

            faces.emplace_back(topLeft, bottomLeft, topRight);
            faces.emplace_back(topRight, bottomLeft, bottomRight);
        }
    }
    indexCount = static_cast<uint32_t>(faces.size() * 3);

    uint32_t vbo;
    graphicsInterface->bufferVertexData(vertices, faces, vao, vbo, 1, ebo);
    bufferObjects.push_back(vbo);

    graphicsInterface->bufferVertexTextureCoordinates(uvs, vao, vbo, 2);
    bufferObjects.push_back(vbo);
}

void WaterRenderMethod::generateNoiseTexture() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    std::vector<float> noiseValues;
    noiseValues.reserve(NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE);
    for (int32_t i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE; ++i) {
        noiseValues.push_back(randomFloats(generator));
    }

    noiseTexture = createTexture(NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, GraphicsInterface::TextureTypes::T2D,
                                  GraphicsInterface::InternalFormatTypes::RED, GraphicsInterface::FormatTypes::RED,
                                  GraphicsInterface::DataTypes::FLOAT, 0);
    setFilterMode(noiseTexture, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::FilterModes::LINEAR);
    setWrapMode(noiseTexture, GraphicsInterface::TextureTypes::T2D,
                GraphicsInterface::TextureWrapModes::REPEAT, GraphicsInterface::TextureWrapModes::REPEAT, GraphicsInterface::TextureWrapModes::REPEAT);
    loadTextureData(noiseTexture, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, GraphicsInterface::TextureTypes::T2D,
                     GraphicsInterface::InternalFormatTypes::RED, GraphicsInterface::FormatTypes::RED,
                     GraphicsInterface::DataTypes::FLOAT, 0, noiseValues.data(),
                     nullptr, nullptr, nullptr, nullptr, nullptr);
}

bool WaterRenderMethod::initRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonTypes::GenericParameter> parameters) {
    double startX = -50.0, startZ = -50.0, endX = 50.0, endZ = 50.0, height = 0.0, triangleSize = 2.0;
    if (parameters.size() >= 6) {
        startX = parameters[0].value.doubleValue;
        startZ = parameters[1].value.doubleValue;
        endX = parameters[2].value.doubleValue;
        endZ = parameters[3].value.doubleValue;
        height = parameters[4].value.doubleValue;
        triangleSize = parameters[5].value.doubleValue;
    }

    generateMesh(startX, startZ, endX, endZ, height, triangleSize);
    generateNoiseTexture();

    if (!program->setUniform("waterNoiseSampler", NOISE_TEXTURE_UNIT)) {
        std::cerr << "WaterRenderMethod: uniform \"waterNoiseSampler\" couldn't be set" << std::endl;
    }
    //Surface albedo authoring, tweak here (not related to the scene's actual lights, which are pulled
    //in for real by the fragment shader's calculateLighting() call).
    if (!program->setUniform("shallowColor", glm::vec3(0.15f, 0.55f, 0.55f))) {
        std::cerr << "WaterRenderMethod: uniform \"shallowColor\" couldn't be set" << std::endl;
    }
    if (!program->setUniform("deepColor", glm::vec3(0.02f, 0.10f, 0.18f))) {
        std::cerr << "WaterRenderMethod: uniform \"deepColor\" couldn't be set" << std::endl;
    }
    return false;
}

void WaterRenderMethod::renderFrame(std::shared_ptr<GraphicsProgram> program) {
    graphicsInterface->attachTexture(noiseTexture, NOISE_TEXTURE_UNIT);
    graphicsInterface->render(program->getID(), vao, ebo, indexCount);
}

bool WaterRenderMethod::cleanupRender(std::shared_ptr<GraphicsProgram> program [[gnu::unused]], std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {
    for (uint32_t bufferObject: bufferObjects) {
        graphicsInterface->freeBuffer(bufferObject);
    }
    bufferObjects.clear();
    graphicsInterface->freeBuffer(ebo);
    ebo = 0;
    graphicsInterface->freeVAO(vao);
    vao = 0;
    deleteTexture(noiseTexture);
    noiseTexture = 0;
    return true;
}
