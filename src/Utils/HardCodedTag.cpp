//
// Created by engin on 01/11/2023.
//

#include "HardCodedTags.h"

const std::string HardCodedTags::OBJECT_MODEL_STATIC        = "static_model_object";
const std::string HardCodedTags::OBJECT_MODEL_PHYSICAL      = "physical_model_object";

const std::string HardCodedTags::OBJECT_MODEL_BASIC         = "basic_model_object";
const std::string HardCodedTags::OBJECT_MODEL_ANIMATED      = "animated_model_object";
const std::string HardCodedTags::OBJECT_MODEL_TRANSPARENT   = "transparent_model_object";

const std::string HardCodedTags::OBJECT_PLAYER_BASIC        = "basic_player_attachment";
const std::string HardCodedTags::OBJECT_PLAYER_ANIMATED     = "animated_player_attachment";
const std::string HardCodedTags::OBJECT_PLAYER_TRANSPARENT  = "transparent_player_attachment";

const std::string HardCodedTags::CAMERA_LIGHT_DIRECTIONAL   = "directional_camera";
const std::string HardCodedTags::CAMERA_LIGHT_POINT         = "point_camera";
const std::string HardCodedTags::CAMERA_PLAYER              = "player_camera";

const std::string HardCodedTags::PICKED_OBJECT              =   "picked_object";

const std::vector<std::string> HardCodedTags::ALL_TAGS              = {
    HardCodedTags::OBJECT_MODEL_STATIC          ,
    HardCodedTags::OBJECT_MODEL_PHYSICAL        ,

    HardCodedTags::OBJECT_MODEL_BASIC           ,
    HardCodedTags::OBJECT_MODEL_ANIMATED        ,
    HardCodedTags::OBJECT_MODEL_TRANSPARENT     ,

    HardCodedTags::OBJECT_PLAYER_BASIC          ,
    HardCodedTags::OBJECT_PLAYER_ANIMATED       ,
    HardCodedTags::OBJECT_PLAYER_TRANSPARENT    ,

    HardCodedTags::CAMERA_LIGHT_DIRECTIONAL     ,
    HardCodedTags::CAMERA_LIGHT_POINT           ,
    HardCodedTags::CAMERA_PLAYER                ,

    HardCodedTags::PICKED_OBJECT
};