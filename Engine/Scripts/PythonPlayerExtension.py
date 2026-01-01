# limonimp.py
import limon
from typing import List, Dict, Any, Optional, Tuple
from PythonThirdPersonCamera import ThirdPersonCamera

class PythonPlayerExtension(limon.PlayerExtensionInterface):
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
                param = limon.GenericParameter()
                param.request_type = limon.RequestParameterType.FREE_NUMBER
                param.value_type = limon.ValueType.LONG
                param.description = "remove id"
                param.is_set = True
                param.value = muzzleFlashObjectId
                param_vector = limon.GenericParameterVector()
                param_vector.append(param)
                self._limon_api.add_timed_event(
                    wait_time=500,  # 0.5 second
                    use_wall_time=True,
                    callback=self.remove_muzzle_flash,
                    parameters=param_vector
                )
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
    def interact(self, interaction_data: List['limon.GenericParameter']) -> None:
        """Handle interaction with the player."""
        pass

    def get_name(self) -> str:
        """Get the name of this extension."""
        return self.__class__.__name__

    def get_custom_camera_attachment(self) -> Optional[limon.CameraAttachment]:
        """Get a custom camera attachment if this extension provides one."""
        return self.camera_instance if hasattr(self, 'camera_instance') and self.camera_instance is not None else None

    def get_parameters(self):
        param = limon.GenericParameter()
        param.request_type = limon.RequestParameterType.FREE_TEXT
        param.description = "Test trigger"
        param.value = "Hello"
        return [param]

    def get_name(self):
        return "PythonPlayerExtension"
