from camera_attachment import CameraAttachment
from typing import List, Dict, Any, Optional, Tuple, Union
import limon
import math
from generic_parameter import GenericParameter, ValueType, RequestParameterType
from vec3 import Vec3


"""
    This is an example of a camera attachment implementation through Python.
    
"""
class ThirdPersonCamera:
    def __init__(self, limon_api):
        self.limon_api = limon_api
        self._dirty = True
        self.distance = 5.0  # Distance from target
        self.height = 2.0    # Height above target
        self.target = Vec3(0.0, 0.0, 0.0)  # Look at origin by default
        self.up = Vec3(0.0, 1.0, 0.0)     # World up vector
        self._raycast_line_buffer = 0      # Debug line buffer ID; 0 = none

    def isDirty(self) -> bool:  # Note: Must match C++ method name exactly
        """Return True if the camera parameters have changed."""
        return self._dirty

    def clearDirty(self) -> None:  # Note: Must match C++ method name exactly
        """Mark the camera parameters as clean."""
        self._dirty = True

    def getCameraVariables(self, position: Vec3, center: Vec3, up: Vec3, right: Vec3):
        try:
            # Get player position and orientation
            (player_pos,
             view_direction,
             player_up,
             player_right) = self.limon_api.get_player_position()

            # Convert Vec3 objects to Vec3 for calculations (they should already be Vec3)
            player_pos_vec = Vec3(player_pos.x, player_pos.y, player_pos.z) if hasattr(player_pos, 'x') else Vec3(player_pos['x'], player_pos['y'], player_pos['z'])
            view_dir_vec = Vec3(view_direction.x, view_direction.y, view_direction.z) if hasattr(view_direction, 'x') else Vec3(view_direction['x'], view_direction['y'], view_direction['z'])
            player_up_vec = Vec3(player_up.x, player_up.y, player_up.z) if hasattr(player_up, 'x') else Vec3(player_up['x'], player_up['y'], player_up['z'])
            player_right_vec = Vec3(player_right.x, player_right.y, player_right.z) if hasattr(player_right, 'x') else Vec3(player_right['x'], player_right['y'], player_right['z'])

            # Set camera position (slightly behind player in view direction)
            camera_distance = 5.0  # Original distance
            camera_height_offset = 1.5  # Original height offset
            camera_pos = player_pos_vec - view_dir_vec * camera_distance + Vec3(0, camera_height_offset, 0)

            # Create Vec4 objects for position and direction
            start_pos = limon.Vec4()
            start_pos.x = player_pos_vec.x
            start_pos.y = player_pos_vec.y
            start_pos.z = player_pos_vec.z
            start_pos.w = 0.0

            # Calculate desired camera position (behind the player)
            desired_pos = limon.Vec4()
            desired_pos.x = camera_pos.x
            desired_pos.y = camera_pos.y
            desired_pos.z = camera_pos.z
            desired_pos.w = 0.0

            # Calculate direction vector for raycast (from player to camera)
            dir_vec = limon.Vec4()
            dir_vec.x = desired_pos.x - start_pos.x
            dir_vec.y = desired_pos.y - start_pos.y
            dir_vec.z = desired_pos.z - start_pos.z
            dir_vec.w = 0.0

            # Perform raycast
            hit_details = self.limon_api.ray_cast_first_hit(start_pos, dir_vec)

            hit_blocked = False

            # hit_details already contains GenericParameter objects from C++
            for detail in hit_details:
                if detail.description == "hit coordinates" and detail.is_vec4():
                    # we hit something, but is that thing further away than camera?
                    hit_coords_vec = Vec3.from_generic_parameter(detail)

                    start_pos_vec = Vec3(start_pos.x, start_pos.y, start_pos.z)
                    desired_pos_vec = Vec3(desired_pos.x, desired_pos.y, desired_pos.z)

                    hit_distance_sq = (hit_coords_vec - start_pos_vec).length_squared()
                    camera_distance_sq = (desired_pos_vec - start_pos_vec).length_squared()

                    if hit_distance_sq < camera_distance_sq:
                        dir_vec_normalized = Vec3(dir_vec.x, dir_vec.y, dir_vec.z).normalized()
                        desired_pos.x = hit_coords_vec.x - dir_vec_normalized.x * 0.1  # 10% buffer so camera won't clip
                        desired_pos.y = hit_coords_vec.y - dir_vec_normalized.y * 0.1
                        desired_pos.z = hit_coords_vec.z - dir_vec_normalized.z * 0.1
                        hit_blocked = True

            # --- Debug line drawing: visualise the camera raycast ---
            # Clear the previous frame's lines first
            if self._raycast_line_buffer != 0:
                self.limon_api.clear_debug_lines(self._raycast_line_buffer)
                self._raycast_line_buffer = 0

            # Look-at point: camera position + view direction scaled to camera_distance
            look_at_x = desired_pos.x + view_dir_vec.x * camera_distance
            look_at_y = desired_pos.y + view_dir_vec.y * camera_distance
            look_at_z = desired_pos.z + view_dir_vec.z * camera_distance

            cam_v4    = limon.Vec4(start_pos.x, start_pos.y, start_pos.z, 1.0)
            lookat_v4 = limon.Vec4(look_at_x,     look_at_y,     look_at_z,     1.0)

            if hit_blocked:
                # Green: camera → look-at point (ray was clipped, show normal view line)
                green = limon.Vec4(0.0, 1.0, 0.0, 1.0)
                self._raycast_line_buffer = self.limon_api.draw_debug_line(
                    cam_v4, lookat_v4, green, green, True)
                # Red: clipped camera position → original desired position (blocked portion)
                original_v4 = limon.Vec4(camera_pos.x, camera_pos.y, camera_pos.z, 1.0)
                red = limon.Vec4(1.0, 0.0, 0.0, 1.0)
                self.limon_api.add_to_debug_line(
                    self._raycast_line_buffer,
                    cam_v4, original_v4, red, red, True)
            else:
                # White: camera → look-at point (unobstructed)
                white = limon.Vec4(1.0, 1.0, 1.0, 1.0)
                self._raycast_line_buffer = self.limon_api.draw_debug_line(
                    cam_v4, lookat_v4, white, white, True)
            # --------------------------------------------------------

            # Update the Vec3 objects directly
            position.x = float(desired_pos.x)
            position.y = float(desired_pos.y)
            position.z = float(desired_pos.z)

            center.x = float(view_dir_vec.x)
            center.y = float(view_dir_vec.y)
            center.z = float(view_dir_vec.z)

            up.x = float(player_up_vec.x)
            up.y = float(player_up_vec.y)
            up.z = float(player_up_vec.z)

            right.x = float(player_right_vec.x)
            right.y = float(player_right_vec.y)
            right.z = float(player_right_vec.z)

        except Exception as e:
            print(f"Error in getCameraVariables: {str(e)}")
            import traceback
            traceback.print_exc()
        return None
