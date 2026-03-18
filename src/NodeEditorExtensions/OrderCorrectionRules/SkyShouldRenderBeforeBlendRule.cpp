//
// Created by engin on 18/03/2026.
//

#include "SkyShouldRenderBeforeBlendRule.h"
#include "Graphics/GraphicsPipeline.h"

void SkyShouldRenderBeforeBlendRule::apply(std::vector<std::pair<std::set<const Node *>, std::shared_ptr<GraphicsPipeline::StageInfo>>> &orderedStages) {
        // Find sky stage and ensure it renders before all blended stages
    struct SkyInfo {
        size_t index;
        std::shared_ptr<Texture> outputs[7];
        std::shared_ptr<Texture> depth;
    };

    std::vector<SkyInfo> skyStages;
    for (size_t i = 0; i < orderedStages.size(); ++i) {
        for (const auto &renderMethod:orderedStages[i].second->renderMethods) {
            if (renderMethod.getName() == "Render Sky") {
                SkyInfo skyInfo;
                skyInfo.index = i;

                // Get all outputs from sky stage
                const auto& skyOutputs = orderedStages[i].second->stage->getOutputs();

                // Initialize all outputs to nullptr
                for (int idx = 0; idx <= 6; ++idx) {
                    skyInfo.outputs[idx] = nullptr;
                }
                skyInfo.depth = nullptr;

                // Extract outputs from the map
                for (const auto& outputPair : skyOutputs) {
                    if (outputPair.second != nullptr) {
                        // Check color outputs (0-6)
                        if (outputPair.first >= GraphicsInterface::FrameBufferAttachPoints::COLOR0 &&
                            outputPair.first <= GraphicsInterface::FrameBufferAttachPoints::COLOR6) {
                            int colorIndex = static_cast<int>(outputPair.first) - static_cast<int>(GraphicsInterface::FrameBufferAttachPoints::COLOR0);
                            skyInfo.outputs[colorIndex] = outputPair.second;
                        }
                        // Check depth output
                        else if (outputPair.first == GraphicsInterface::FrameBufferAttachPoints::DEPTH) {
                            skyInfo.depth = outputPair.second;
                        }
                    }
                }

                skyStages.push_back(skyInfo);
                break;//We don't need to process same stage again.
            }
        }
    }

    for (size_t i = 0; i < skyStages.size(); ++i) {
        for (size_t j = 0; j < skyStages[i].index; ++j) {//we only need to check stages before sky
            const std::shared_ptr<GraphicsPipelineStage> currentStage = orderedStages[j].second->stage;
            if (currentStage->isBlendEnabled()) {
                //Check if this stage has any shared outputs with sky
                bool hasSharedOutput = false;

                // Get all outputs from current stage
                const auto& currentOutputs = currentStage->getOutputs();

                // Check if any current output matches sky outputs
                for (const auto& currentOutputPair : currentOutputs) {
                    if (currentOutputPair.second != nullptr) {
                        // Check color outputs (0-6)
                        if (currentOutputPair.first >= GraphicsInterface::FrameBufferAttachPoints::COLOR0 &&
                            currentOutputPair.first <= GraphicsInterface::FrameBufferAttachPoints::COLOR6) {
                            int colorIndex = static_cast<int>(currentOutputPair.first) - static_cast<int>(GraphicsInterface::FrameBufferAttachPoints::COLOR0);
                            if (skyStages[i].outputs[colorIndex] == currentOutputPair.second) {
                                hasSharedOutput = true;
                                break;
                            }
                        }
                        // Check depth output
                        else if (currentOutputPair.first == GraphicsInterface::FrameBufferAttachPoints::DEPTH) {
                            if (skyStages[i].depth == currentOutputPair.second) {
                                hasSharedOutput = true;
                                break;
                            }
                        }
                    }
                }

                //If it shares some output, move it before sky
                if (hasSharedOutput) {
                    std::swap(orderedStages[j], orderedStages[skyStages[i].index]);
                    skyStages[i].index = j; // Update sky index after swap
                }
            }
        }
    }
}
