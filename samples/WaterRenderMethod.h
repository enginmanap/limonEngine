//
// Created for the water render method tutorial.
//

#ifndef LIMONENGINE_WATERRENDERMETHOD_H
#define LIMONENGINE_WATERRENDERMETHOD_H

#include "limonAPI/Graphics/RenderMethodInterface.h"

class GraphicsInterface;

class WaterRenderMethod : public RenderMethodInterface {
    // Reserved bands are model=1, bone=2, shadow=3-4, material=5-9 (see GraphicsInterface.h);
    // the noise texture is owned entirely by this render method (not a pipeline-graph connection),
    // so a fixed unit above all reserved bands is used directly.
    static constexpr int32_t NOISE_TEXTURE_UNIT = 10;
    // Deliberately low-res: each texel holds an independent random value, so GL_LINEAR filtering
    // between them is what gives us smooth "value noise" hills. Too high a resolution here (relative
    // to the vertex shader's WAVE_SCALE) makes adjacent samples uncorrelated, which reads as flicker/
    // static rather than a wave - see Water/vertex.glsl for the matching scale constants.
    static constexpr int32_t NOISE_TEXTURE_SIZE = 32;
    // 16-bit indices (glm::u16vec3) cap the grid at this many vertices.
    static constexpr uint32_t MAX_GRID_VERTICES = 65535;

    uint32_t vao = 0;
    uint32_t ebo = 0;
    std::vector<uint32_t> bufferObjects;
    uint32_t noiseTexture = 0;
    uint32_t indexCount = 0;

    void generateMesh(double startX, double startZ, double endX, double endZ, double height, double triangleSize);
    void generateNoiseTexture();

public:
    explicit WaterRenderMethod(GraphicsInterface *graphicsInterface) : RenderMethodInterface(graphicsInterface) {}

    std::vector<LimonTypes::GenericParameter> getParameters() const override;

    bool initRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonTypes::GenericParameter> parameters) override;

    void renderFrame(std::shared_ptr<GraphicsProgram> program) override;

    bool cleanupRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonTypes::GenericParameter> parameters) override;

    std::string getName() const override {
        return "WaterRenderMethod";
    }
};

#endif //LIMONENGINE_WATERRENDERMETHOD_H
