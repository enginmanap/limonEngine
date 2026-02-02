from typing import Dict, Any, Optional
from camera_attachment import CameraAttachment


class PlayerExtensionInterface:

    def __init__(self, limon_api):
        self.limon_api = limon_api

    """
    Base class for player extensions.
    Implement this to create custom player behaviors.
    """
    def process_input(self, input_state: Dict[str, Any]) -> None:
        """
        provides the input state to the player extension from the engine.
        Use it based on the player implementation requirements.
        Args:
            input_state: Dictionary containing the current input state
        """
        raise NotImplementedError("process_input() not implemented")

    def interact(self, target: Any) -> bool:
        """
        Some other actor is interacted with the player. Process accordingly.
        Actors will be using actor_interface so the data passed is going to be defined there.
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

    def get_custom_camera_attachment(self) -> Optional[CameraAttachment]:
        """
        Returns the camera attachment for this player extension. If no camera attachment is returned, the default camera is first person.
        Returns:
            Optional[CameraAttachment]: A CameraAttachment instance, or None to use default camera
        """
        return None
