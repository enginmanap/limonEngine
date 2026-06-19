from typing import List

import limon
from generic_parameter import GenericParameter
from vec3 import Vec3


class CameraExtensionInterface:
    """
    Base class for a registered camera rig - the Python mirror of C++ CameraExtensionInterface.

    Subclass this in a script under Engine/Scripts or Data/Scripts and the engine auto-discovers it by
    class name, exactly like Triggers, Actors, and Player Extensions. The rig then appears in the editor's
    Cameras tree, is configurable through get_parameters / set_parameters, supports perspective or
    orthographic projection, and can follow an object via the engine's attachment system - full parity
    with C++ rigs.
    """

    def __init__(self, limon_api):
        self.limon_api = limon_api
        self._dirty = True

    def get_name(self) -> str:
        """Registered type name of this rig (shown in the editor). Must be unique."""
        raise NotImplementedError("get_name() not implemented")

    def get_parameters(self) -> List[GenericParameter]:
        """Configurable parameters of this rig - drives editor editing and world-file round-trip."""
        return []

    def set_parameters(self, parameters: List[GenericParameter]) -> None:
        """Apply edited or loaded parameter values."""
        pass

    def is_dirty(self) -> bool:
        """
        Return True when the camera pose changed since the last frame. A follow camera typically returns
        True every frame. The engine clears it via clear_dirty - do not clear it yourself.
        """
        return self._dirty

    def clear_dirty(self) -> None:
        """The engine marks the pose consumed."""
        self._dirty = False

    def get_camera_variables(self, position: Vec3, center: Vec3, up: Vec3, right: Vec3) -> None:
        """
        Fill the camera pose in place. Write to the .x/.y/.z of the supplied Vec3 objects: position (camera
        position), center (look-at direction), up, and right.
        """
        raise NotImplementedError("get_camera_variables() not implemented")

    def get_projection(self) -> "limon.ProjectionParameters":
        """
        Projection the engine should build. Default is a standard perspective camera. For orthographic, set
        params.type = limon.ProjectionType.ORTHOGRAPHIC and params.orthographic_half_height (the zoom).
        """
        return limon.ProjectionParameters()

    def set_attachment_transform(self, position: Vec3, orientation, scale: Vec3) -> None:
        """
        Optional. When this rig's CameraRig is attached to a world object, the engine calls this each frame
        (before get_camera_variables) with the target's resolved world transform: position (Vec3),
        orientation (a quaternion with .x/.y/.z/.w), and scale (Vec3). Unattached rigs never receive it and
        should produce their own pose in get_camera_variables instead.
        """
        pass
