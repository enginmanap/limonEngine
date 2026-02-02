from camera_attachment import CameraAttachment
from typing import List, Dict, Any, Optional, Tuple, Union
import limon
from limon import ValueType
from generic_parameter import GenericParameter


"""
    This is an example of a camera attachment implementation through Python.
    
"""
def normalize(v):
    """Simple vector normalization for (x,y,z) tuples"""
    length = (v[0]**2 + v[1]**2 + v[2]**2) ** 0.5
    if length == 0:
        return (0.0, 0.0, 0.0)
    return (v[0]/length, v[1]/length, v[2]/length)

def cross(a, b):
    """Cross product for (x,y,z) tuples"""
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    )

class ThirdPersonCamera:
    def __init__(self, limon_api):
        self.limon_api = limon_api
        self._dirty = True
        self.distance = 5.0  # Distance from target
        self.height = 2.0    # Height above target
        self.target = (0.0, 0.0, 0.0)  # Look at origin by default
        self.up = (0.0, 1.0, 0.0)     # World up vector

    def isDirty(self) -> bool:  # Note: Must match C++ method name exactly
        """Return True if the camera parameters have changed."""
        return self._dirty

    def clearDirty(self) -> None:  # Note: Must match C++ method name exactly
        """Mark the camera parameters as clean."""
        self._dirty = True

    def getCameraVariables(self, position, center, up, right):
        try:
            # Get player position and orientation
            (player_pos,
             view_direction,
             player_up,
             player_right) = self.limon_api.get_player_position()
            # Set camera position (slightly behind player in view direction)
            camera_distance = 5.0  # Adjust as needed
            camera_pos = {
                'x': player_pos['x'] - (view_direction['x'] * camera_distance),
                'y': player_pos['y'] - (view_direction['y'] * camera_distance) + 1.5,  # Slight height offset
                'z': player_pos['z'] - (view_direction['z'] * camera_distance)
            }
            # Create Vec4 objects for position and direction
            start_pos = limon.Vec4()
            start_pos.x = player_pos['x']
            start_pos.y = player_pos['y']
            start_pos.z = player_pos['z']
            start_pos.w = 0.0
            # Calculate desired camera position (behind the player)
            desired_pos = limon.Vec4()
            desired_pos.x = float(camera_pos['x'])
            desired_pos.y = float(camera_pos['y'])
            desired_pos.z = float(camera_pos['z'])
            desired_pos.w = 0.0

            # Calculate direction vector for raycast (from player to camera)
            dir_vec = limon.Vec4()
            dir_vec.x = desired_pos.x - start_pos.x
            dir_vec.y = desired_pos.y - start_pos.y
            dir_vec.z = desired_pos.z - start_pos.z
            dir_vec.w = 0.0
            # Perform raycast
            hit_details = self.limon_api.ray_cast(start_pos, dir_vec)

            # hit_details already contains GenericParameter objects from C++
            for detail in hit_details:
                if detail.description == "hit coordinates" and detail.is_vec4():
                    # we hit something, but is that thing further away than camera?
                    hit_coords = detail.get_vec3()  # Get first 3 components
                    hit_distance_sq = (hit_coords[0] - start_pos.x) * (hit_coords[0] - start_pos.x) + (hit_coords[1] - start_pos.y) * (hit_coords[1] - start_pos.y) + (hit_coords[2] - start_pos.z) * (hit_coords[2] - start_pos.z)
                    camera_distance_sq = (desired_pos.x - start_pos.x) * (desired_pos.x - start_pos.x) + (desired_pos.y - start_pos.y) * (desired_pos.y - start_pos.y) + (desired_pos.z - start_pos.z) * (desired_pos.z - start_pos.z)
                    if hit_distance_sq < camera_distance_sq:
                        desired_pos.x = hit_coords[0] - (dir_vec.x * 0.1)  # 10% buffer so camera won't clip
                        desired_pos.y = hit_coords[1] - (dir_vec.y * 0.1)
                        desired_pos.z = hit_coords[2] - (dir_vec.z * 0.1)
            # Update output parameters by modifying the dictionaries in-place
            # Update camera position
            position.update({
                'x': float(desired_pos.x),
                'y': float(desired_pos.y),
                'z': float(desired_pos.z)
            })

            # Update center, up, and right vectors from player's orientation
            center.update({
                'x': float(view_direction['x']),
                'y': float(view_direction['y']),
                'z': float(view_direction['z'])
            })

            up.update({
                'x': float(player_up['x']),
                'y': float(player_up['y']),
                'z': float(player_up['z'])
            })

            right.update({
                'x': float(player_right['x']),
                'y': float(player_right['y']),
                'z': float(player_right['z'])
            })

        except Exception as e:
            print(f"Error in getCameraVariables: {str(e)}")
            import traceback
            traceback.print_exc()
        return None