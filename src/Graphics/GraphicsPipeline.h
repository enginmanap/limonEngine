//
// Created by engin on 31.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINE_H
#define LIMONENGINE_GRAPHICSPIPELINE_H


#include <vector>
#include <functional>
#include <memory>

#include "GraphicsPipelineStage.h"
#include "../GameObjects/Light.h"
class World;
class GraphicsProgram;

class GraphicsPipeline {
public:
    static std::vector<std::string> renderMethodNames;//This is not array, because custom effects might be loaded on runtime as extensions.

    class RenderMethods;
    class RenderMethod {
        friend class RenderMethods;

        std::string name;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> method;
        std::shared_ptr<GraphicsProgram> glslProgram;

        explicit RenderMethod(){} //private

    public:
        void operator()() {
            method(glslProgram);
        }

    };

    class RenderMethods {
        friend class World;//so it can fill it up.
    private:
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderOpaqueObjects;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderAnimatedObjects;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderTransparentObjects;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderGUITexts;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderGUIImages;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderPlayerAttachmentOpaque;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderPlayerAttachmentTransparent;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderPlayerAttachmentAnimated;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderSky;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderEditor;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderDebug;

        std::function<void(const std::shared_ptr<GraphicsProgram>&)> renderQuad;//For offscreen stuff

        //These methods are not exposed to the interface
        //They are also not possible to add to render pipeline, so a method should be created and assigned.
        std::function<std::vector<size_t>(Light::LightTypes)> getLightsByType;
        std::function<void(unsigned int, std::shared_ptr<GraphicsProgram>)> renderLight;

        std::function<void(const std::shared_ptr<GraphicsProgram>&)> getRenderMethodByName(const std::string& name, bool& found) const {
            found  = true;
            if(name == "None") {
                std::cerr << "Building graphics pipeline with empty method, are you sure that was set correctly?" << std::endl;
                return [](const std::shared_ptr<GraphicsProgram>& notUsed[[gnu::unused]]){};
            } else if(name == "Render Opaque Objects") {
                return  renderOpaqueObjects;
            } else if(name == "Render Animated Objects") {
                return renderAnimatedObjects;
            } else if(name == "Render Transparent Objects") {
                return renderTransparentObjects;
            } else if(name == "Render GUI Texts") {
                return renderGUITexts;
            } else if(name == "Render GUI Images") {
                return renderGUIImages;
            } else if(name == "Render Editor") {
                return renderEditor;
            } else if(name == "Render sky") {
                return renderSky;
            } else if(name == "Render Debug Information") {
                return renderDebug;
            } else if(name == "Render Opaque Player Attachment") {
                return renderPlayerAttachmentOpaque;
            } else if(name == "Render Transparent Player Attachment") {
                return renderPlayerAttachmentTransparent;
            } else if(name == "Render Animated Player Attachment") {
                return renderPlayerAttachmentAnimated;
            } else if(name == "Render quad") {
                return renderQuad;
            }
            found = false;
            return nullptr;
        }

        std::vector<size_t> getLightIndexes(Light::LightTypes lightType) const {
            return getLightsByType(lightType);
        }

        std::function<void(unsigned int, std::shared_ptr<GraphicsProgram>)>& getRenderLightMethod() {
            return renderLight;
        }

    public:
        RenderMethod getRenderMethod(const std::string& methodName, std::shared_ptr<GraphicsProgram> glslProgram, bool& isFound) const {
            RenderMethod renderMethod;
            renderMethod.method = getRenderMethodByName(methodName, isFound);
            if(!isFound) {
                return renderMethod;
            }
            renderMethod.name = methodName;
            renderMethod.glslProgram = glslProgram;
            return renderMethod;
        }

        RenderMethod getRenderMethodAllDirectionalLights(std::shared_ptr<GraphicsPipelineStage>& stage, std::shared_ptr<Texture>& layeredDepthMap, std::shared_ptr<GraphicsProgram> glslProgram) const {
            RenderMethod renderMethod;
            renderMethod.method = [&](const std::shared_ptr<GraphicsProgram> &renderProgram) {
                std::vector<size_t> lights = getLightIndexes(Light::LightTypes::DIRECTIONAL);
                for (size_t light:lights) {
                    //set the layer that will be rendered. Also set clear so attached layer will be cleared right away.
                    //this is important because they will not be cleared other way.
                    stage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, layeredDepthMap, true, light);
                    //generate shadow map
                    renderLight(light, renderProgram);
                }
            };

            renderMethod.name = "All directional shadows";
            renderMethod.glslProgram = glslProgram;
            return renderMethod;


        }

        RenderMethod getRenderMethodAllPointLights(std::shared_ptr<GraphicsProgram> glslProgram) const {
            RenderMethod renderMethod;

            renderMethod.method = [&] (const std::shared_ptr<GraphicsProgram> &renderProgram) {
                std::vector<size_t> lights = getLightIndexes(Light::LightTypes::POINT);
                for (size_t light:lights) {
                    renderLight(light, renderProgram);
                }
            };
            renderMethod.name = "All point shadows";
            renderMethod.glslProgram = glslProgram;
            return renderMethod;
        }

    };
    
    
    explicit GraphicsPipeline(RenderMethods renderMethods) : renderMethods(renderMethods) {}

    const RenderMethods &getRenderMethods() const {
        return renderMethods;
    }


    static const std::vector<std::string> &getRenderMethodNames() {
        return renderMethodNames;
    }

    struct StageInfo {
        std::shared_ptr<GraphicsPipelineStage> stage;
        bool clear = false;
        std::vector<RenderMethod> renderMethods;

        void addRenderMethod(RenderMethod method) {
            renderMethods.emplace_back(method);
        }
    };

    void addNewStage(const StageInfo& stageInformation) {
        pipelineStages.push_back(stageInformation);
    }
    void render();


private:
    RenderMethods renderMethods;
    std::vector<StageInfo> pipelineStages;
};


#endif //LIMONENGINE_GRAPHICSPIPELINE_H
