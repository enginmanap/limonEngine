import limon
from camera_extension_interface import CameraExtensionInterface
from generic_parameter import GenericParameter, RequestParameterType, ValueType
from vec3 import Vec3


class PythonOrthographicCameraRig(CameraExtensionInterface):
    """
    A Python orthographic camera rig - an isometric / top-down view that follows the player. It is the
    Python twin of the C++ OrthographicCameraRig sample, and demonstrates full Python/C++ parity for the
    registered camera-rig system: it is discovered by class name, appears in the editor's Cameras tree,
    is configurable, and drives an orthographic player camera.

    It is UNATTACHED - it produces its own pose by querying the player position through the API every frame
    (so it does not need a CameraRig parent).
    """

    def __init__(self, limon_api):
        super().__init__(limon_api)
        self.offset = Vec3(0.0, 20.0, 20.0)
        self.half_height = 15.0
        self.near_plane = 0.1
        self.far_plane = 1000.0

    def get_name(self) -> str:
        return "PythonOrthographicCameraRig"

    def get_parameters(self):
        parameters = []
        descriptions = ["Offset X", "Offset Y", "Offset Z", "Zoom (half height)", "Near plane", "Far plane"]
        values = [self.offset.x, self.offset.y, self.offset.z, self.half_height, self.near_plane, self.far_plane]
        for description, value in zip(descriptions, values):
            parameters.append(GenericParameter(RequestParameterType.FREE_NUMBER, description,
                                               ValueType.DOUBLE, float(value), True))
        return parameters

    def set_parameters(self, parameters):
        if len(parameters) > 5:
            self.offset = Vec3(float(parameters[0].value), float(parameters[1].value), float(parameters[2].value))
            self.half_height = float(parameters[3].value)
            self.near_plane = float(parameters[4].value)
            self.far_plane = float(parameters[5].value)

    def get_camera_variables(self, position, center, up, right):
        # The player's own eye pose (independent of whichever camera is active). get_player_position returns
        # (position, view_direction, up, right); each may arrive as a Vec3-like or a {x,y,z} dict.
        (player_pos, _view_direction, _player_up, _player_right) = self.limon_api.get_player_position()
        px = player_pos.x if hasattr(player_pos, 'x') else player_pos['x']
        py = player_pos.y if hasattr(player_pos, 'y') else player_pos['y']
        pz = player_pos.z if hasattr(player_pos, 'z') else player_pos['z']

        camera_x = px + self.offset.x
        camera_y = py + self.offset.y
        camera_z = pz + self.offset.z

        # Position: above/behind the player.
        position.x, position.y, position.z = camera_x, camera_y, camera_z

        # Center: look toward the player (a direction vector).
        dir_x = px - camera_x
        dir_y = py - camera_y
        dir_z = pz - camera_z
        length = max((dir_x * dir_x + dir_y * dir_y + dir_z * dir_z) ** 0.5, 1e-6)
        center.x, center.y, center.z = dir_x / length, dir_y / length, dir_z / length

        # Up is world-up; right is derived from center x up.
        up.x, up.y, up.z = 0.0, 1.0, 0.0
        right.x = center.y * up.z - center.z * up.y
        right.y = center.z * up.x - center.x * up.z
        right.z = center.x * up.y - center.y * up.x

    def get_projection(self):
        projection = limon.ProjectionParameters()
        projection.type = limon.ProjectionType.ORTHOGRAPHIC
        projection.orthographic_half_height = self.half_height
        projection.near_plane = self.near_plane
        projection.far_plane = self.far_plane
        return projection
