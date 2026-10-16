//
// Created by engin on 15.10.2026.
//

#include "EngineDataFiles.h"

const std::string EngineDataFiles::RELEASE = "./Data/Release.xml";
const std::string EngineDataFiles::USER_OPTIONS = "./Data/Options.xml";
const std::string EngineDataFiles::INPUT_BINDINGS = "./Data/inputBindings.xml";
const std::string EngineDataFiles::DEFAULT_FONT = "./Data/Fonts/Helvetica-Normal.ttf";
const std::string EngineDataFiles::DEBUG_FONT = "./Data/Fonts/Cousine-Regular.ttf";
const std::string EngineDataFiles::CROSSHAIR = "./Data/Textures/crosshair.png";
const std::string EngineDataFiles::PACKAGE_MANIFEST = "./Data/pack_manifest.xml";

std::vector<std::string> EngineDataFiles::getAll() {
    return {RELEASE, USER_OPTIONS, INPUT_BINDINGS, DEFAULT_FONT, DEBUG_FONT, CROSSHAIR};
}
