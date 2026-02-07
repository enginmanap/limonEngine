"""
Python Vec3 class for 3D vector operations with mathematical methods.
This class interfaces with C++ glm::vec3 through pybind11.
"""

import math
from typing import Union, Tuple


class Vec3:
    """3D vector class with mathematical operations"""
    
    def __init__(self, x: float = 0.0, y: float = 0.0, z: float = 0.0):
        self.x = float(x)
        self.y = float(y)
        self.z = float(z)
    
    def __repr__(self) -> str:
        return f"Vec3({self.x}, {self.y}, {self.z})"
    
    def __str__(self) -> str:
        return f"({self.x}, {self.y}, {self.z})"
    
    # Equality operators
    def __eq__(self, other: 'Vec3') -> bool:
        if not isinstance(other, Vec3):
            return False
        return (abs(self.x - other.x) < 1e-6 and 
                abs(self.y - other.y) < 1e-6 and 
                abs(self.z - other.z) < 1e-6)
    
    def __ne__(self, other: 'Vec3') -> bool:
        return not self.__eq__(other)
    
    # Addition
    def __add__(self, other: Union['Vec3', Tuple[float, float, float]]) -> 'Vec3':
        if isinstance(other, Vec3):
            return Vec3(self.x + other.x, self.y + other.y, self.z + other.z)
        elif isinstance(other, (tuple, list)) and len(other) >= 3:
            return Vec3(self.x + other[0], self.y + other[1], self.z + other[2])
        else:
            raise TypeError("Operand must be Vec3 or tuple/list of 3 floats")
    
    def __radd__(self, other: Union['Vec3', Tuple[float, float, float]]) -> 'Vec3':
        return self.__add__(other)
    
    def __iadd__(self, other: Union['Vec3', Tuple[float, float, float]]) -> 'Vec3':
        result = self.__add__(other)
        self.x, self.y, self.z = result.x, result.y, result.z
        return self
    
    # Subtraction
    def __sub__(self, other: Union['Vec3', Tuple[float, float, float]]) -> 'Vec3':
        if isinstance(other, Vec3):
            return Vec3(self.x - other.x, self.y - other.y, self.z - other.z)
        elif isinstance(other, (tuple, list)) and len(other) >= 3:
            return Vec3(self.x - other[0], self.y - other[1], self.z - other[2])
        else:
            raise TypeError("Operand must be Vec3 or tuple/list of 3 floats")
    
    def __rsub__(self, other: Union['Vec3', Tuple[float, float, float]]) -> 'Vec3':
        if isinstance(other, Vec3):
            return Vec3(other.x - self.x, other.y - self.y, other.z - self.z)
        elif isinstance(other, (tuple, list)) and len(other) >= 3:
            return Vec3(other[0] - self.x, other[1] - self.y, other[2] - self.z)
        else:
            raise TypeError("Operand must be Vec3 or tuple/list of 3 floats")
    
    def __isub__(self, other: Union['Vec3', Tuple[float, float, float]]) -> 'Vec3':
        result = self.__sub__(other)
        self.x, self.y, self.z = result.x, result.y, result.z
        return self
    
    # Scalar multiplication and division
    def __mul__(self, scalar: float) -> 'Vec3':
        return Vec3(self.x * scalar, self.y * scalar, self.z * scalar)
    
    def __rmul__(self, scalar: float) -> 'Vec3':
        return self.__mul__(scalar)
    
    def __imul__(self, scalar: float) -> 'Vec3':
        self.x *= scalar
        self.y *= scalar
        self.z *= scalar
        return self
    
    def __truediv__(self, scalar: float) -> 'Vec3':
        if scalar == 0:
            raise ValueError("Cannot divide by zero")
        return Vec3(self.x / scalar, self.y / scalar, self.z / scalar)
    
    def __itruediv__(self, scalar: float) -> 'Vec3':
        if scalar == 0:
            raise ValueError("Cannot divide by zero")
        self.x /= scalar
        self.y /= scalar
        self.z /= scalar
        return self
    
    # Negation
    def __neg__(self) -> 'Vec3':
        return Vec3(-self.x, -self.y, -self.z)
    
    # Length and normalization
    def length(self) -> float:
        """Return the magnitude (length) of the vector"""
        return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)
    
    def length_squared(self) -> float:
        """Return the squared length (more efficient than length() for comparisons)"""
        return self.x * self.x + self.y * self.y + self.z * self.z
    
    def normalized(self) -> 'Vec3':
        """Return a normalized copy of this vector"""
        length = self.length()
        if length == 0:
            return Vec3(0, 0, 0)
        return Vec3(self.x / length, self.y / length, self.z / length)
    
    def normalize(self) -> 'Vec3':
        """Normalize this vector in place and return it"""
        length = self.length()
        if length != 0:
            self.x /= length
            self.y /= length
            self.z /= length
        else:
            self.x, self.y, self.z = 0, 0, 0
        return self
    
    # Mathematical operations
    def dot(self, other: 'Vec3') -> float:
        """Return the dot product with another vector"""
        if not isinstance(other, Vec3):
            raise TypeError("Operand must be Vec3")
        return self.x * other.x + self.y * other.y + self.z * other.z
    
    def cross(self, other: 'Vec3') -> 'Vec3':
        """Return the cross product with another vector"""
        if not isinstance(other, Vec3):
            raise TypeError("Operand must be Vec3")
        return Vec3(
            self.y * other.z - self.z * other.y,
            self.z * other.x - self.x * other.z,
            self.x * other.y - self.y * other.x
        )
    
    # Utility methods
    def to_tuple(self) -> Tuple[float, float, float]:
        """Convert to tuple representation"""
        return (self.x, self.y, self.z)
    
    def to_list(self) -> list:
        """Convert to list representation"""
        return [self.x, self.y, self.z]
    
    @classmethod
    def from_tuple(cls, t: Tuple[float, float, float]) -> 'Vec3':
        """Create Vec3 from tuple"""
        return cls(t[0], t[1], t[2])
    
    @classmethod
    def from_list(cls, l: list) -> 'Vec3':
        """Create Vec3 from list"""
        return cls(l[0], l[1], l[2])
    
    # Common vector constants
    @classmethod
    def zero(cls) -> 'Vec3':
        """Zero vector (0, 0, 0)"""
        return cls(0, 0, 0)
    
    @classmethod
    def one(cls) -> 'Vec3':
        """One vector (1, 1, 1)"""
        return cls(1, 1, 1)
    
    @classmethod
    def up(cls) -> 'Vec3':
        """Up vector (0, 1, 0)"""
        return cls(0, 1, 0)
    
    @classmethod
    def right(cls) -> 'Vec3':
        """Right vector (1, 0, 0)"""
        return cls(1, 0, 0)
    
    @classmethod
    def forward(cls) -> 'Vec3':
        """Forward vector (0, 0, 1)"""
        return cls(0, 0, 1)
    
    @staticmethod
    def from_generic_parameter(param) -> 'Vec3':
        """
        Create a Vec3 from a GenericParameter.
        
        Args:
            param: GenericParameter object that should contain VEC4 data
            
        Returns:
            Vec3 object with the first 3 components of the VEC4
            
        Raises:
            ValueError: If the GenericParameter is not a VEC4 type
        """
        if not hasattr(param, 'is_vec4') or not param.is_vec4():
            raise ValueError(f"GenericParameter is not a VEC4, it's {param.value_type.name}")
        
        vec4 = param.get_vec4()
        return Vec3(vec4[0], vec4[1], vec4[2])
    
    @staticmethod
    def from_generic_parameters(params) -> list:
        """
        Convert a list of GenericParameters to Vec3 objects.
        
        Args:
            params: List of GenericParameter objects
            
        Returns:
            List of Vec3 objects for VEC4 parameters, None for others
            
        Example:
            vec3_list = Vec3.from_generic_parameters(generic_params)
        """
        vec3_list = []
        for param in params:
            try:
                vec3 = Vec3.from_generic_parameter(param)
                vec3_list.append(vec3)
            except ValueError:
                # Skip non-VEC4 parameters
                vec3_list.append(None)
        return vec3_list
