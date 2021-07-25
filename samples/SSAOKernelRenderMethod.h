//
// Created by engin on 23.09.2019.
//

#ifndef LIMONENGINE_SSAOKERNELRENDERMETHOD_H
#define LIMONENGINE_SSAOKERNELRENDERMETHOD_H

#include "API/Graphics/RenderMethodInterface.h"

class GraphicsInterface;

class SSAOKernelRenderMethod : public RenderMethodInterface{
    uint32_t vao, ebo;
    std::vector<uint32_t> bufferObjects;
    uint32_t ssaoNoiseTexture;

    std::vector<glm::vec3> generateSSAOKernels(uint32_t kernelSize);
    void generateSSAONoiseTexture();

public:
    explicit SSAOKernelRenderMethod(GraphicsInterface* graphicsInterface) : RenderMethodInterface(graphicsInterface) {}

    std::vector<LimonTypes::GenericParameter> getParameters() const override {
        return std::vector<LimonTypes::GenericParameter>();
    }

    bool initRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) override;

    virtual void renderFrame(std::shared_ptr<GraphicsProgram> program[[gnu::unused]]) {
        graphicsInterface->attachTexture(ssaoNoiseTexture, graphicsInterface->getMaxTextureImageUnits()-3);

        graphicsInterface->render(program->getID(), vao, ebo, 3 * 2);//2 triangles
    };

    bool cleanupRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) override;

    std::string getName() const override {
        return "SSAOKernelRenderMethod";
    }

    void setupQuad();
};

extern "C" void registerRenderMethods(std::map<std::string, RenderMethodInterface*(*)(GraphicsInterface*)>* renderMethodMap);


#endif //LIMONENGINE_SSAOKERNELRENDERMETHOD_H
