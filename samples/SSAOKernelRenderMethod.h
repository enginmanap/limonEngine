//
// Created by engin on 23.09.2019.
//

#ifndef LIMONENGINE_SSAOKERNELRENDERMETHOD_H
#define LIMONENGINE_SSAOKERNELRENDERMETHOD_H

#include "API/Graphics/RenderMethodInterface.h"

class GraphicsInterface;

class SSAOKernelRenderMethod : public RenderMethodInterface{
    std::vector<glm::vec3> generateSSAOKernels(uint32_t kernelSize);
public:
    explicit SSAOKernelRenderMethod(GraphicsInterface* graphicsInterface) : RenderMethodInterface(graphicsInterface) {}

    std::vector<LimonAPI::ParameterRequest> getParameters() const override {
        return std::vector<LimonAPI::ParameterRequest>();
    }

    bool initRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonAPI::ParameterRequest> parameters [[gnu::unused]]) override;

    std::string getName() const override {
        return "SSAOKernelRenderMethod";
    }

};

extern "C" void registerRenderMethods(std::map<std::string, RenderMethodInterface*(*)(GraphicsInterface*)>* renderMethodMap);


#endif //LIMONENGINE_SSAOKERNELRENDERMETHOD_H
