#include <Python.h>
#include "PythonLimonAPIBinding.h"
#include "PythonTypeCasters.h"
#include <pybind11/embed.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limonAPI/LimonAPI.h>
#include <limonAPI/LimonTypes.h>
#include "GenericParameterConverter.h"
#include "PyTimedEventCallback.h"

void bindLimonAPI(pybind11::module_& m) {
    // Bind LimonTypes::Vec4
    pybind11::class_<LimonTypes::Vec4>(m, "Vec4")
            .def(pybind11::init<>())
            .def(pybind11::init<float, float, float, float>(),
                 pybind11::arg("x") = 0.0f, pybind11::arg("y") = 0.0f,
                 pybind11::arg("z") = 0.0f, pybind11::arg("w") = 0.0f)
            .def_readwrite("x", &LimonTypes::Vec4::x)
            .def_readwrite("y", &LimonTypes::Vec4::y)
            .def_readwrite("z", &LimonTypes::Vec4::z)
            .def_readwrite("w", &LimonTypes::Vec4::w);

    // Bind audio mixing channels (buses)
    pybind11::enum_<LimonTypes::AudioChannel>(m, "AudioChannel")
            .value("MASTER", LimonTypes::AudioChannel::MASTER)
            .value("MUSIC", LimonTypes::AudioChannel::MUSIC)
            .value("SFX", LimonTypes::AudioChannel::SFX)
            .value("SPEECH", LimonTypes::AudioChannel::SPEECH);

    // Bind ProfileScope — non-constructable from Python; obtained only via LimonAPI.profile_scope()
    pybind11::class_<ProfileScope>(m, "ProfileScope")
        .def("__enter__", [](ProfileScope& self) -> ProfileScope& { return self; },
             pybind11::return_value_policy::reference)
        .def("__exit__", [](ProfileScope& self, pybind11::object, pybind11::object, pybind11::object) {
            self.endZone();
            return false;
        });

    // Bind LimonAPI
    pybind11::class_<LimonAPI> limon(m, "LimonAPI");

    limon.def("get_options", &LimonAPI::getOptions, "Get engine options")
            .def("save_options", &LimonAPI::saveOptions, "Save current options to user options file; returns True on success")

            // GUI Methods
            .def("add_gui_text",
                 [](LimonAPI &self,
                    const std::string &font_file_path,
                    unsigned int font_size,
                    const std::string &name,
                    const std::string &text,
                    const pybind11::object &color,
                    const pybind11::object &position,
                    float rotation) -> unsigned int {
                     glm::vec3 color_vec(1.0f, 1.0f, 1.0f);
                     if (!color.is_none()) {
                         color_vec = pybind11::cast<glm::vec3>(color);
                     }

                     glm::vec2 pos_vec(0.0f, 0.0f);
                     if (!position.is_none()) {
                         pos_vec = pybind11::cast<glm::vec2>(position);
                     }

                     return self.addGuiText(font_file_path, font_size, name, text,
                                            color_vec, pos_vec, rotation);
                 },
                 "Add a text element to the GUI\n\
             Args:\n\
             font_file_path (str): Path to the font file\n\
             font_size (int): Size of the font\n\
             name (str): Name of the text element\n\
             text (str): The text to display\n\
             color (tuple, optional): RGB color as (r, g, b). Defaults to white (1.0, 1.0, 1.0).\n\
             position (tuple, optional): Position as (x, y). Defaults to (0.0, 0.0).\n\
             rotation (float, optional): Rotation in radians, clockwise. Defaults to 0.0.",
                 pybind11::arg("font_file_path"),
                 pybind11::arg("font_size"),
                 pybind11::arg("name"),
                 pybind11::arg("text"),
                 pybind11::arg("color") = pybind11::none(),
                 pybind11::arg("position") = pybind11::none(),
                 pybind11::arg("rotation") = 0.0f)
            .def("add_gui_image",
                 [](LimonAPI &self,
                    const std::string &image_file_path,
                    const std::string &name,
                    const pybind11::object &position,
                    const pybind11::object &scale,
                    float rotation) -> unsigned int {
                     LimonTypes::Vec2 pos_vec{0.0f, 0.0f};
                     if (!position.is_none()) {
                         glm::vec2 pos = pybind11::cast<glm::vec2>(position);
                         pos_vec.x = pos.x;
                         pos_vec.y = pos.y;
                     }

                     LimonTypes::Vec2 scale_vec{1.0f, 1.0f};
                     if (!scale.is_none()) {
                         glm::vec2 scl = pybind11::cast<glm::vec2>(scale);
                         scale_vec.x = scl.x;
                         scale_vec.y = scl.y;
                     }

                     return self.addGuiImage(image_file_path, name, pos_vec, scale_vec, rotation);
                 },
                 "Add an image to the GUI\n\
             Args:\n\
             image_file_path (str): Path to the image file\n\
             name (str): Name of the image element\n\
             position (tuple, optional): Position as (x, y). Defaults to (0.0, 0.0).\n\
             scale (tuple, optional): Scale as (x, y). Defaults to (1.0, 1.0).\n\
             rotation (float, optional): Rotation in radians, clockwise. Defaults to 0.0.",
                 pybind11::arg("image_file_path"),
                 pybind11::arg("name"),
                 pybind11::arg("position") = pybind11::none(),
                 pybind11::arg("scale") = pybind11::none(),
                 pybind11::arg("rotation") = 0.0f)
            .def("update_gui_text", &LimonAPI::updateGuiText,
                 "Update GUI text element",
                 pybind11::arg("gui_text_id"), pybind11::arg("new_text"))
            .def("remove_gui_element", &LimonAPI::removeGuiElement,
                 "Remove a GUI element",
                 pybind11::arg("gui_element_id"))
            .def("get_gui_element_position", &LimonAPI::getGuiElementPosition,
                 "Returns the screen position of a GUI element as Vec4 (x, y, 0, 1). Returns zero Vec4 if not found",
                 pybind11::arg("gui_element_id"))
            .def("set_gui_element_position", &LimonAPI::setGuiElementPosition,
                 "Set the screen position of a GUI element. Returns False if not found",
                 pybind11::arg("gui_element_id"), pybind11::arg("position"))
            .def("set_gui_element_visible", &LimonAPI::setGuiElementVisible,
                 "Show or hide a GUI element. Returns False if not found",
                 pybind11::arg("gui_element_id"), pybind11::arg("visible"))

            // Object Manipulation
            .def("set_object_temporary", &LimonAPI::setObjectTemporary,
                 "Set if an object is temporary",
                 pybind11::arg("object_id"), pybind11::arg("temporary"))
            .def("attach_object_to_object", &LimonAPI::attachObjectToObject,
                 "Attach child to parent; child's current transform is treated as local offset relative to parent.",
                 pybind11::arg("object_id"), pybind11::arg("object_to_attach_to_id"))
            .def("attach_object_to_object_at_world_position", &LimonAPI::attachObjectToObjectAtWorldPosition,
                 "Attach child to parent; child keeps its current world position (local offset is derived automatically).",
                 pybind11::arg("object_id"), pybind11::arg("object_to_attach_to_id"))
            .def("remove_trigger_object", &LimonAPI::removeTriggerObject,
                 "Remove a trigger object",
                 pybind11::arg("trigger_object_id"))
            .def("is_inside_trigger", &LimonAPI::isInsideTrigger,
                 "Returns True if a player is currently inside the trigger volume",
                 pybind11::arg("trigger_id"))
            .def("get_object_by_name", &LimonAPI::getObjectByName,
                 "Find an object by name. Returns 0 if not found. Searches models, GUI elements, and triggers",
                 pybind11::arg("name"))
            .def("get_object_parent", &LimonAPI::getObjectParent,
                 "Returns the parent object ID of the given object. Returns 0 if no parent or not found",
                 pybind11::arg("object_id"))
            .def("is_object_physics_connected", &LimonAPI::isObjectPhysicsConnected,
                 "Returns True if the object is active in the physics simulation",
                 pybind11::arg("object_id"))
            .def("get_object_linear_velocity", &LimonAPI::getObjectLinearVelocity,
                 "Returns the linear velocity of an object as Vec4 (w=0). Returns zero Vec4 if not found",
                 pybind11::arg("object_id"))
            .def("set_object_linear_velocity", &LimonAPI::setObjectLinearVelocity,
                 "Set the linear velocity of an object. Returns False if not found",
                 pybind11::arg("object_id"), pybind11::arg("velocity"))
            .def("get_object_mass", &LimonAPI::getObjectMass,
                 "Returns the mass of an object in kg. Returns 0.0 for static objects or if not found",
                 pybind11::arg("object_id"))
            .def("disconnect_object_from_physics", &LimonAPI::disconnectObjectFromPhysics,
                 "Disable physics for an object",
                 pybind11::arg("object_id"))
            .def("reconnect_object_to_physics", &LimonAPI::reconnectObjectToPhysics,
                 "Re-enable physics for an object",
                 pybind11::arg("object_id"))
            .def("apply_force", &LimonAPI::applyForce,
                 "Apply force to an object",
                 pybind11::arg("object_id"), pybind11::arg("force_position"), pybind11::arg("force_amount"))
            .def("apply_force_to_player", &LimonAPI::applyForceToPlayer,
                 "Apply force to the player",
                 pybind11::arg("force_amount"))
            .def("add_object_translate", &LimonAPI::addObjectTranslate,
                 "Add to an object's position",
                 pybind11::arg("object_id"), pybind11::arg("translation"))
            .def("add_object_scale", &LimonAPI::addObjectScale,
                 "Add to an object's scale",
                 pybind11::arg("object_id"), pybind11::arg("scale"))
            .def("add_object_orientation", &LimonAPI::addObjectOrientation,
                 "Add to an object's orientation",
                 pybind11::arg("object_id"), pybind11::arg("orientation"))
            .def("set_object_translate", &LimonAPI::setObjectTranslate,
                 "Set an object's position",
                 pybind11::arg("object_id"), pybind11::arg("translation"))
            .def("set_object_mass", &LimonAPI::setObjectMass,
                 "Set a model's mass (0 = static triangle mesh, >0 = dynamic convex hull). No effect on animated models.",
                 pybind11::arg("object_id"), pybind11::arg("mass"))
            .def("set_object_scale", &LimonAPI::setObjectScale,
                 "Set an object's scale",
                 pybind11::arg("object_id"), pybind11::arg("scale"))
            .def("set_object_orientation", &LimonAPI::setObjectOrientation,
                 "Set an object's orientation",
                 pybind11::arg("object_id"), pybind11::arg("orientation"))
            .def("get_object_transformation", &LimonAPI::getObjectTransformation,
                 "Get an object's transformation",
                 pybind11::arg("object_id"))
            .def("get_object_position", &LimonAPI::getObjectPosition,
                 "Get an object's world-space position (Vec4)",
                 pybind11::arg("object_id"))
            .def("get_object_front_vector", &LimonAPI::getObjectFrontVector,
                 "Get an object's world-space forward direction (Vec4)",
                 pybind11::arg("object_id"))
            .def("get_object_transformation_matrix", &LimonAPI::getObjectTransformationMatrix,
                 "Get an object's transformation matrix. Prefer this over get_object_transformation for physical objects",
                 pybind11::arg("object_id"))
            .def("get_model_children", &LimonAPI::getModelChildren,
                 "Get child object IDs of a model",
                 pybind11::arg("model_id"))

            // Sound
            .def("play_sound", &LimonAPI::playSound,
                 "Play a sound. Returns sound ID",
                 pybind11::arg("sound_path"), pybind11::arg("position"),
                 pybind11::arg("position_relative") = false, pybind11::arg("looped") = false,
                 pybind11::arg("reference_distance") = 2.0f, pybind11::arg("max_distance") = 50.0f)
            .def("stop_sound", &LimonAPI::stopSound,
                 "Stop a playing sound. Returns False if not found",
                 pybind11::arg("sound_id"))
            .def("pause_sound", &LimonAPI::pauseSound,
                 "Pause a playing sound without stopping it. Returns False if not found",
                 pybind11::arg("sound_id"))
            .def("resume_sound", &LimonAPI::resumeSound,
                 "Resume a paused sound. Returns False if not found",
                 pybind11::arg("sound_id"))
            .def("set_sound_volume", &LimonAPI::setSoundVolume,
                 "Set the volume of a sound. Returns False if not found",
                 pybind11::arg("sound_id"), pybind11::arg("volume"))
            .def("set_sound_looped", &LimonAPI::setSoundLooped,
                 "Change whether a sound loops. Returns False if not found",
                 pybind11::arg("sound_id"), pybind11::arg("looped"))
            .def("is_sound_playing", &LimonAPI::isSoundPlaying,
                 "Returns True if the sound is currently playing",
                 pybind11::arg("sound_id"))

            // Music (dedicated MUSIC channel)
            .def("set_music", &LimonAPI::setMusic,
                 "Switch the level music on the MUSIC channel. fade_seconds > 0 crossfades. Returns True on success",
                 pybind11::arg("music_path"), pybind11::arg("fade_seconds") = 0.0f, pybind11::arg("looped") = true)
            .def("stop_music", &LimonAPI::stopMusic,
                 "Stop the level music, optionally fading out over fade_seconds. Returns False if no music set",
                 pybind11::arg("fade_seconds") = 0.0f)
            .def("get_music_name", &LimonAPI::getMusicName,
                 "Get the current music asset path, or empty string if none")
            .def("is_music_playing", &LimonAPI::isMusicPlaying,
                 "Returns True if level music is currently playing")
            // Channel volumes are global options ("soundVolumeMaster/Music/SFX/Speech"); change them
            // via get_options()/save_options() and the engine applies them to the audio mixer.

            // AI / Player Interaction
            .def("interact_with_player", [](LimonAPI& self, pybind11::object py_params) {
                std::vector<LimonTypes::GenericParameter> params =
                    GenericParameterConverter::convertPythonListToGenericParameterVector(py_params);
                self.interactWithPlayer(params);
            }, "Send interaction data to the active player extension",
                 pybind11::arg("interaction_information"))
            .def("interact_with_ai", [](LimonAPI& self, uint32_t ai_id, pybind11::object py_params) -> bool {
                try {
                    std::vector<LimonTypes::GenericParameter> params =
                        GenericParameterConverter::convertPythonListToGenericParameterVector(py_params);
                    return self.interactWithAI(ai_id, params);
                } catch (const std::exception& e) {
                    std::cerr << "Error in interact_with_ai: " << e.what() << std::endl;
                    return false;
                }
            }, "Interact with an AI actor",
                 pybind11::arg("ai_id"), pybind11::arg("interaction_information"))

            // Particle Systems
            .def("disable_particle_emitter", &LimonAPI::disableParticleEmitter,
                 "Disable a particle emitter",
                 pybind11::arg("emitter_id"))
            .def("enable_particle_emitter", &LimonAPI::enableParticleEmitter,
                 "Enable a particle emitter",
                 pybind11::arg("emitter_id"))
            .def("add_particle_emitter", &LimonAPI::addParticleEmitter,
                 "Add a new particle emitter",
                 pybind11::arg("name"), pybind11::arg("texture_file"),
                 pybind11::arg("start_position"), pybind11::arg("max_start_distances"),
                 pybind11::arg("size"), pybind11::arg("count"),
                 pybind11::arg("life_time"), pybind11::arg("particles_per_ms"),
                 pybind11::arg("continuously_emit"))
            .def("remove_particle_emitter", &LimonAPI::removeParticleEmitter,
                 "Remove a particle emitter",
                 pybind11::arg("emitter_id"))
            .def("set_emitter_particle_speed", &LimonAPI::setEmitterParticleSpeed,
                 "Set particle speed for an emitter",
                 pybind11::arg("emitter_id"), pybind11::arg("speed_multiplier"),
                 pybind11::arg("speed_offset"))
            .def("set_emitter_particle_gravity", &LimonAPI::setEmitterParticleGravity,
                 "Set particle gravity for an emitter",
                 pybind11::arg("emitter_id"), pybind11::arg("gravity"))

            // Ray Casting
            .def("ray_cast_to_cursor", [](LimonAPI& self) -> pybind11::list {
                try {
                    std::vector<LimonTypes::GenericParameter> hitDetails = self.rayCastToCursor();
                    return GenericParameterConverter::convertGenericParameterVectorToObjects(hitDetails);
                } catch (const std::exception& e) {
                    std::cerr << "Error in ray_cast_to_cursor: " << e.what() << std::endl;
                    return pybind11::list();
                }
            }, "Cast a ray from camera to cursor position")
            .def("ray_cast_first_hit", [](LimonAPI& self, const LimonTypes::Vec4& start, const LimonTypes::Vec4& direction) -> pybind11::list {
                std::vector<LimonTypes::GenericParameter> hitDetails = self.rayCastFirstHit(start, direction);
                pybind11::list result = GenericParameterConverter::convertGenericParameterVectorToObjects(hitDetails);
                return result;
            }, "Cast a ray from start point in direction",
                 pybind11::arg("start"), pybind11::arg("direction"))

            // Lighting
            .def("add_light", &LimonAPI::addLight,
                 "Add a light. lightType: 1=directional, 2=point. Returns light ID, or 0 on failure.",
                 pybind11::arg("light_type"), pybind11::arg("position"), pybind11::arg("color"))
            .def("remove_light", &LimonAPI::removeLight,
                 "Remove a light by ID. Returns true on success.",
                 pybind11::arg("light_id"))
            .def("add_light_translate", &LimonAPI::addLightTranslate,
                 "Translate a light",
                 pybind11::arg("light_id"), pybind11::arg("translation"))
            .def("set_light_color", &LimonAPI::setLightColor,
                 "Set a light's color",
                 pybind11::arg("light_id"), pybind11::arg("color"))
            .def("get_light_position", &LimonAPI::getLightPosition,
                 "Returns the world-space position of a light as Vec4 (w=1). Returns zero Vec4 if not found",
                 pybind11::arg("light_id"))
            .def("get_light_color", &LimonAPI::getLightColor,
                 "Returns the color of a light as Vec4 (w=1). Returns zero Vec4 if not found",
                 pybind11::arg("light_id"))
            .def("set_light_translate", &LimonAPI::setLightTranslate,
                 "Set a light's absolute position. Returns False if not found",
                 pybind11::arg("light_id"), pybind11::arg("position"))

            // World Management
            .def("load_and_remove", &LimonAPI::loadAndRemove,
                 "Load a new world and remove the current one",
                 pybind11::arg("world_file_name"))
            .def("return_previous_world", &LimonAPI::returnPreviousWorld,
                 "Return to the previously loaded world")
            .def("quit_game", &LimonAPI::quitGame,
                 "Quit the game")
            .def("change_render_pipeline", &LimonAPI::changeRenderPipeline,
                 "Change the render pipeline",
                 pybind11::arg("pipeline_file_name"))

            // Player Related
            .def("kill_player", &LimonAPI::killPlayer,
                 "Kill the player")
            .def("get_player_look_direction", &LimonAPI::getPlayerLookDirection,
                 "Returns the player's normalized look direction as Vec4 (w=0)")
            .def("get_camera_position", &LimonAPI::getCameraPosition,
                 "Returns the camera's world position as Vec4 (same as player position)")
            .def("get_camera_look_direction", &LimonAPI::getCameraLookDirection,
                 "Returns the camera's normalized look direction as Vec4 (w=0)")
            .def("create_camera_rig", &LimonAPI::createCameraRig,
                 "Create a registered camera rig type and return its world object id (0 if unknown)")
            .def("activate_camera_rig", &LimonAPI::activateCameraRig,
                 "Make the camera rig with this id drive the player camera (true if found)")
            .def("deactivate_camera_rig", &LimonAPI::deactivateCameraRig,
                 "Revert to the player's own camera")
            .def("set_model_animation", &LimonAPI::setModelAnimation,
                 "Set an animation for a model",
                 pybind11::arg("model_id"), pybind11::arg("animation_name"),
                 pybind11::arg("looped") = true)
            .def("set_model_animation_with_blend", &LimonAPI::setModelAnimationWithBlend,
                 "Set an animation for a model with blending",
                 pybind11::arg("model_id"), pybind11::arg("animation_name"),
                 pybind11::arg("looped") = true, pybind11::arg("blend_time") = 100)
            .def("set_model_animation_speed", &LimonAPI::setModelAnimationSpeed,
                 "Set the animation speed for a model",
                 pybind11::arg("model_id"), pybind11::arg("speed"))

            // Variable Management
            .def("get_variable", &LimonAPI::getVariable,
                 "Get a script variable by name",
                 pybind11::arg("variable_name"),
                 pybind11::return_value_policy::reference)

            // Input
            .def("simulate_input", &LimonAPI::simulateInput,
                 "Simulate input events",
                 pybind11::arg("input_states"))

            // Trigger System
            .def("get_result_of_trigger", [](LimonAPI& self, uint32_t trigger_object_id, uint32_t trigger_code_id) -> pybind11::list {
                std::vector<LimonTypes::GenericParameter> result = self.getResultOfTrigger(trigger_object_id, trigger_code_id);
                return GenericParameterConverter::convertGenericParameterVectorToObjects(result);
            }, "Get result of a trigger",
                 pybind11::arg("trigger_object_id"), pybind11::arg("trigger_code_id"))

            // Logging
            .def("log", &LimonAPI::log,
                 "Write a message to the engine log",
                 pybind11::arg("subsystem"), pybind11::arg("level"), pybind11::arg("text"))

            // Debug line drawing
            .def("draw_debug_line",
                 &LimonAPI::drawDebugLine,
                 "Create a new persistent debug line buffer with one line; returns buffer ID",
                 pybind11::arg("from_pos"), pybind11::arg("to_pos"),
                 pybind11::arg("from_color"), pybind11::arg("to_color"),
                 pybind11::arg("require_camera_transform") = true)
            .def("add_to_debug_line",
                 &LimonAPI::addToDebugLine,
                 "Append a line segment to an existing debug line buffer",
                 pybind11::arg("buffer_id"),
                 pybind11::arg("from_pos"), pybind11::arg("to_pos"),
                 pybind11::arg("from_color"), pybind11::arg("to_color"),
                 pybind11::arg("require_camera_transform") = true)
            .def("clear_debug_lines",
                 &LimonAPI::clearDebugLines,
                 "Remove a debug line buffer; returns false if buffer_id was not found",
                 pybind11::arg("buffer_id"))

            // Profiling
            .def("profile_scope", [](LimonAPI& self, const std::string& name) {
                return self.profileScope(name);
            }, "Begin a named profiling zone; use as context manager",
                 pybind11::arg("name"));

    // Animation methods
    // TODO: remove this binding only after 0.7 is released and 0.8 work has started.
    limon.def("animate_model", &LimonAPI::animateModel,
              "[DEPRECATED] Animate a model by numeric animation index. Animation indices are positional and depend on load order; use animate_model_by_name() instead.",
              pybind11::arg("model_id"), pybind11::arg("animation_id"),
              pybind11::arg("looped") = false, pybind11::arg("sound_path") = "");

    limon.def("animate_model_by_name", &LimonAPI::animateModelByName,
              "Animate a model using the loaded animation's name instead of its index. Returns the model id, or 0 if no loaded animation matches the name",
              pybind11::arg("model_id"), pybind11::arg("animation_name"),
              pybind11::arg("looped") = false, pybind11::arg("sound_path") = "");

    limon.def("list_loaded_animations", &LimonAPI::listLoadedAnimations,
              "Returns the names of all custom (transformation) animations loaded in the world. These names can be passed to animate_model_by_name");

    limon.def("get_model_animation_name", &LimonAPI::getModelAnimationName,
              "Get the name of the current animation for a model",
              pybind11::arg("model_id"));

    limon.def("get_model_animation_finished", &LimonAPI::getModelAnimationFinished,
              "Check if model's animation has finished",
              pybind11::arg("model_id"));

    limon.def("get_model_animation_progress", &LimonAPI::getModelAnimationProgress,
              "Returns the normalized progress [0.0, 1.0] of a running custom animation. Returns 0.0 if no active custom animation",
              pybind11::arg("model_id"));

    limon.def("list_model_animations", &LimonAPI::listModelAnimations,
              "Returns a list of animation names available on the model asset",
              pybind11::arg("model_id"));

    // Object manipulation methods
    limon.def("add_object",
              [](LimonAPI &self,
                 const std::string &model_file_path,
                 float model_weight,
                 bool physical,
                 const glm::vec3 &position,
                 const glm::vec3 &scale,
                 const glm::quat &orientation) {
                  return self.addObject(model_file_path, model_weight, physical, position, scale, orientation);
              },
              "Add a new object to the scene",
              pybind11::arg("model_file_path"),
              pybind11::arg("model_weight") = 1.0f,
              pybind11::arg("physical") = true,
              pybind11::arg("position") = glm::vec3(0, 0, 0),
              pybind11::arg("scale") = glm::vec3(1, 1, 1),
              pybind11::arg("orientation") = glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    limon.def("remove_object", &LimonAPI::removeObject,
              "Remove an object from the scene",
              pybind11::arg("object_id"), pybind11::arg("remove_children") = true);

    // Player methods
    limon.def("get_player_position", [](LimonAPI &self) {
        glm::vec3 position, center, up, right;
        self.getPlayerPosition(position, center, up, right);
        return pybind11::make_tuple(
            pybind11::cast(position),
            pybind11::cast(center),
            pybind11::cast(up),
            pybind11::cast(right)
        );
    }, "Get player position, view direction, up and right vectors");

    limon.def("get_player_attached_model", &LimonAPI::getPlayerAttachedModel,
              "Get the ID of the model attached to the player");

    limon.def("get_player_attached_model_offset", &LimonAPI::getPlayerAttachedModelOffset,
              "Get the offset of the model attached to the player");

    limon.def("set_player_attached_model_offset", &LimonAPI::setPlayerAttachedModelOffset,
              "Set the offset of the model attached to the player",
              pybind11::arg("new_offset"));

    // World management
    limon.def("load_and_switch_world", &LimonAPI::loadAndSwitchWorld,
              "Load and switch to a new world",
              pybind11::arg("world_file_name"));

    limon.def("return_to_world", &LimonAPI::returnToWorld,
              "Return to a previously loaded world",
              pybind11::arg("world_file_name"));

    // Timed events with Python callback support
    limon.def("add_timed_event",
              [](LimonAPI &self, uint64_t wait_time, bool use_wall_time,
                 const pybind11::function &callback,
                 const pybind11::object &py_params) {
                  std::vector<LimonTypes::GenericParameter> params;
                  if (!py_params.is_none()) {
                      params = GenericParameterConverter::convertPythonListToGenericParameterVector(py_params);
                  }
                  PyTimedEventCallback cb = PyTimedEventCallback(callback);
                  return self.addTimedEvent(wait_time, use_wall_time, cb, params);
              },
              "Add a timed event with a Python callback",
              pybind11::arg("wait_time"),
              pybind11::arg("use_wall_time"),
              pybind11::arg("callback"),
              pybind11::arg("parameters") = pybind11::none());

    limon.def("cancel_timed_event", &LimonAPI::cancelTimedEvent,
              "Cancel a previously scheduled timed event",
              pybind11::arg("timer_id"));

    // Utility function to create a Vec4 from Python
    m.def("create_vec4", [](float x, float y, float z, float w) {
              return LimonTypes::Vec4{x, y, z, w};
          }, "Create a new Vec4",
          pybind11::arg("x") = 0.0f, pybind11::arg("y") = 0.0f,
          pybind11::arg("z") = 0.0f, pybind11::arg("w") = 0.0f);
}
