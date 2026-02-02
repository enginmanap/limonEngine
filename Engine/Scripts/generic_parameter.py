"""
Python wrapper for LimonTypes::GenericParameter
"""
from typing import Union, List, Tuple, Any
from limon import ValueType, RequestParameterType


class GenericParameter:
    """Python wrapper for C++ GenericParameter struct
       This is used for parameters that are accessible on the editor interface.
       It contains a type and a value for the runtime usage, but also contains
       editor related data, like description, request type and is_set
    """
    
    def __init__(self, request_type: RequestParameterType = RequestParameterType.FREE_NUMBER,
                 description: str = "", value_type: ValueType = ValueType.VEC4,
                 value: Any = None, is_set: bool = False):
        self.request_type = request_type
        self.description = description
        self.value_type = value_type
        self.value = value
        self.is_set = is_set
    
    def __repr__(self) -> str:
        return f"GenericParameter(description='{self.description}', type={self.value_type.name}, value={self.value})"
    
    def is_string(self) -> bool:
        return self.value_type == ValueType.STRING
    
    def is_double(self) -> bool:
        return self.value_type == ValueType.DOUBLE
    
    def is_long(self) -> bool:
        return self.value_type == ValueType.LONG
    
    def is_boolean(self) -> bool:
        return self.value_type == ValueType.BOOLEAN
    
    def is_vec4(self) -> bool:
        return self.value_type == ValueType.VEC4
    
    def is_mat4(self) -> bool:
        return self.value_type == ValueType.MAT4
    
    def is_long_array(self) -> bool:
        return self.value_type == ValueType.LONG_ARRAY
    
    def get_string(self) -> str:
        if self.is_string():
            return self.value
        raise ValueError(f"GenericParameter is not a string, it's {self.value_type.name}")
    
    def get_double(self) -> float:
        if self.is_double():
            return self.value
        raise ValueError(f"GenericParameter is not a double, it's {self.value_type.name}")
    
    def get_long(self) -> int:
        if self.is_long():
            return self.value
        raise ValueError(f"GenericParameter is not a long, it's {self.value_type.name}")
    
    def get_boolean(self) -> bool:
        if self.is_boolean():
            return self.value
        raise ValueError(f"GenericParameter is not a boolean, it's {self.value_type.name}")
    
    def get_vec4(self) -> Tuple[float, float, float, float]:
        if self.is_vec4() and isinstance(self.value, (list, tuple)) and len(self.value) >= 4:
            return tuple(self.value[:4])
        raise ValueError(f"GenericParameter is not a valid VEC4, it's {self.value_type.name} with value {self.value}")
    
    def get_vec3(self) -> Tuple[float, float, float]:
        """Convenience method to get first 3 components of VEC4"""
        vec4 = self.get_vec4()
        return vec4[:3]
    
    def get_mat4(self) -> List[List[float]]:
        if self.is_mat4() and isinstance(self.value, list):
            return self.value
        raise ValueError(f"GenericParameter is not a valid MAT4, it's {self.value_type.name}")
    
    def get_long_array(self) -> List[int]:
        if self.is_long_array() and isinstance(self.value, list):
            return self.value
        raise ValueError(f"GenericParameter is not a valid LONG_ARRAY, it's {self.value_type.name}")
