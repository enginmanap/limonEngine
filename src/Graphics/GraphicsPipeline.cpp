//
// Created by engin on 31.03.2019.
//

#include "GraphicsPipeline.h"
#include "API/Graphics/GraphicsProgram.h"
#include "GraphicsProgramLoader.h"
#include "API/Graphics/RenderMethodInterface.h"

//Static initialize of the vector
std::vector<std::string> GraphicsPipeline::renderMethodNames{"None", "All directional shadows", "All point shadows", "Render Opaque Objects", "Render Animated Objects",
                                                             "Render Transparent Objects", "Render GUI Texts", "Render GUI Images", "Render Editor", "Render Sky",
                                                             "Render Debug Information", "Render Opaque Player Attachment", "Render Animated Player Attachment",
                                                             "Render Transparent Player Attachment", "Render quad"};


void GraphicsPipeline::render() {
    for(auto stageInfo:pipelineStages) {
        stageInfo.stage->activate(stageInfo.clear);
        for(auto renderMethod:stageInfo.renderMethods) {
            renderMethod();
        }
    }
}

bool GraphicsPipeline::serialize(const std::string& renderPipelineFileName, Options *options) {
    /**
    *     to serialize, we need 3 set of data
    *     1) Textures
    *     2) Stages
    *     3) Stage render order
    */
    tinyxml2::XMLDocument document;

    tinyxml2::XMLElement *graphicsPipelineElement = document.NewElement("GraphicsPipeline");
    document.InsertEndChild(graphicsPipelineElement);

    tinyxml2::XMLElement *texturesElement = document.NewElement("Textures");
    uint32_t textureSerializeID = 0;
    for(const std::shared_ptr<Texture>& texture:this->textures) {
        textureSerializeID++;
        if(texture->getSerializeID() == 0 ) {
            texture->setSerializeID(textureSerializeID);
        }
        texture->serialize(document, texturesElement, options);
    }
    graphicsPipelineElement->InsertEndChild(texturesElement);

    tinyxml2::XMLElement *stagesElement = document.NewElement("Stages");
    for(StageInfo stageInfo:this->pipelineStages) {
        stageInfo.serialize(document, stagesElement, options);
    }
    graphicsPipelineElement->InsertEndChild(stagesElement);

    tinyxml2::XMLError eResult = document.SaveFile(renderPipelineFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

bool GraphicsPipeline::StageInfo::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options) {
    /**
     * This method needs to serialize 3 things:
     * 1) methods
     * 2) clear boolean
     * 3) Stage itself
     *
     * Stage serialization is Stage classes responsibility
     */

    tinyxml2::XMLElement *stageInformationElement = document.NewElement("StageInformation");
    parentNode->InsertEndChild(stageInformationElement);

    tinyxml2::XMLElement *methodList = document.NewElement("RenderMethods");
    for (size_t i = 0; i < renderMethods.size(); ++i) {
        tinyxml2::XMLElement *methodElement = document.NewElement("Method");
        tinyxml2::XMLElement *methodNameElement = document.NewElement("Name");
        methodNameElement->SetText(renderMethods[i].getName().c_str());
        methodElement->InsertEndChild(methodNameElement);

        tinyxml2::XMLElement *methodIndexElement = document.NewElement("Index");
        methodIndexElement->SetText(std::to_string(i).c_str());
        methodElement->InsertEndChild(methodIndexElement);
        if(renderMethods[i].getGlslProgram() != nullptr) {
            GraphicsProgramLoader::serialize(document, methodElement, renderMethods[i].getGlslProgram());
        } else {
            methodElement->SetAttribute("ProgramNull", "True");
        }

        //now we need to parse the external methods
        for (auto erIterator = externalRenderMethods.begin(); erIterator != externalRenderMethods.end(); ++erIterator) {
            tinyxml2::XMLElement* externalMethodElement =  document.NewElement("ExternalMethod");
            externalMethodElement->SetText(erIterator->first.c_str());
        }
        methodList->InsertEndChild(methodElement);
    }
    stageInformationElement->InsertEndChild(methodList);

    tinyxml2::XMLElement *clearElement = document.NewElement("Clear");
    if(this->clear) {
        clearElement->SetText("True");
    } else {
        clearElement->SetText("False");
    }
    stageInformationElement->InsertEndChild(clearElement);
    this->stage->serialize(document, stageInformationElement, options);

    return true;
}

std::unique_ptr<GraphicsPipeline>
GraphicsPipeline::deserialize(const std::string &graphicsPipelineFileName, GraphicsInterface *graphicsWrapper,  std::shared_ptr<AssetManager> assetManager, Options *options, RenderMethods renderMethods) {
    /**
*     to serialize, we need 3 set of data
*     1) Textures
*     2) Stages
*     3) Stage render order
*/

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(graphicsPipelineFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading XML " << graphicsPipelineFileName << ": " << xmlDoc.ErrorName() << std::endl;
        exit(-1);
    }

    tinyxml2::XMLNode * graphicsPipelineNode = xmlDoc.FirstChildElement("GraphicsPipeline");
    if (graphicsPipelineNode == nullptr) {
        std::cerr << "Loaded file doesn't have GraphicsPipeline node. cancelling!" << std::endl;
        return nullptr;
    }

    tinyxml2::XMLElement* graphicsPipelineElement = nullptr;

    graphicsPipelineElement = graphicsPipelineNode->FirstChildElement("Textures");
    if (graphicsPipelineElement == nullptr) {
        std::cerr << "GraphicsPipeline must have Textures. Skipping!" << std::endl;
        return nullptr;
    }

    tinyxml2::XMLElement* textureElement =  graphicsPipelineElement->FirstChildElement("Texture");
    if (textureElement == nullptr) {
        std::cout << "Render pipeline doesn't have any textures, this might be a mistake." << std::endl;
    }
    std::unique_ptr<GraphicsPipeline> graphicsPipeline = std::make_unique<GraphicsPipeline>(renderMethods);
    while(textureElement !=nullptr) {
        std::shared_ptr<Texture> texture = Texture::deserialize(textureElement, graphicsWrapper, assetManager, options);
        if(texture != nullptr) {
            graphicsPipeline->addTexture(texture);
        }
        textureElement =  textureElement->NextSiblingElement("Texture");
    }

    graphicsPipelineElement = graphicsPipelineNode->FirstChildElement("Stages");
    if (graphicsPipelineElement == nullptr) {
        std::cerr << "GraphicsPipeline must have Stages. Skipping!" << std::endl;
        return nullptr;
    }

    tinyxml2::XMLElement* stageInfoElement =  graphicsPipelineElement->FirstChildElement("StageInformation");
    if (stageInfoElement == nullptr) {
        std::cerr << "Render pipeline doesn't have any stages, this is definitely a mistake, cancelling!" << std::endl;
        return nullptr;
    }


    while(stageInfoElement !=nullptr) {
        std::shared_ptr<StageInfo> stageInfo = StageInfo::deserialize(stageInfoElement, assetManager, graphicsPipeline, graphicsPipeline->textures, options);
        if(stageInfo != nullptr) {
            graphicsPipeline->addNewStage(*(stageInfo.get()));
        }
        stageInfoElement =  stageInfoElement->NextSiblingElement("StageInformation");
    }

    return graphicsPipeline;
}

std::shared_ptr<GraphicsPipeline::StageInfo>
GraphicsPipeline::StageInfo::deserialize(tinyxml2::XMLElement *stageInfoElement, std::shared_ptr<AssetManager> assetManager, std::unique_ptr <GraphicsPipeline>& pipeline,
                                         const std::vector<std::shared_ptr<Texture>> &textures, Options *options) {

    tinyxml2::XMLElement * clearElement = stageInfoElement->FirstChildElement("Clear");
    bool clear = true;
    if (clearElement == nullptr) {
        std::cerr << "Loaded file doesn't have Clear node. defaulting to true" << std::endl;
    } else {
        if(clearElement->GetText() == nullptr) {
            std::cerr << "Loaded file has Clear node, but it doesn't have text. defaulting to true" << std::endl;
        } else {
            std::string clearString = clearElement->GetText();
            if(clearString == "False") {
                clear = false;
            } else if (clearString != "True") {
                std::cerr << "Clear field has unknown value. defaulting to true" << std::endl;
            }
        }
    }
    std::shared_ptr<GraphicsPipeline::StageInfo> newStageInfo = std::make_shared<GraphicsPipeline::StageInfo>();
    newStageInfo->clear = clear;

    tinyxml2::XMLElement* renderMethodsElement =  stageInfoElement->FirstChildElement("RenderMethods");
    if (renderMethodsElement == nullptr) {
        std::cerr << "StageInfo has no render methods, this is definitely a mistake, cancelling!" << std::endl;
        return nullptr;
    }

    tinyxml2::XMLElement* methodElement =  renderMethodsElement->FirstChildElement("Method");
    if (methodElement == nullptr) { //I know we check this in while, but in while it doesn't hard fail
        std::cerr << "StageInfo has no render methods, this is definitely a mistake, cancelling!" << std::endl;
        return nullptr;
    }

    while(methodElement !=nullptr) {
        tinyxml2::XMLElement* methodNameElement =  methodElement->FirstChildElement("Name");
        if (methodNameElement == nullptr) {
            std::cerr << "StageInfo Method has no name, this is definitely a mistake, cancelling!" << std::endl;
            return nullptr;
        }
        if(methodNameElement->GetText() == nullptr) {
            std::cerr << "Method name has no text, this is a mistake, cancelling!" << std::endl;
            return nullptr;
        }
        std::string methodName = methodNameElement->GetText();

        tinyxml2::XMLElement* methodIndexElement =  methodElement->FirstChildElement("Index");
        if (methodIndexElement == nullptr) {
            std::cerr << "StageInfo Method has no index, this is definitely a mistake, cancelling!" << std::endl;
            return nullptr;
        }
        if(methodIndexElement->GetText() == nullptr) {
            std::cerr << "Method name has no index, this is a mistake, cancelling!" << std::endl;
            return nullptr;
        }

        //This is not the last part because lights require the stage

        tinyxml2::XMLElement* graphicsStageElement =  stageInfoElement->FirstChildElement("GraphicsPipelineStage");
        if (graphicsStageElement == nullptr) {
            std::cerr << "StageInfo has no stage, this is definitely a mistake, cancelling!" << std::endl;
            return nullptr;
        }

        newStageInfo->stage = GraphicsPipelineStage::deserialize(graphicsStageElement, assetManager->getGraphicsWrapper(), textures, options);

        //uint32_t methodIndex = std::stoi(methodIndexElement->GetText()); //this variable is not used

        std::shared_ptr<GraphicsProgram> graphicsProgram;

        //handle external Graphics program case
        if(methodElement->Attribute("ProgramNull") != nullptr && std::string(methodElement->Attribute("ProgramNull")) == "True" ) {
            //no program needed.
            graphicsProgram = nullptr;
        } else {
            tinyxml2::XMLElement* graphicsProgramElement =  methodElement->FirstChildElement("GraphicsProgram");
            if (graphicsProgramElement == nullptr) {
                std::cerr << "StageInfo has no render method, but it is not tagged as such, cancelling!" << std::endl;
                return nullptr;
            }
            graphicsProgram = GraphicsProgramLoader::deserialize(graphicsProgramElement, assetManager);
        }


        bool isFound = true;
        if(methodName == "All directional shadows") {
            std::shared_ptr<Texture> depthMap = newStageInfo->stage->getOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH);
            RenderMethods::RenderMethod method  = pipeline->getRenderMethods().getRenderMethodAllDirectionalLights(newStageInfo->stage, depthMap, graphicsProgram);
            newStageInfo->addRenderMethod(method);
        } else if(methodName == "All point shadows") {
            RenderMethods::RenderMethod method = pipeline->getRenderMethods().getRenderMethodAllPointLights(graphicsProgram);
            newStageInfo->addRenderMethod(method);
        } else {
            RenderMethods::RenderMethod method = pipeline->getRenderMethods().getRenderMethod(methodName, graphicsProgram, isFound);
            if(!isFound) {
                std::cerr << "Render method build failed, please check!" << std::endl;
                return nullptr;
            }
            newStageInfo->addRenderMethod(method);
        }

        //now we need to parse the external methods
        tinyxml2::XMLElement* externalMethodElement =  methodElement->FirstChildElement("ExternalMethod");
        while(externalMethodElement !=nullptr) {
            if(externalMethodElement->GetText() != nullptr ) {
             std::string externalMethodNameString = externalMethodElement->GetText();
                RenderMethodInterface* externalRenderMethod = RenderMethodInterface::createRenderMethod(externalMethodNameString, assetManager->getGraphicsWrapper());
                externalRenderMethod->initRender(graphicsProgram, std::vector<LimonAPI::ParameterRequest>());
                newStageInfo->addExternalRenderMethod(externalMethodNameString, externalRenderMethod);
            }
            externalMethodElement =  externalMethodElement->NextSiblingElement("ExternalMethod");
        }

        if(graphicsProgram != nullptr) {//debug render program is not loaded by the internal systems
            newStageInfo->programs.emplace_back(graphicsProgram);
        }

        methodElement =  methodElement->NextSiblingElement("Method");
    }
    // end of method list parsing

    return newStageInfo;
}

