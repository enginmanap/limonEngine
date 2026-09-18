#include "RenderProfileScope.h"
#include "ProfilerState.h"
#include "Graphics/GraphicsPipelineStage.h"
#include "Graphics/Texture.h"
#include "limonAPI/Graphics/GraphicsProgram.h"
#include "GameObjects/Light.h"

#ifdef TRACY_ENABLE
//one source location per category, the label goes in as a name override. A label in the source location would
//create one per stage/method/light and they are capped at int16 in the Tracy server
static constexpr tracy::SourceLocationData STAGE_SOURCE_LOCATION{"RenderStage", "RenderProfileScope", TracyFile, (uint32_t)TracyLine, 0};
static constexpr tracy::SourceLocationData METHOD_SOURCE_LOCATION{"RenderMethod", "RenderProfileScope", TracyFile, (uint32_t)TracyLine, 0};
static constexpr tracy::SourceLocationData LIGHT_SOURCE_LOCATION{"RenderLight", "RenderProfileScope", TracyFile, (uint32_t)TracyLine, 0};
#endif

static const char* onOff(bool value) {
    return value ? "on" : "off";
}

static const char* cullModeName(GraphicsInterface::CullModes cullMode) {
    switch (cullMode) {
        case GraphicsInterface::CullModes::FRONT: return "front";
        case GraphicsInterface::CullModes::BACK: return "back";
        case GraphicsInterface::CullModes::NONE: return "none";
        case GraphicsInterface::CullModes::NO_CHANGE: return "no change";
    }
    return "unknown";
}

RenderProfileScope::RenderProfileScope(GraphicsInterface* graphicsWrapper, const GraphicsPipelineStage& stage, bool clear) : graphicsWrapper(graphicsWrapper) {
    if (graphicsWrapper == nullptr || !ProfilerState::isTracingGpuRendering()) {
        return;
    }
    std::string targets;
    for (const std::pair<const GraphicsInterface::FrameBufferAttachPoints, std::shared_ptr<Texture>>& output : stage.getOutputs()) {
        if (!targets.empty()) {
            targets += ", ";
        }
        targets += output.second->getName().empty() ? "unnamed" : output.second->getName();
    }
    if (targets.empty()) {
        targets = "default framebuffer";
    }
    std::string text = "size " + std::to_string(stage.getRenderWidth()) + "x" + std::to_string(stage.getRenderHeight()) +
                       "\ntargets " + targets +
                       "\nclear " + onOff(clear) +
                       ", blend " + onOff(stage.isBlendEnabled()) +
                       ", depth test " + onOff(stage.isDepthTestEnabled()) +
                       ", depth write " + onOff(stage.isDepthWriteEnabled()) +
                       ", cull " + cullModeName(stage.getCullMode());
    begin(Category::STAGE, stage.getFoundName(), text);
}

RenderProfileScope::RenderProfileScope(GraphicsInterface* graphicsWrapper, const std::string& methodName, const GraphicsProgram* program,
                                       const std::string& cameraName, const std::vector<HashUtil::HashedString>& tags) : graphicsWrapper(graphicsWrapper) {
    if (graphicsWrapper == nullptr || !ProfilerState::isTracingGpuRendering()) {
        return;
    }
    std::string tagList;
    for (const HashUtil::HashedString& tag : tags) {
        if (!tagList.empty()) {
            tagList += ", ";
        }
        tagList += tag.text;
    }
    std::string text = "program " + (program != nullptr ? program->getProgramName() : std::string("none")) +
                       "\ncamera " + (cameraName.empty() ? std::string("none") : cameraName) +
                       "\ntags " + (tagList.empty() ? std::string("none") : tagList);
    begin(Category::METHOD, methodName, text);
}

RenderProfileScope::RenderProfileScope(GraphicsInterface* graphicsWrapper, const Light& light, uint32_t layer) : graphicsWrapper(graphicsWrapper) {
    if (graphicsWrapper == nullptr || !ProfilerState::isTracingGpuRendering()) {
        return;
    }
    std::string label = light.getName();
    if (light.getLightType() == Light::LightTypes::DIRECTIONAL) {
        label += " cascade " + std::to_string(layer);
    }
    begin(Category::LIGHT, label, "");
}

void RenderProfileScope::begin(Category category, const std::string& label, const std::string& text) {
    active = true;
#ifdef TRACY_ENABLE
    const tracy::SourceLocationData* sourceLocation = &STAGE_SOURCE_LOCATION;
    switch (category) {
        case Category::STAGE: sourceLocation = &STAGE_SOURCE_LOCATION; break;
        case Category::METHOD: sourceLocation = &METHOD_SOURCE_LOCATION; break;
        case Category::LIGHT: sourceLocation = &LIGHT_SOURCE_LOCATION; break;
    }
    cpuZone.emplace(sourceLocation, TRACY_CALLSTACK, true);
    cpuZone->Name(label.c_str(), label.size());
    if (!text.empty()) {
        cpuZone->Text(text.c_str(), text.size());
    }
#endif
    //GPU zone name is the full label, so it must never contain a per frame number
    graphicsWrapper->beginGpuProfileZone(label.c_str(), true);
    statsAtBegin = graphicsWrapper->getCurrentFrameStats();
}

RenderProfileScope::~RenderProfileScope() {
    if (!active) {
        return;
    }
    const GraphicsInterface::RenderStats& statsAtEnd = graphicsWrapper->getCurrentFrameStats();
    graphicsWrapper->endGpuProfileZone();
#ifdef TRACY_ENABLE
    std::string statsText = "triangles " + std::to_string(statsAtEnd.triangleCount - statsAtBegin.triangleCount) +
                            "\nlines " + std::to_string(statsAtEnd.lineCount - statsAtBegin.lineCount) +
                            "\ndraw calls " + std::to_string(statsAtEnd.drawCallCount - statsAtBegin.drawCallCount) +
                            "\ninstances " + std::to_string(statsAtEnd.instanceCount - statsAtBegin.instanceCount) +
                            "\nbatches " + std::to_string(statsAtEnd.batchCount - statsAtBegin.batchCount) +
                            "\nprogram switches " + std::to_string(statsAtEnd.programSwitchCount - statsAtBegin.programSwitchCount) +
                            " of " + std::to_string(statsAtEnd.programSwitchRequestCount - statsAtBegin.programSwitchRequestCount) +
                            "\ntexture binds " + std::to_string(statsAtEnd.textureBindCount - statsAtBegin.textureBindCount) +
                            " of " + std::to_string(statsAtEnd.textureBindRequestCount - statsAtBegin.textureBindRequestCount) +
                            "\nmaterial switches " + std::to_string(statsAtEnd.materialSwitchCount - statsAtBegin.materialSwitchCount) +
                            "\nuniform sets " + std::to_string(statsAtEnd.uniformSetCount - statsAtBegin.uniformSetCount);
    cpuZone->Text(statsText.c_str(), statsText.size());
    cpuZone.reset();
#endif
}
