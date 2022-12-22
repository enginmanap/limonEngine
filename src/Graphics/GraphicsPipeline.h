//
// Created by engin on 31.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINE_H
#define LIMONENGINE_GRAPHICSPIPELINE_H


#include <utility>
#include <vector>
#include <functional>
#include <memory>
#include <Assets/AssetManager.h>

#include "GraphicsPipelineStage.h"
#include "../GameObjects/Light.h"
#include "RenderMethods.h"

class World;
class GraphicsProgram;
class RenderMethodInterface;

class GraphicsPipeline {
    friend class SDL2Helper;
    GraphicsPipeline() = default;//used for deserialize

public:
    static std::vector<std::string> renderMethodNames;//This is not array, because custom effects might be loaded on runtime as extensions.

    explicit GraphicsPipeline(RenderMethods renderMethods) : renderMethods(std::move(renderMethods)) {
        auto tempNameVector = RenderMethodInterface::getRenderMethodNames();
        renderMethodNames.insert(renderMethodNames.end(),
                                 tempNameVector.begin(),
                                 tempNameVector.end());
    }

    const RenderMethods &getRenderMethods() const {
        return renderMethods;
    }


    static const std::vector<std::string> &getRenderMethodNames() {
        return renderMethodNames;
    }

    struct StageInfo {
        int highestPriority = 999;//keeps the highest priority, used for render ordering, which is lower. priority 1 is higher priority then priority 10.
        std::shared_ptr<GraphicsPipelineStage> stage;
        bool clear = false;
        std::vector<RenderMethods::RenderMethod> renderMethods;
        std::unordered_map<std::string, RenderMethodInterface*> externalRenderMethods;

        void addRenderMethod(RenderMethods::RenderMethod method) {
            for (auto iterator = renderMethods.begin(); iterator != renderMethods.end();++iterator) {
                if(iterator->getPriority() > method.getPriority()) {
                    renderMethods.insert(iterator, method);
                    return;
                }
            }
            highestPriority = method.getPriority();
            renderMethods.emplace_back(method);
        }

        void addExternalRenderMethod(const std::string& methodName, RenderMethodInterface* externalMethod) {
            externalRenderMethods[methodName] = externalMethod;
        }

        bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);
        static bool
        deserialize(tinyxml2::XMLElement *stageInfoElement,
                    std::shared_ptr<AssetManager> assetManager,
                    std::unique_ptr<GraphicsPipeline> &pipeline,
                    const std::vector<std::shared_ptr<Texture>> &textures,
                    GraphicsPipeline::StageInfo &newStageInfo);
        std::vector<std::shared_ptr<GraphicsProgram>> programs;
    };

    void addNewStage(const StageInfo& stageInformation) {
        //we need to sort based on priority
        for (auto stageInfo = pipelineStages.rbegin(); stageInfo != pipelineStages.rend(); ++stageInfo ) {
            if(stageInfo->highestPriority <= stageInformation.highestPriority) {
                pipelineStages.insert(stageInfo.base(), stageInformation);//oddly, reverse iterator base is pointing to after the iterator
                return;
            }
        }
        pipelineStages.insert(pipelineStages.begin(),stageInformation);//if not found, it means that should be inserted to beginning
    }

    void addTexture(const std::shared_ptr<Texture>& texture) {
        textures.emplace_back(texture);
    }

    std::vector<StageInfo>& getStages() {
        return pipelineStages;
    };

    std::shared_ptr<Texture> getTexture(uint32_t serializeID) {
        for(std::shared_ptr<Texture> texture:this->textures) {
            if(texture->getSerializeID() == serializeID) {
                return texture;
            }
        }
        return nullptr;
    }

    void initialize();

    inline void render() {
        for(auto& stageInfo:pipelineStages) {
            stageInfo.stage->activate(stageInfo.clear);
            for(auto& renderMethod:stageInfo.renderMethods) {
                renderMethod();
            }
        }
    }

    void finalize();

    bool serialize(const std::string& renderPipelineFileName, Options *options);

    static std::unique_ptr<GraphicsPipeline> deserialize(const std::string &graphicsPipelineFileName, GraphicsInterface *graphicsWrapper,  std::shared_ptr<AssetManager>, Options *options, RenderMethods renderMethods);

    const std::vector<std::shared_ptr<Texture>> &getTextures() {
        return textures;
    };
private:
    RenderMethods renderMethods;
    std::vector<StageInfo> pipelineStages;
    std::vector<std::shared_ptr<Texture>> textures;
};

#endif //LIMONENGINE_GRAPHICSPIPELINE_H
