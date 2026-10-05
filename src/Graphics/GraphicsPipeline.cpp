//
// Created by engin on 31.03.2019.
//

#include "GraphicsPipeline.h"
#include "limonAPI/Graphics/GraphicsProgram.h"
#include "GraphicsProgramLoader.h"
#include "limonAPI/Graphics/RenderMethodInterface.h"
#include "Utils/StringUtils.hpp"
#include "../Profiler/RenderProfileScope.h"
#include "../Profiler/ProfilerMacros.h"
#include "../GamePlay/APISerializer.h"
#include "../Material.h"
#include <algorithm>

//Static initialize of the vector
std::vector<std::string> GraphicsPipeline::renderMethodNames{"None", "All directional shadows", "All point shadows", "Render Tagged Objects", "Render Opaque Objects",
                                                             "Render Animated Objects", "Render Transparent Objects", "Render GUI Texts", "Render GUI Images", "Render Editor",
                                                             "Render Sky", "Render Debug Information", "Render Particle Emitters", "Render GPU Particle Emitters",
                                                             "Render Opaque Player Attachment", "Render Animated Player Attachment","Render Transparent Player Attachment",
                                                             "Render quad"};

void GraphicsPipeline::initialize() {
    for(auto& stageInfo:pipelineStages) {
        stageInfo.stage->activate(stageInfo.clear);
        for(auto& renderMethod:stageInfo.renderMethods) {
            renderMethod.initialize();
        }
    }
}

void GraphicsPipeline::render() {
    for(auto& stageInfo:pipelineStages) {
        lastStageInfo = &stageInfo;
        RenderProfileScope stageScope(graphicsWrapper, *stageInfo.stage, stageInfo.clear);
        {
            PROFILE_RENDERING("GraphicsPipeline::activateStage");//framebuffer switch and clear, apart from what the methods draw
            stageInfo.stage->activate(stageInfo.clear);
        }
        for(auto& renderMethod:stageInfo.renderMethods) {
            RenderProfileScope methodScope(graphicsWrapper, renderMethod.getName(), renderMethod.getGlslProgram().get(),
                                           renderMethod.getCameraName(), renderMethod.getRenderTags());
            renderMethod();
        }
    }
}

void GraphicsPipeline::finalize() {
    for(auto& stageInfo:pipelineStages) {
        stageInfo.stage->activate(stageInfo.clear);
        for(auto& renderMethod:stageInfo.renderMethods) {
            renderMethod.finalize();
        }
    }
}

void GraphicsPipeline::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentElement, OptionsUtil::Options *options) {
    /**
    *     to serialize, we need 3 set of data
    *     1) Textures
    *     2) Stages
    *     3) Stage render order
    */
    tinyxml2::XMLElement *graphicsPipelineElement = document.NewElement("GraphicsPipeline");
    parentElement->InsertEndChild(graphicsPipelineElement);

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
}

bool GraphicsPipeline::StageInfo::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, OptionsUtil::Options *options) {
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

        //serialize the parameter values so they can be fed to the method at load/create time
        const std::vector<LimonTypes::GenericParameter> &methodParameters = renderMethods[i].getParameters();
        tinyxml2::XMLElement *parametersElement = document.NewElement("Parameters");
        for (size_t parameterIndex = 0; parameterIndex < methodParameters.size(); ++parameterIndex) {
            APISerializer::serializeParameterRequest(methodParameters[parameterIndex], document, parametersElement, parameterIndex);
        }
        methodElement->InsertEndChild(parametersElement);

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

static std::unique_ptr<GraphicsPipeline> rejectPipeline(std::vector<std::string> &errors, const std::string &graphicsPipelineFileName, const std::string &message) {
    std::cerr << graphicsPipelineFileName << ": " << message << std::endl;
    errors.emplace_back(graphicsPipelineFileName + ": " + message);
    return nullptr;
}

static bool failStageInfoLoad(std::vector<std::string> &errors, const std::string &message) {
    std::cerr << message << std::endl;
    errors.emplace_back(message);
    return false;
}

std::unique_ptr<GraphicsPipeline>
GraphicsPipeline::deserialize(const std::string &graphicsPipelineFileName, GraphicsInterface *graphicsWrapper,  std::shared_ptr<AssetManager> assetManager, OptionsUtil::Options *options, RenderMethods renderMethods,
                              std::vector<std::string> &errors) {
    /**
*     to serialize, we need 3 set of data
*     1) Textures
*     2) Stages
*     3) Stage render order
*/

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(graphicsPipelineFileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        return rejectPipeline(errors, graphicsPipelineFileName, std::string("Error loading XML: ") + xmlDoc.ErrorName());
    }

    tinyxml2::XMLElement * renderPipelineElement = xmlDoc.FirstChildElement("RenderPipeline");
    if (renderPipelineElement == nullptr) {
        return rejectPipeline(errors, graphicsPipelineFileName, "is not a render pipeline file. Files saved before the node graph and pipeline were kept together must be loaded in the node editor and saved again.");
    }
    tinyxml2::XMLNode * graphicsPipelineNode = renderPipelineElement->FirstChildElement("GraphicsPipeline");
    if (graphicsPipelineNode == nullptr) {
        return rejectPipeline(errors, graphicsPipelineFileName, "has no built pipeline, its node graph didn't build when it was saved. Fix it in the node editor and save again.");
    }

    tinyxml2::XMLElement* graphicsPipelineElement = nullptr;

    graphicsPipelineElement = graphicsPipelineNode->FirstChildElement("Textures");
    if (graphicsPipelineElement == nullptr) {
        return rejectPipeline(errors, graphicsPipelineFileName, "GraphicsPipeline must have Textures.");
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
        return rejectPipeline(errors, graphicsPipelineFileName, "GraphicsPipeline must have Stages.");
    }

    tinyxml2::XMLElement* stageInfoElement =  graphicsPipelineElement->FirstChildElement("StageInformation");
    if (stageInfoElement == nullptr) {
        return rejectPipeline(errors, graphicsPipelineFileName, "Render pipeline doesn't have any stages.");
    }

    uint32_t stageIndex = 0;
    while(stageInfoElement !=nullptr) {
        StageInfo stageInfo;
        if(!StageInfo::deserialize(stageInfoElement, assetManager, graphicsPipeline,
                                  graphicsPipeline->textures,
                                  stageInfo, errors)) {
            return rejectPipeline(errors, graphicsPipelineFileName, "stage " + std::to_string(stageIndex) + " can't be loaded, the pipeline is not used.");
        }
        graphicsPipeline->addNewStage(std::move(stageInfo));
        stageInfoElement =  stageInfoElement->NextSiblingElement("StageInformation");
        ++stageIndex;
    }

    std::string assignmentError;
    if (!graphicsPipeline->assignTextureUnits(graphicsWrapper, assignmentError)) {
        return rejectPipeline(errors, graphicsPipelineFileName, assignmentError);
    }
    graphicsPipeline->printTextureUnits(graphicsWrapper, graphicsPipelineFileName);

    graphicsPipeline->setGraphicsWrapper(graphicsWrapper);
    graphicsPipeline->initialize();
    return graphicsPipeline;
}

bool
GraphicsPipeline::StageInfo::deserialize(tinyxml2::XMLElement *stageInfoElement,
                                         std::shared_ptr<AssetManager> assetManager,
                                         std::unique_ptr<GraphicsPipeline> &pipeline,
                                         const std::vector<std::shared_ptr<Texture>> &textures,
                                         GraphicsPipeline::StageInfo &newStageInfo,
                                         std::vector<std::string> &errors) {

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

    newStageInfo.clear = clear;

    tinyxml2::XMLElement* renderMethodsElement =  stageInfoElement->FirstChildElement("RenderMethods");
    if (renderMethodsElement == nullptr) {
        return failStageInfoLoad(errors, "StageInfo has no render methods, this is definitely a mistake, cancelling!");
    }

    tinyxml2::XMLElement* methodElement =  renderMethodsElement->FirstChildElement("Method");
    if (methodElement == nullptr) { //I know we check this in while, but in while it doesn't hard fail
        return failStageInfoLoad(errors, "StageInfo has no render methods, this is definitely a mistake, cancelling!");
    }

    // Stage is shared by every Method under the stage. We should de-serialize once, instead of with each method
    tinyxml2::XMLElement* graphicsStageElement =  stageInfoElement->FirstChildElement("GraphicsPipelineStage");
    if (graphicsStageElement == nullptr) {
        return failStageInfoLoad(errors, "StageInfo has no stage, this is definitely a mistake, cancelling!");
    }

    std::map<uint32_t, std::shared_ptr<Texture>> legacyInputs;
    std::map<std::string, uint32_t> legacySamplerUnits;
    newStageInfo.stage = GraphicsPipelineStage::deserialize(graphicsStageElement, assetManager->getGraphicsWrapper(), textures, legacyInputs, errors);
    if (newStageInfo.stage == nullptr) {
        return failStageInfoLoad(errors, "StageInfo's GraphicsPipelineStage failed to deserialize, cancelling!");
    }

    newStageInfo.renderTags = newStageInfo.stage->getObjectTags();
    std::vector<HashUtil::HashedString> hashedRenderTags;
    for (const auto &item: newStageInfo.renderTags) {
        hashedRenderTags.emplace_back(item);
    }
    newStageInfo.cameraTags = newStageInfo.stage->getCameraTags();

    while(methodElement !=nullptr) {
        tinyxml2::XMLElement* methodNameElement =  methodElement->FirstChildElement("Name");
        if (methodNameElement == nullptr) {
            return failStageInfoLoad(errors, "StageInfo Method has no name, this is definitely a mistake, cancelling!");
        }
        if(methodNameElement->GetText() == nullptr) {
            return failStageInfoLoad(errors, "Method name has no text, this is a mistake, cancelling!");
        }
        std::string methodName = methodNameElement->GetText();

        tinyxml2::XMLElement* methodIndexElement =  methodElement->FirstChildElement("Index");
        if (methodIndexElement == nullptr) {
            return failStageInfoLoad(errors, "StageInfo Method has no index, this is definitely a mistake, cancelling!");
        }
        if(methodIndexElement->GetText() == nullptr) {
            return failStageInfoLoad(errors, "Method name has no index, this is a mistake, cancelling!");
        }

        //uint32_t methodIndex = std::stoi(methodIndexElement->GetText()); //this variable is not used

        std::shared_ptr<GraphicsProgram> graphicsProgram;

        //handle external Graphics program case
        if(methodElement->Attribute("ProgramNull") != nullptr && std::string(methodElement->Attribute("ProgramNull")) == "True" ) {
            //no program needed.
            graphicsProgram = nullptr;
        } else {
            tinyxml2::XMLElement* graphicsProgramElement =  methodElement->FirstChildElement("GraphicsProgram");
            if (graphicsProgramElement == nullptr) {
                return failStageInfoLoad(errors, "StageInfo has no render method, but it is not tagged as such, cancelling!");
            }
            std::map<std::string, uint32_t> programLegacySamplerUnits;
            graphicsProgram = GraphicsProgramLoader::deserialize(graphicsProgramElement, assetManager, programLegacySamplerUnits);
            for (const std::pair<const std::string, uint32_t>& samplerUnit : programLegacySamplerUnits) {
                std::map<std::string, uint32_t>::iterator existingSampler = legacySamplerUnits.find(samplerUnit.first);
                if (existingSampler != legacySamplerUnits.end() && existingSampler->second != samplerUnit.second) {
                    return failStageInfoLoad(errors, "Two programs of one stage put " + samplerUnit.first + " on different units, it can't be converted to a named input.");
                }
                legacySamplerUnits[samplerUnit.first] = samplerUnit.second;
            }
        }


        //read the saved parameter values so they can be fed to the method at load/create time
        std::vector<LimonTypes::GenericParameter> methodParameters;
        tinyxml2::XMLElement* parametersElement = methodElement->FirstChildElement("Parameters");
        if(parametersElement != nullptr) {
            tinyxml2::XMLElement* parameterElement = parametersElement->FirstChildElement("Parameter");
            while(parameterElement != nullptr) {
                uint32_t parameterIndex = 0;
                std::shared_ptr<LimonTypes::GenericParameter> request = APISerializer::deserializeParameterRequest(parameterElement, parameterIndex);
                if(request != nullptr) {
                    if(parameterIndex >= methodParameters.size()) {
                        methodParameters.resize(parameterIndex + 1);
                    }
                    methodParameters[parameterIndex] = *request;
                }
                parameterElement = parameterElement->NextSiblingElement("Parameter");
            }
        }

        bool isFound = true;
        if(methodName == "All directional shadows") {
            std::shared_ptr<Texture> depthMap = newStageInfo.stage->getOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH);
            RenderMethods::RenderMethod method  = pipeline->getRenderMethods().getRenderMethodAllDirectionalLights(newStageInfo.stage, depthMap, graphicsProgram, assetManager->getGraphicsWrapper()->getOptions());
            method.setRenderTags(hashedRenderTags);
            method.setCameraName(StringUtils::join(newStageInfo.cameraTags, ","));
            method.setParameters(methodParameters);
            newStageInfo.addRenderMethod(method);
        } else if(methodName == "All point shadows") {
            RenderMethods::RenderMethod method = pipeline->getRenderMethods().getRenderMethodAllPointLights(graphicsProgram);
            method.setRenderTags(hashedRenderTags);
            method.setCameraName(StringUtils::join(newStageInfo.cameraTags, ","));
            method.setParameters(methodParameters);
            newStageInfo.addRenderMethod(method);
        } else {
            RenderMethods::RenderMethod method = pipeline->getRenderMethods().getRenderMethod(assetManager->getGraphicsWrapper(), methodName,
                                                                                                     graphicsProgram,
                                                                                                     isFound);
            method.setRenderTags(hashedRenderTags);
            method.setCameraName(StringUtils::join(newStageInfo.cameraTags, ","));
            method.setParameters(methodParameters);
            newStageInfo.addRenderMethod(method);
            if(!isFound) {
                return failStageInfoLoad(errors, "Render method '" + methodName + "' not found. If this is a dynamic method, ensure the plugin DLL is loaded before the pipeline deserializes.");
            }
        }

        if(graphicsProgram != nullptr) {//debug render program is not loaded by the internal systems
            newStageInfo.programs.emplace_back(graphicsProgram);
        }

        methodElement =  methodElement->NextSiblingElement("Method");
    }
    // end of method list parsing

    //old files only link a sampler to its texture through the unit number they both carry
    std::set<uint32_t> matchedLegacyUnits;
    for (const std::pair<const std::string, uint32_t>& samplerUnit : legacySamplerUnits) {
        std::map<uint32_t, std::shared_ptr<Texture>>::iterator legacyInput = legacyInputs.find(samplerUnit.second);
        if (legacyInput == legacyInputs.end()) {
            return failStageInfoLoad(errors, "Sampler " + samplerUnit.first + " is preset to unit " + std::to_string(samplerUnit.second) + " but the stage binds nothing there.");
        }
        if (!newStageInfo.stage->setNamedInput(samplerUnit.first, legacyInput->second)) {
            return failStageInfoLoad(errors, "Sampler " + samplerUnit.first + " is set to two different textures in one stage.");
        }
        matchedLegacyUnits.insert(samplerUnit.second);
    }
    for (const std::pair<const uint32_t, std::shared_ptr<Texture>>& legacyInput : legacyInputs) {
        if (matchedLegacyUnits.find(legacyInput.first) == matchedLegacyUnits.end()) {
            return failStageInfoLoad(errors, "Stage binds texture " + legacyInput.second->getName() + " to unit " + std::to_string(legacyInput.first) + " but no sampler is preset to it.");
        }
    }

    return true;
}

bool GraphicsPipeline::isReadByMoreStages(const std::pair<std::shared_ptr<Texture>, std::vector<size_t>> &left, const std::pair<std::shared_ptr<Texture>, std::vector<size_t>> &right) {
    return left.second.size() > right.second.size();
}

bool GraphicsPipeline::isEngineOwnedSampler(const std::string &samplerName) {
    //the backend binds these on the units in GraphicsInterface.h, see attachModelTexture/attachRigTexture
    if (samplerName == "allModelTransformsTexture" || samplerName == "allBoneTransformsTexture") {
        return true;
    }
    for (int32_t samplerIndex = 0; samplerIndex < Material::SAMPLER_COUNT; ++samplerIndex) {
        if (samplerName == Material::samplerName(static_cast<Material::Sampler>(samplerIndex))) {
            return true;
        }
    }
    return false;
}

bool GraphicsPipeline::assignTextureUnits(GraphicsInterface* graphicsInterface, std::string &error) {
    const uint32_t unitCount = (uint32_t)graphicsInterface->getMaxTextureImageUnits();
    const size_t stageCount = pipelineStages.size();

    //which units each stage may use, and which samplers its methods bind themselves
    std::vector<std::vector<bool>> unitAllowed(stageCount, std::vector<bool>(unitCount, false));
    std::vector<std::vector<std::pair<std::shared_ptr<GraphicsProgram>, std::string>>> selfBoundSamplers(stageCount);
    std::set<const GraphicsProgram*> seenPrograms;
    for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
        bool materialStage = false;
        for (const RenderMethods::RenderMethod& renderMethod : pipelineStages[stageIndex].renderMethods) {
            const std::shared_ptr<GraphicsProgram>& program = renderMethod.getGlslProgram();
            if (program == nullptr) {
                continue;
            }
            if (!seenPrograms.insert(program.get()).second) {
                error = "Program " + program->getProgramName() + " is used by more than one render method, its sampler units would conflict.";
                return false;
            }
            materialStage = materialStage || program->isMaterialRequired();
            std::vector<std::string> samplerNames;
            for (const std::pair<const std::string, std::shared_ptr<Uniform>>& uniform : program->getUniformMap()) {
                if (GraphicsProgramLoader::isSamplerType(uniform.second->type) && uniform.first.rfind("pre_", 0) != 0 && !isEngineOwnedSampler(uniform.first)) {
                    samplerNames.emplace_back(uniform.first);
                }
            }
            std::sort(samplerNames.begin(), samplerNames.end());//uniform map is unordered, assignment has to be repeatable
            for (const std::string& samplerName : samplerNames) {
                selfBoundSamplers[stageIndex].emplace_back(program, samplerName);
            }
        }
        for (uint32_t unit = GraphicsInterface::FIRST_PIPELINE_TEXTURE_UNIT; unit < unitCount; ++unit) {
            //material textures are bound on every draw, anything else on those units would be overwritten
            bool materialUnit = unit >= (uint32_t)Material::MATERIAL_SAMPLER_TEXTURE_UNIT_START &&
                                unit < (uint32_t)(Material::MATERIAL_SAMPLER_TEXTURE_UNIT_START + Material::SAMPLER_COUNT);
            unitAllowed[stageIndex][unit] = !(materialStage && materialUnit);
        }
    }

    //every texture the pipeline reads, with the stages reading it
    std::vector<std::pair<std::shared_ptr<Texture>, std::vector<size_t>>> textureUses;
    for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
        std::set<const Texture*> stageTextures;
        for (const std::pair<const std::string, std::shared_ptr<Texture>>& namedInput : pipelineStages[stageIndex].stage->getNamedInputs()) {
            if (!stageTextures.insert(namedInput.second.get()).second) {
                continue;
            }
            std::vector<std::pair<std::shared_ptr<Texture>, std::vector<size_t>>>::iterator useIterator = textureUses.begin();
            while (useIterator != textureUses.end() && useIterator->first != namedInput.second) {
                ++useIterator;
            }
            if (useIterator == textureUses.end()) {
                textureUses.emplace_back(namedInput.second, std::vector<size_t>());
                useIterator = textureUses.end() - 1;
            }
            useIterator->second.push_back(stageIndex);
        }
        size_t allowedUnitCount = (size_t)std::count(unitAllowed[stageIndex].begin(), unitAllowed[stageIndex].end(), true);
        size_t neededUnitCount = stageTextures.size() + selfBoundSamplers[stageIndex].size();
        if (neededUnitCount > allowedUnitCount) {
            error = "Stage " + std::to_string(stageIndex) + " (" + pipelineStages[stageIndex].stage->getFoundName() + ") needs " + std::to_string(neededUnitCount) +
                    " texture units, this GPU has " + std::to_string(allowedUnitCount) + " usable for it (" + std::to_string(unitCount) + " total).";
            return false;
        }
    }
    //textures read by more stages pick first, so the ones that would be rebound most often get a unit of their own
    std::stable_sort(textureUses.begin(), textureUses.end(), isReadByMoreStages);

    std::vector<std::vector<bool>> unitUsedInStage(stageCount, std::vector<bool>(unitCount, false));
    std::vector<uint32_t> textureCountOnUnit(unitCount, 0);
    std::vector<std::map<const Texture*, uint32_t>> stageTextureUnits(stageCount);
    for (const std::pair<std::shared_ptr<Texture>, std::vector<size_t>>& textureUse : textureUses) {
        int64_t sharedUnit = -1;
        //first a unit nothing else uses, then one shared only with textures of other stages
        for (uint32_t pass = 0; pass < 2 && sharedUnit < 0; ++pass) {
            for (uint32_t unit = 0; unit < unitCount && sharedUnit < 0; ++unit) {
                if (pass == 0 && textureCountOnUnit[unit] != 0) {
                    continue;
                }
                bool fits = true;
                for (size_t stageIndex : textureUse.second) {
                    fits = fits && unitAllowed[stageIndex][unit] && !unitUsedInStage[stageIndex][unit];
                }
                if (fits) {
                    sharedUnit = unit;
                }
            }
        }
        for (size_t stageIndex : textureUse.second) {
            int64_t unitForStage = sharedUnit;
            //no single unit is free in all of its stages, so it moves between stages and gets rebound
            for (uint32_t unit = 0; unit < unitCount && unitForStage < 0; ++unit) {
                if (unitAllowed[stageIndex][unit] && !unitUsedInStage[stageIndex][unit]) {
                    unitForStage = unit;
                }
            }
            if (unitForStage < 0) {
                error = "No texture unit left for " + textureUse.first->getName() + " in stage " + std::to_string(stageIndex) + ".";
                return false;
            }
            unitUsedInStage[stageIndex][unitForStage] = true;
            ++textureCountOnUnit[unitForStage];
            stageTextureUnits[stageIndex][textureUse.first.get()] = (uint32_t)unitForStage;
        }
    }

    //self bound samplers change texture every draw, they go where no input lives so they never evict one
    for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
        for (const std::pair<std::shared_ptr<GraphicsProgram>, std::string>& selfBoundSampler : selfBoundSamplers[stageIndex]) {
            int64_t samplerUnit = -1;
            for (uint32_t pass = 0; pass < 2 && samplerUnit < 0; ++pass) {
                for (uint32_t unit = 0; unit < unitCount && samplerUnit < 0; ++unit) {
                    if (unitAllowed[stageIndex][unit] && !unitUsedInStage[stageIndex][unit] && (pass == 1 || textureCountOnUnit[unit] == 0)) {
                        samplerUnit = unit;
                    }
                }
            }
            if (samplerUnit < 0) {
                error = "No texture unit left for sampler " + selfBoundSampler.second + " in stage " + std::to_string(stageIndex) + ".";
                return false;
            }
            unitUsedInStage[stageIndex][samplerUnit] = true;
            selfBoundSampler.first->setTextureUnit(selfBoundSampler.second, (int32_t)samplerUnit);
            selfBoundSampler.first->setUniform(selfBoundSampler.second, (int)samplerUnit);
        }
    }

    for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
        const std::shared_ptr<GraphicsPipelineStage>& stage = pipelineStages[stageIndex].stage;
        stage->clearInputUnits();
        for (const std::pair<const std::string, std::shared_ptr<Texture>>& namedInput : stage->getNamedInputs()) {
            uint32_t unit = stageTextureUnits[stageIndex][namedInput.second.get()];
            stage->setInputUnit(unit, namedInput.second);
            for (const RenderMethods::RenderMethod& renderMethod : pipelineStages[stageIndex].renderMethods) {
                const std::shared_ptr<GraphicsProgram>& program = renderMethod.getGlslProgram();
                if (program != nullptr && program->getUniformMap().find(namedInput.first) != program->getUniformMap().end()) {
                    program->setUniform(namedInput.first, (int)unit);
                }
            }
        }
    }
    return true;
}

void GraphicsPipeline::printTextureUnits(GraphicsInterface* graphicsInterface, const std::string &pipelineName) const {
    std::cout << "Texture units for pipeline " << pipelineName << ", " << graphicsInterface->getMaxTextureImageUnits() << " available:" << std::endl;
    std::map<uint32_t, std::vector<const Texture*>> unitOccupants;//in stage order, to count rebinds
    for (size_t stageIndex = 0; stageIndex < pipelineStages.size(); ++stageIndex) {
        const StageInfo& stageInfo = pipelineStages[stageIndex];
        std::vector<std::string> methodNames;
        for (const RenderMethods::RenderMethod& renderMethod : stageInfo.renderMethods) {
            methodNames.emplace_back(renderMethod.getGlslProgram() == nullptr ? renderMethod.getName() : renderMethod.getName() + " (" + renderMethod.getGlslProgram()->getProgramName() + ")");
        }
        std::cout << "  stage " << stageIndex << " " << stageInfo.stage->getFoundName() << ": " << StringUtils::join(methodNames, ", ") << std::endl;
        for (const std::pair<const std::string, std::shared_ptr<Texture>>& namedInput : stageInfo.stage->getNamedInputs()) {
            for (const std::pair<const uint32_t, std::shared_ptr<Texture>>& input : stageInfo.stage->getInputs()) {
                if (input.second == namedInput.second) {
                    std::cout << "    " << namedInput.first << " -> unit " << input.first << " (" << namedInput.second->getName() << ")" << std::endl;
                    break;
                }
            }
        }
        for (const std::pair<const uint32_t, std::shared_ptr<Texture>>& input : stageInfo.stage->getInputs()) {
            unitOccupants[input.first].push_back(input.second.get());
        }
        for (const RenderMethods::RenderMethod& renderMethod : stageInfo.renderMethods) {
            const std::shared_ptr<GraphicsProgram>& program = renderMethod.getGlslProgram();
            if (program == nullptr) {
                continue;
            }
            for (const std::pair<const std::string, std::shared_ptr<Uniform>>& uniform : program->getUniformMap()) {
                if (GraphicsProgramLoader::isSamplerType(uniform.second->type) && uniform.first.rfind("pre_", 0) != 0 && !isEngineOwnedSampler(uniform.first)) {
                    std::cout << "    " << uniform.first << " -> unit " << program->getTextureUnit(uniform.first) << " (bound by the render method)" << std::endl;
                }
            }
        }
    }
    //the frame wraps, so the last occupant of a unit is what the first stage finds there
    uint32_t inputRebindsPerFrame = 0;
    for (const std::pair<const uint32_t, std::vector<const Texture*>>& occupants : unitOccupants) {
        for (size_t occupantIndex = 0; occupantIndex < occupants.second.size(); ++occupantIndex) {
            const Texture* previous = occupants.second[(occupantIndex + occupants.second.size() - 1) % occupants.second.size()];
            if (previous != occupants.second[occupantIndex]) {
                ++inputRebindsPerFrame;
            }
        }
    }
    std::cout << "  stage input rebinds per frame: " << inputRebindsPerFrame << std::endl;
}

