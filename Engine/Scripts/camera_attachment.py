from typing import Tuple


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
        Return true if data changed. True will trigger both get_camera_variables and clear_dirty. But also
        internal processes, like culling.
        Don't automatically set the dirty flag to false, it is done by the engine calling clear_dirty.
        Returns:
            bool: True if the camera state has changed, False otherwise.
        """
        raise NotImplementedError("is_dirty() not implemented")

    def clear_dirty(self) -> None:
        """
        Set the dirty flag to false.
        This is called after the camera state has been processed fully for the engine.
        """
        raise NotImplementedError("clear_dirty() not implemented")

    def get_camera_variables(self) -> Tuple[Tuple[float, float, float],  # position
    Tuple[float, float, float],  # target
    Tuple[float, float, float],  # up
    Tuple[float, float, float]]:  # right
        """
        Get the camera's view vectors.
        Returns:
            tuple: (position, target, up, right) as (x,y,z) tuples
        """
        raise NotImplementedError("get_camera_variables() not implemented")
