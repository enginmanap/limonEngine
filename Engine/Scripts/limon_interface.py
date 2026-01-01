# limon_interface.py
from typing import List, Dict, Any, Optional, Tuple

class CameraAttachment:
    def __init__(self, limon_api):
        self.limon_api = limon_api
        self._dirty = True

    """
    Base class for custom camera attachments in the game.
    Implement this to create custom camera behaviors.
    """
    def is_dirty(self) -> bool:
        """
        Check if the camera state has changed and needs to be updated.
        Returns:
            bool: True if the camera state has changed, False otherwise.
        """
        raise NotImplementedError("is_dirty() not implemented")

    def clear_dirty(self) -> None:
        """
        Mark the camera state as clean (updated).
        This is called after the camera state has been processed.
        """
        raise NotImplementedError("clear_dirty() not implemented")

    def get_camera_variables(self) -> Tuple[Tuple[float, float, float],  # position
    Tuple[float, float, float],  # target
    Tuple[float, float, float],  # up
    Tuple[float, float, float]]:  # right
        """
        Get the camera's current state.
        Returns:
            tuple: (position, target, up, right) as (x,y,z) tuples
        """
        raise NotImplementedError("get_camera_variables() not implemented")


class TriggerInterface:
    """
    Base class for game triggers.
    Implement this to create custom trigger behaviors.
    """
    def get_parameters(self) -> List[Dict[str, Any]]:
        """
        Get the parameters required by this trigger.
        Returns:
            List[Dict[str, Any]]: List of parameter definitions
        """
        raise NotImplementedError("get_parameters() not implemented")

    def run(self, parameters: Dict[str, Any]) -> bool:
        """
        Execute the trigger's action.
        Args:
            parameters: Dictionary of parameter values
        Returns:
            bool: True if the trigger executed successfully, False otherwise
        """
        raise NotImplementedError("run() not implemented")

    def get_results(self) -> Dict[str, Any]:
        """
        Get any results from the trigger's execution.
        Returns:
            Dict[str, Any]: Dictionary of result values
        """
        raise NotImplementedError("get_results() not implemented")

    def get_name(self) -> str:
        """
        Get the name of this trigger type.
        Returns:
            str: The trigger's name
        """
        raise NotImplementedError("get_name() not implemented")


class PlayerExtensionInterface:

    def __init__(self, limon_api):
        self.limon_api = limon_api

    """
    Base class for player extensions.
    Implement this to create custom player behaviors.
    """
    def process_input(self, input_state: Dict[str, Any]) -> None:
        """
        Process player input.
        Args:
            input_state: Dictionary containing the current input state
        """
        raise NotImplementedError("process_input() not implemented")

    def interact(self, target: Any) -> bool:
        """
        Handle player interaction with an object.
        Args:
            target: The object being interacted with
        Returns:
            bool: True if the interaction was successful
        """
        raise NotImplementedError("interact() not implemented")

    def get_name(self) -> str:
        """
        Get the name of this extension.
        Returns:
            str: The extension's name
        """
        raise NotImplementedError("get_name() not implemented")

    def get_custom_camera_attachment(self) -> Optional['CameraAttachment']:
        """
        Get a custom camera attachment for this extension.
        Returns:
            Optional[CameraAttachment]: A CameraAttachment instance, or None to use default camera
        """
        return None