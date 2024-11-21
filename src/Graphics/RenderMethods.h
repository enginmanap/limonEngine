//
// Created by engin on 25.10.2020.
//

#ifndef LIMONENGINE_RENDERMETHODS_H
#define LIMONENGINE_RENDERMETHODS_H


#include <string>
#include <functional>
#include <utility>
#include <API/Graphics/GraphicsProgram.h>
#include <API/Graphics/RenderMethodInterface.h>
#include "../GameObjects/Light.h"
#include "GraphicsPipelineStage.h"


class RenderMethods {
    friend class World;//so it can fill it up.
public:
    class RenderMethod {
        std::string name;
        std::string cameraName;//FIXME I am not sure if we want multiple tags in single call or single, needs decision.
        std::vector<HashUtil::HashedString> renderTags;
        std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::vector<LimonTypes::GenericParameter>&)> initializer;
        std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]])> method;
        std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::vector<LimonTypes::GenericParameter>&)> finalizer;
        std::shared_ptr<GraphicsProgram> glslProgram;
        uint32_t priority{};
        bool isInitialized = false;

    public:
        RenderMethod(std::string  name,
                     uint32_t priority,
                     std::function<void(const std::shared_ptr<GraphicsProgram> &, const std::vector<LimonTypes::GenericParameter> &)> initializer,
                     std::function<void(const std::shared_ptr<GraphicsProgram> &, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]])> method,
                     std::function<void(const std::shared_ptr<GraphicsProgram> &, const std::vector<LimonTypes::GenericParameter> &)> finalizer,
                     std::shared_ptr<GraphicsProgram> glslProgram) :
                     name(std::move(name)), initializer(std::move(initializer)), method(std::move(method)), finalizer(std::move(finalizer)),
                     glslProgram(std::move(glslProgram)), priority(priority) {
            if(!this->initializer) {
                isInitialized = true;
            }
        }

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
            method(glslProgram, cameraName, renderTags);
        }

        void initialize(const std::vector<LimonTypes::GenericParameter>& parameters) {
            if(this->initializer != nullptr) {
                initializer(glslProgram, parameters);
            }
            isInitialized = true;
        }

        void finalize(const std::vector<LimonTypes::GenericParameter>& parameters) {
            if(this->finalizer != nullptr) {
                finalizer(glslProgram, parameters);
            }
            isInitialized = false;
        }

        const std::string &getName() const {
            return name;
        }

        uint32_t getPriority() const {
            return priority;
        }

        const std::shared_ptr<GraphicsProgram> &getGlslProgram() const {
            return glslProgram;
        }

        bool getInitialized() const {
            return isInitialized;
        }

        const std::string &getCameraName() const {
            return cameraName;
        }

        void setCameraName(const std::string &cameraName) {
            RenderMethod::cameraName = cameraName;
        }

        const std::vector<HashUtil::HashedString> &getRenderTags() const {
            return renderTags;
        }

        void setRenderTags(const std::vector<HashUtil::HashedString> &renderTags) {
            this->renderTags.clear();
            for (const auto &item: renderTags) {
                this->renderTags.emplace_back(item);
            }
        }
    };
private:
    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderParticleEmitters;
    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderGPUParticleEmitters;
    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderGUITexts;
    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderGUIImages;
    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderSky;
    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderEditor;
    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderDebug;

    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderQuad;//For offscreen stuff

    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string&, const std::vector<HashUtil::HashedString> &)> renderCameraByTag;//For 3d rendering

    mutable std::unordered_map<std::string, RenderMethodInterface*> dynamicRenderMethodInstances;// Not allowing more than one instance for now, used like a cache so mutable
    //These methods are not exposed to the interface
    //They are also not possible to add to render pipeline, so a method should be created and assigned.
    std::function<std::vector<size_t>(Light::LightTypes)> getLightsByType;
    std::function<void(unsigned int, unsigned int, std::shared_ptr<GraphicsProgram>)> renderLight;

    /**
     * Always returns a method, even if not found. If not found, the parameter is set to false.
     * @param name      Method name to search for
     * @param found     Is the method found
     * @return          The given method, or noop method
     */
    std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string &cameraName, const std::vector<HashUtil::HashedString> &)> getRenderMethodByName(const std::string& name, bool& found, uint32_t& priority) const {
        found  = true;
        if(name == "Render Tagged Objects") {
            priority = 2;
            return renderCameraByTag;
        } else if(name == "Render Particle Emitters") {
            priority = 13;
            return renderParticleEmitters;
        } else if(name == "Render GPU Particle Emitters") {
            priority = 14;
            return renderGPUParticleEmitters;
        } else if(name == "Render GUI Texts") {
            priority = 21;
            return renderGUITexts;
        } else if(name == "Render GUI Images") {
            priority = 22;
            return renderGUIImages;
        } else if(name == "Render Editor") {
            priority = 23;
            return renderEditor;
        } else if(name == "Render Sky") {
            priority = 20;
            return renderSky;
        } else if(name == "Render Debug Information") {
            priority = 24;
            return renderDebug;
        } else if(name == "Render quad") {
            priority = 10;
            return renderQuad;
        } else if(name == "None") {
            priority = 0;
            std::cerr << "Building graphics pipeline with empty method, are you sure that was set correctly?" << std::endl;
        } else {
            found = false;
        }
        return [](const std::shared_ptr<GraphicsProgram>& notUsed[[gnu::unused]], const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]){};//this is returned for both None and not found
    }

    std::vector<size_t> getLightIndexes(Light::LightTypes lightType) const {
        return getLightsByType(lightType);
    }

    std::function<void(unsigned int, unsigned int, std::shared_ptr<GraphicsProgram>)>& getRenderLightMethod() {
        return renderLight;
    }

    RenderMethod getBuiltInRenderMethod(const std::string& methodName, const std::shared_ptr<GraphicsProgram>& glslProgram, bool& isFound) const {
        uint32_t priority = 0;
        std::function<void(const std::shared_ptr<GraphicsProgram>&, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]])> method = getRenderMethodByName(methodName, isFound, priority);
        if(!isFound) {
            return RenderMethod("NotFound", priority, nullptr, method, nullptr, glslProgram);
        }
        return RenderMethod(methodName, priority, nullptr, method, nullptr, glslProgram);
    }

public:
    RenderMethod getRenderMethod(GraphicsInterface* graphicsInterface, const std::string& methodName, const std::shared_ptr<GraphicsProgram>& glslProgram, bool& isFound) const {
        uint32_t priority = 999;
        //First check if we already created an instance
        if(dynamicRenderMethodInstances.find(methodName) == dynamicRenderMethodInstances.end()) {
            //create an instance for usage
            auto dynamicRenderMethodNames = RenderMethodInterface::getRenderMethodNames();
            for (const std::string& dynamicMethodName:dynamicRenderMethodNames) {
                if(dynamicMethodName == methodName) {
                    RenderMethodInterface * methodInterface = RenderMethodInterface::createRenderMethodInterfaceInstance(methodName, graphicsInterface);
                    dynamicRenderMethodInstances[methodName] = methodInterface;
                    break;
                }
            }
        }
        if(dynamicRenderMethodInstances.find(methodName) != dynamicRenderMethodInstances.end()) {
            //we built it at some point
            RenderMethodInterface * methodInterface = dynamicRenderMethodInstances[methodName];
            if(methodInterface != nullptr) {
                return RenderMethod(methodName,
                                    priority,
                                    [methodInterface](const std::shared_ptr<GraphicsProgram>& program, const std::vector<LimonTypes::GenericParameter> & params)
                                    {return methodInterface->initRender(program, params);},
                                    [methodInterface](const std::shared_ptr<GraphicsProgram>& program, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]])
                                    {return methodInterface->renderFrame(program);},
                                    [methodInterface](const std::shared_ptr<GraphicsProgram>& program, const std::vector<LimonTypes::GenericParameter> &params)
                                    {return methodInterface->cleanupRender(program, params);},
                                    glslProgram
                );
            } else {
                std::cerr << "Dynamic render method found but instance creation failed, check API doc for proper register steps." << std::endl;
            }
        }

        //if we hit here, no dynamic match found, search for built in
        return getBuiltInRenderMethod(methodName, glslProgram, isFound);
    }

    RenderMethod getRenderMethodAllDirectionalLights(std::shared_ptr<GraphicsPipelineStage> &stage, std::shared_ptr<Texture> &layeredDepthMap,
                                                     const std::shared_ptr<GraphicsProgram> &glslProgram,
                                                     OptionsUtil::Options *options) const {

        OptionsUtil::Options::Option<long> optionNewSet = options->getOption<long>(HASH("CascadeCount"));
        OptionsUtil::Options::Option<double> option2 = options->getOption<double>(HASH("lookAroundSpeed"));
        return RenderMethod("All directional shadows",
                            1,
                            nullptr,
                            [=](const std::shared_ptr<GraphicsProgram> &renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) {
                                long cascadeCount = optionNewSet.get();

                                std::vector<size_t> lights = getLightIndexes(Light::LightTypes::DIRECTIONAL);
                                if(lights.size() > 1) {
                                    std::cerr << "only one directional light is supported since CSM, this will not work correctly" << std::endl;
                                }
                                if(lights.empty() ) {
                                    return;
                                }
                                for (int i = 0; i < cascadeCount; ++i) {
                                        stage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, layeredDepthMap, true, i);
                                }
                                size_t lightId = lights[0];
                                for (int i = 0; i < cascadeCount; ++i) {
                                    stage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, layeredDepthMap, false, i);
                                    renderLight(lightId, i, renderProgram);
                                }

                            },
                            nullptr,
                            glslProgram
        );
    }

    RenderMethod getRenderMethodAllPointLights(const std::shared_ptr<GraphicsProgram>& glslProgram) const {
        return RenderMethod("All point shadows",
                            1,
                            nullptr,
                            [&] (const std::shared_ptr<GraphicsProgram> &renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) {
                                std::vector<size_t> lights = getLightIndexes(Light::LightTypes::POINT);
                                for (size_t light:lights) {
                                    renderLight(light, 0, renderProgram);
                                }
                            },
                            nullptr,
                            glslProgram);
    }


};

#endif //LIMONENGINE_RENDERMETHODS_H
