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
class GLSLProgram;

class GraphicsPipeline {
public:
    static std::vector<std::string> renderMethodNames;//This is not array, because custom effects might be loaded on runtime as extensions.

    struct RenderMethods {
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderOpaqueObjects;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderAnimatedObjects;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderTransparentObjects;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderGUITexts;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderGUIImages;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderPlayerAttachmentOpaque;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderPlayerAttachmentTransparent;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderPlayerAttachmentAnimated;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderSky;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderEditor;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderDebug;

        //These methods are not exposed to the interface
        //They are also not possible to add to render pipeline, so a method should be created and assigned.
        std::function<std::vector<size_t>(Light::LightTypes)> getLightsByType;
        std::function<void(unsigned int, std::shared_ptr<GLSLProgram>)> renderLight;

    };
    
    
    explicit GraphicsPipeline(RenderMethods renderMethods) : renderMethods(renderMethods) {
        
    }

    std::function<void(const std::shared_ptr<GLSLProgram>&)> getRenderMethodByName(const std::string& name) {
        if(name == "None") {
            std::cerr << "Building graphics pipeline with empty method, are you sure that was set correctly?" << std::endl;
            return [](const std::shared_ptr<GLSLProgram>& notUsed[[gnu::unused]]){};
        } else if(name == "Render Opaque Objects") {
            return  renderMethods.renderOpaqueObjects;
        } else if(name == "Render Animated Objects") {
            return renderMethods.renderAnimatedObjects;
        } else if(name == "Render Transparent Objects") {
            return renderMethods.renderTransparentObjects;
        } else if(name == "Render GUI Texts") {
            return renderMethods.renderGUITexts;
        } else if(name == "Render GUI Images") {
            return renderMethods.renderGUIImages;
        } else if(name == "Render Editor") {
            return renderMethods.renderEditor;
        } else if(name == "Render sky") {
            return renderMethods.renderSky;
        } else if(name == "Render Debug Information") {
            return renderMethods.renderDebug;
        } else if(name == "Render Opaque Player Attachment") {
            return renderMethods.renderPlayerAttachmentOpaque;
        } else if(name == "Render Transparent Player Attachment") {
            return renderMethods.renderPlayerAttachmentTransparent;
        } else if(name == "Render Animated Player Attachment") {
            return renderMethods.renderPlayerAttachmentAnimated;
        }
        std::cerr << "Given name for render method is unknown! It will not render anything!" << std::endl;
        return nullptr;

    }

    std::vector<size_t> getLightIndexes(Light::LightTypes lightType) {
        return renderMethods.getLightsByType(lightType);
    }

    std::function<void(unsigned int, std::shared_ptr<GLSLProgram>)>& getRenderLightMethod() {
        return this->renderMethods.renderLight;
    }

    static const std::vector<std::string> &getRenderMethodNames() {
        return renderMethodNames;
    }

    struct StageInfo {
        std::shared_ptr<GraphicsPipelineStage> stage;
        std::map<std::shared_ptr<Texture>, std::pair<GLHelper::FrameBufferAttachPoints, int>> attachmentLayerMap;
        std::vector<std::pair<std::function<void(const std::shared_ptr<GLSLProgram>&)>, std::shared_ptr<GLSLProgram>>> renderMethods;
        bool clear = false;
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
