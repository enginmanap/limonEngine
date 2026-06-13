from typing import Dict, Any, List, Optional
from generic_parameter import GenericParameter
from vec3 import Vec3


class InformationRequest:
    """
    Python mirror of C++ ActorInterface::InformationRequest.

    Inside play(), set route_to_player (or route_to_custom_position together with custom_position)
    to ask the engine to compute a navigation route. The engine consumes the request once per tick,
    exactly like the C++ getRequests() path.
    """

    def __init__(self):
        self.route_to_player = False
        self.route_to_custom_position = False
        self.custom_position = Vec3(0.0, 0.0, 0.0)


class ActorInterface:
    """
    Base class for actors in the game world.
    Implement this to create custom actor behaviors.
    """

    def __init__(self, actor_id: int, limon_api):
        # actor_id is the actor's own engine-assigned world ID (mirrors C++ worldID).
        self.actor_id = actor_id
        self.limon_api = limon_api
        self.information_request = InformationRequest()
        # model_id is the world object ID of the model this actor drives (mirrors C++ modelID). It
        # differs from actor_id. The engine writes it directly onto this attribute after binding the
        # model; it stays 0 until then.
        self.model_id = 0

    def get_model_id(self) -> int:
        """Return the world object ID of the model this actor drives. Mirrors C++ getModelID()."""
        return self.model_id

    def get_world_id(self) -> int:
        """Return the actor's own engine-assigned ID. Mirrors C++ getWorldID()."""
        return self.actor_id

    def get_position(self) -> Vec3:
        """
        World position of the driven model. Mirrors C++ ActorInterface::getPosition(); both forward to
        the same engine implementation (LimonAPI.get_object_position), so no logic is duplicated here.
        """
        position = self.limon_api.get_object_position(self.model_id)  # limon.Vec4
        return Vec3(position.x, position.y, position.z)

    def get_front_vector(self) -> Vec3:
        """
        Forward direction of the driven model. Mirrors C++ ActorInterface::getFrontVector(); both
        forward to the same engine implementation (LimonAPI.get_object_front_vector).
        """
        front = self.limon_api.get_object_front_vector(self.model_id)  # limon.Vec4
        return Vec3(front.x, front.y, front.z)

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
