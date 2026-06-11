# limonimp.py
import limon
from typing import List, Optional
from player_extension_interface import PlayerExtensionInterface
from python_third_person_camera import ThirdPersonCamera
from generic_parameter import RequestParameterType, ValueType, GenericParameter

"""
    This is an example of a player extension implementation through Python.

    The base class is the pure-Python PlayerExtensionInterface from
    Engine/Scripts, NOT limon.PlayerExtensionInterface. `limon` is still
    imported below only for value/enum types (Vec4, Inputs, LogSubsystem,
    CameraAttachment) that the engine exposes.
"""
class PythonPlayerExtension(PlayerExtensionInterface):
    def __init__(self, limon_api):
        super().__init__(limon_api)
        self._limon_api = limon_api
        self.playerAttachedModelID = -1
        self.playerAttachedPistolId = -1
        self.camera_instance = None

        try:
            print("Initializing PythonPlayerExtension")
            self.playerAttachedModelID = self._limon_api.get_player_attached_model()
            print(f"Player model ID: {self.playerAttachedModelID}")

            if hasattr(self._limon_api, 'get_model_children') and self.playerAttachedModelID != -1:
                children = self._limon_api.get_model_children(self.playerAttachedModelID)
                if children and len(children) > 0:
                    self.playerAttachedPistolId = children[0]
                    print(f"Pistol ID: {self.playerAttachedPistolId}")

            print("Initializing third person camera")
            self.camera_instance = ThirdPersonCamera(self._limon_api)

        except Exception as e:
            print(f"Error in PythonPlayerExtension.__init__: {str(e)}")
            import traceback
            traceback.print_exc()


    def process_input(self, input_states, player_info, time: int) -> None:
        """
        Process input for the player.

        Args:
            input_states: The current input states
            player_info: Information about the player
            time: Current time in milliseconds
        """
        try:

            soundOffset = limon.Vec4()
            soundOffset.x = 0.0
            soundOffset.y = 0.0
            soundOffset.z = 0.0
            if input_states.get_input_events(limon.Inputs.MOUSE_BUTTON_LEFT) and input_states.get_input_status(limon.Inputs.MOUSE_BUTTON_LEFT):
                self._limon_api.log(limon.LogSubsystem.INPUT, limon.LogLevel.INFO, "player shoot")
                self._limon_api.play_sound("./Data/Sounds/EasyFPS/shot.wav", soundOffset, False, False)
                self._limon_api.set_model_animation(self.playerAttachedModelID, "Shooting|", False)
                self._limon_api.set_model_animation_speed(self.playerAttachedModelID, 1.5)
                muzzleFlashOffset = limon.Vec4(0.010, 0.173, 0.844, 0.0)
                scale = limon.Vec4(0.25,0.25,0.25, 0.0)
                direction = limon.Vec4(0,1,0,0)
                muzzleFlashObjectId = self._limon_api.add_object("./Data/Models/Muzzle/Muzzle.obj", 0, False, muzzleFlashOffset, scale, direction)
                isAttached = self._limon_api.attach_object_to_object(muzzleFlashObjectId, self.playerAttachedPistolId)
                if(not isAttached):
                    print("attachment failed")
                else:
                    print("attachment success ", muzzleFlashObjectId)
                #now setup removal
                param = GenericParameter(
                    request_type=RequestParameterType.FREE_NUMBER,
                    description="remove id",
                    value_type=ValueType.LONG,
                    value=muzzleFlashObjectId,
                    is_set=True
                )
                param_vector = [param]  # Python list - binding handles automatic conversion to C++
                self._limon_api.add_timed_event(
                    wait_time=500,  # 0.5 second
                    use_wall_time=True,
                    callback=self.remove_muzzle_flash,
                    parameters=param_vector
                )
                # Add muzzle flash light at player position
                light_position = limon.Vec4(0.0, 2.0, 0.0, 0.0)
                if self.playerAttachedModelID != -1:
                    transform = self._limon_api.get_object_transformation(self.playerAttachedModelID)
                    if transform and len(transform) > 0:
                        pos = transform[0].value
                        light_position = limon.Vec4(pos[0], pos[1], pos[2], 0.0)
                muzzle_light_id = self._limon_api.add_light(2, light_position, limon.Vec4(1.0, 0.6, 0.1, 1.0))
                if muzzle_light_id != 0:
                    light_param = GenericParameter(
                        request_type=RequestParameterType.FREE_NUMBER,
                        description="light id",
                        value_type=ValueType.LONG,
                        value=muzzle_light_id,
                        is_set=True
                    )
                    self._limon_api.add_timed_event(
                        wait_time=600,
                        use_wall_time=False,
                        callback=self.remove_muzzle_flash_light,
                        parameters=[light_param]
                    )

                ray_result = self._limon_api.ray_cast_to_cursor()
                if ray_result:
                    print("Ray cast result: ", ray_result)
                    if len(ray_result) == 4:
                        print("Hit AI")
                        # ray_result[3] should be a Python GenericParameter with the AI ID
                        ai_id_param = ray_result[3]
                        print(f"AI param type: {type(ai_id_param)}")
                        print(f"AI param description: '{ai_id_param.description}'")
                        print(f"AI param value_type: {ai_id_param.value_type}")
                        if ai_id_param.is_long():
                            ai_id = ai_id_param.get_long()
                            print(f"AI ID found: {ai_id}")
                            interaction_parameter = GenericParameter(
                                request_type=RequestParameterType.FREE_TEXT,
                                description="hit interaction",
                                value_type=ValueType.STRING,
                                value="GOT_HIT",
                                is_set=True
                            )
                            self._limon_api.interact_with_ai(ai_id, [interaction_parameter])
                else:
                    print("No ray cast result")

        except Exception as e:
            print(f"Error in run method: {type(e).__name__}: {e}")
            import traceback
            traceback.print_exc()
        return True

    def remove_muzzle_flash(self, parameters):
        """
        Callback function to remove muzzle flash.

        Args:
            parameters: List of GenericParameter objects passed from the timer
        """
        try:
            # Check if parameters exist and are valid
            if not parameters or not hasattr(parameters, '__getitem__') or len(parameters) == 0:
                print("Error: Invalid or empty parameters in remove_muzzle_flash")
                return
            param = parameters[0]
            if not hasattr(param, 'is_set') or not param.is_set:
                print("Error: Parameter is not properly set")
                return
            # Get the muzzle flash ID
            muzzle_flash_id = int(param.value)
            print(f"Removing muzzle flash with ID: {muzzle_flash_id}")
            # Add your code to remove the muzzle flash object here
            # For example:
            # if muzzle_flash_id:
            self._limon_api.remove_object(muzzle_flash_id, True)

        except Exception as e:
            print(f"Error in remove_muzzle_flash: {str(e)}")
            import traceback
            traceback.print_exc()
    def remove_muzzle_flash_light(self, parameters):
        try:
            if not parameters or len(parameters) == 0:
                return
            param = parameters[0]
            light_id = int(param.value)
            self._limon_api.remove_light(light_id)
        except Exception as e:
            print(f"Error in remove_muzzle_flash_light: {str(e)}")
            import traceback
            traceback.print_exc()

    def interact(self, interaction_data: List['GenericParameter']) -> None:
        """Handle interaction with the player."""
        pass

    def get_name(self) -> str:
        """Get the name of this extension."""
        return self.__class__.__name__

    def get_custom_camera_attachment(self) -> Optional[limon.CameraAttachment]:
        """Get a custom camera attachment if this extension provides one."""
        return self.camera_instance if hasattr(self, 'camera_instance') and self.camera_instance is not None else None

    def get_name(self):
        return "PythonPlayerExtension"
