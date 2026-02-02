from typing import Dict, Any, List, Optional
from generic_parameter import GenericParameter


class ActorInterface:
    """
    Base class for actors in the game world.
    Implement this to create custom actor behaviors.
    """

    def __init__(self, limon_api):
        self.limon_api = limon_api

    def get_name(self) -> str:
        """
        Get the name of this actor class. Actors of same class would differentiate with the parameters set.
        Returns:
            str: The actor's name
        """
        raise NotImplementedError("get_name() not implemented")

    def play(self, time: int, actor_information: Dict[str, Any]) -> None:
        """
        Called as update function for the actor. Check Actor Information on python API documentation for details.
        Args:
            time: Current game time
            actor_information: Information about the actor's state and environment
        """
        raise NotImplementedError("play() not implemented")

    def interaction(self, interaction_data: List[GenericParameter]) -> bool:
        """
        Handle interaction with this actor. It is likely to be from the player.
        Args:
            interaction_data: List of parameters describing the interaction
        Returns:
            bool: True if the interaction was successful
        """
        raise NotImplementedError("interaction() not implemented")

    def get_parameters(self) -> List[GenericParameter]:
        """
        Get the current parameters of this actor.
        Returns:
            List[GenericParameter]: List of actor parameters
        """
        return []

    def set_parameters(self, parameters: List[GenericParameter]) -> None:
        """
        Set the parameters of this actor.
        Args:
            parameters: List of parameters to set
        """
        pass
