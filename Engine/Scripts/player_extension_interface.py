from typing import List
from generic_parameter import GenericParameter


class PlayerExtensionInterface:
    """
    Base class for player extensions.
    Implement this to create custom player behaviors.
    """

    def __init__(self, limon_api):
        self.limon_api = limon_api

    def process_input(self, input_states, player_information, time: int) -> None:
        """
        Called every tick with the current input.
        Args:
            input_states: limon.InputStates, query with get_input_status / get_input_events / get_analog_value
            player_information: limon.PlayerInformation, position and look_direction as limon.Vec4
            time: Current game time in milliseconds
        """
        raise NotImplementedError("process_input() not implemented")

    def interact(self, interaction_data: List[GenericParameter]) -> None:
        """
        Some other actor interacted with the player. Process accordingly.
        Args:
            interaction_data: List of parameters describing the interaction, defined by the interacting actor
        """
        raise NotImplementedError("interact() not implemented")

    def get_name(self) -> str:
        """
        Get the name of this extension.
        Returns:
            str: The extension's name
        """
        raise NotImplementedError("get_name() not implemented")

    def get_parameters(self) -> List[GenericParameter]:
        """
        Default parameters of this extension, shown in the editor and saved with the world. Called only
        once, when the extension is created; the engine keeps the edited or loaded values and hands them
        to set_parameters().
        Returns:
            List[GenericParameter]: List of extension parameters
        """
        return []

    def set_parameters(self, parameters: List[GenericParameter]) -> None:
        """
        Apply edited or loaded parameter values.
        Args:
            parameters: List of parameters to set
        """
        pass
