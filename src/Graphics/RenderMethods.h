//
// Created by engin on 25.10.2020.
//

#ifndef LIMONENGINE_RENDERMETHODS_H
#define LIMONENGINE_RENDERMETHODS_H


#include <string>
#include <functional>
#include <utility>
#include <API/Graphics/GraphicsProgram.h>
#include "../GameObjects/Light.h"
#include "GraphicsPipelineStage.h"


class RenderMethods {
    friend class World;//so it can fill it up.
public:
    class RenderMethod {
        std::string name;
        std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::vector<LimonAPI::ParameterRequest>&)> initializer;
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> method;
        std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::vector<LimonAPI::ParameterRequest>&)> finalizer;
        std::shared_ptr<GraphicsProgram> glslProgram;
        bool isInitialized = false;

    public:
        RenderMethod(std::string name,
                     std::function<void(const std::shared_ptr<GraphicsProgram> &, const std::vector<LimonAPI::ParameterRequest> &)> initializer,
                     std::function<void(const std::shared_ptr<GraphicsProgram> &)> method,
                     std::function<void(const std::shared_ptr<GraphicsProgram> &, const std::vector<LimonAPI::ParameterRequest> &)> finalizer,
                     std::shared_ptr<GraphicsProgram> glslProgram) :
                     name(std::move(name)), initializer(std::move(initializer)), method(std::move(method)), finalizer(std::move(finalizer)),
                     glslProgram(std::move(glslProgram)) {}

    public:
        void operator()() {
#ifndef NDEBUG
        //only check on debug builds
            if(!isInitialized) {
                if(initializer == nullptr) {
                    isInitialized = true;
                } else {
                    std::cerr << "RenderMethod " << name << " is not initialized, skipping." << std::endl;
                }
            }
#endif
            method(glslProgram);
        }

        void initialize(const std::vector<LimonAPI::ParameterRequest>& parameters) {
            if(this->initializer != nullptr) {
                initializer(glslProgram, parameters);
            }
            isInitialized = true;
        }

        void finalize(const std::vector<LimonAPI::ParameterRequest>& parameters) {
            if(this->finalizer != nullptr) {
                finalizer(glslProgram, parameters);
            }
            isInitialized = false;
        }

        const std::string &getName() const {
            return name;
        }

        const std::shared_ptr<GraphicsProgram> &getGlslProgram() const {
            return glslProgram;
        }
    };
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

    /**
     * Always returns a method, even if not found. If not found, the parameter is set to false.
     * @param name      Method name to search for
     * @param found     Is the method found
     * @return          The given method, or noop method
     */
    std::function<void(const std::shared_ptr<GraphicsProgram>&)> getRenderMethodByName(const std::string& name, bool& found) const {
        found  = true;
        if(name == "Render Opaque Objects") {
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
        } else if(name == "Render Sky") {
            return renderSky;
        } else if(name == "Render Debug Information") {
            return renderDebug;
        } else if(name == "Render Opaque Player Attachment") {
            return renderPlayerAttachmentOpaque;
        } else if(name == "Render Animated Player Attachment") {
            return renderPlayerAttachmentAnimated;
        } else if(name == "Render Transparent Player Attachment") {
            return renderPlayerAttachmentTransparent;
        } else if(name == "Render quad") {
            return renderQuad;
        } else if(name == "None") {
            std::cerr << "Building graphics pipeline with empty method, are you sure that was set correctly?" << std::endl;
        } else {
            found = false;
        }
        return [](const std::shared_ptr<GraphicsProgram>& notUsed[[gnu::unused]]){};//this is returned for both None and not found
    }

    std::vector<size_t> getLightIndexes(Light::LightTypes lightType) const {
        return getLightsByType(lightType);
    }

    std::function<void(unsigned int, std::shared_ptr<GraphicsProgram>)>& getRenderLightMethod() {
        return renderLight;
    }

public:
    RenderMethod getRenderMethod(const std::string& methodName, const std::shared_ptr<GraphicsProgram>& glslProgram, bool& isFound) const {
        std::function<void(const std::shared_ptr<GraphicsProgram>&)> method = getRenderMethodByName(methodName, isFound);
        if(!isFound) {
            return RenderMethod("NotFound", nullptr, method, nullptr, glslProgram);
        }
        return RenderMethod(methodName, nullptr, method, nullptr, glslProgram);
    }

    RenderMethod getRenderMethodAllDirectionalLights(std::shared_ptr<GraphicsPipelineStage>& stage, std::shared_ptr<Texture>& layeredDepthMap, const std::shared_ptr<GraphicsProgram>& glslProgram) const {
        return RenderMethod("All directional shadows",
                            nullptr,
                            [=](const std::shared_ptr<GraphicsProgram> &renderProgram) {
                                std::vector<size_t> lights = getLightIndexes(Light::LightTypes::DIRECTIONAL);
                                for (size_t light:lights) {
                                    //set the layer that will be rendered. Also set clear so attached layer will be cleared right away.
                                    //this is important because they will not be cleared other way.
                                    stage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, layeredDepthMap, true, light);
                                    //generate shadow map
                                    renderLight(light, renderProgram);
                                }
                            },
                            nullptr,
                            glslProgram
        );
    }

    RenderMethod getRenderMethodAllPointLights(const std::shared_ptr<GraphicsProgram>& glslProgram) const {
        return RenderMethod("All point shadows",
                            nullptr,
                            [&] (const std::shared_ptr<GraphicsProgram> &renderProgram) {
                                std::vector<size_t> lights = getLightIndexes(Light::LightTypes::POINT);
                                for (size_t light:lights) {
                                    renderLight(light, renderProgram);
                                }
                            },
                            nullptr,
                            glslProgram);
    }

};

#endif //LIMONENGINE_RENDERMETHODS_H
