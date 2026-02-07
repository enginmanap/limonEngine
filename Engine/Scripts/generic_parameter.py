"""
Python wrapper for LimonTypes::GenericParameter
"""
from typing import Union, List, Tuple, Any
from enum import Enum


class ValueType(Enum):
    """Python equivalent of C++ LimonTypes::GenericParameter::ValueTypes"""
    STRING = 0
    DOUBLE = 1
    LONG = 2
    LONG_ARRAY = 3
    BOOLEAN = 4
    VEC4 = 5
    MAT4 = 6


class RequestParameterType(Enum):
    """Python equivalent of C++ LimonTypes::GenericParameter::RequestParameterTypes"""
    MODEL = 0
    ANIMATION = 1
    SWITCH = 2
    FREE_TEXT = 3
    TRIGGER = 4
    GUI_TEXT = 5
    FREE_NUMBER = 6
    COORDINATE = 7
    TRANSFORM = 8
    MULTI_SELECT = 9


class GenericParameter:
    """Python wrapper for C++ GenericParameter struct
       This is used for parameters that are accessible on the editor interface.
       It contains a type and a value for the runtime usage, but also contains
       editor related data, like description, request type and is_set
    """
    
    def __init__(self, request_type: Union[RequestParameterType, int] = RequestParameterType.FREE_NUMBER,
                 description: str = "", value_type: Union[ValueType, int] = ValueType.VEC4,
                 value: Any = None, is_set: bool = False):
        # Handle integer values for enums
        if isinstance(request_type, int):
            self.request_type = RequestParameterType(request_type)
        else:
            self.request_type = request_type
            
        if isinstance(value_type, int):
            self.value_type = ValueType(value_type)
        else:
            self.value_type = value_type
            
        self.description = description
        self.value = value
        self.is_set = is_set
    
    def __repr__(self) -> str:
        # Convert integer back to enum for display
        try:
            value_type_name = ValueType(self.value_type).name
        except (ValueError, AttributeError):
            value_type_name = str(self.value_type)
        return f"GenericParameter(description='{self.description}', type={value_type_name}, value={self.value})"
    
    def is_string(self) -> bool:
        return self.value_type == ValueType.STRING or self.value_type == ValueType.STRING.value
    
    def is_double(self) -> bool:
        return self.value_type == ValueType.DOUBLE or self.value_type == ValueType.DOUBLE.value
    
    def is_long(self) -> bool:
        return self.value_type == ValueType.LONG or self.value_type == ValueType.LONG.value
    
    def is_boolean(self) -> bool:
        return self.value_type == ValueType.BOOLEAN or self.value_type == ValueType.BOOLEAN.value
    
    def is_vec4(self) -> bool:
        return self.value_type == ValueType.VEC4 or self.value_type == ValueType.VEC4.value
    
    def is_mat4(self) -> bool:
        return self.value_type == ValueType.MAT4 or self.value_type == ValueType.MAT4.value
    
    def is_long_array(self) -> bool:
        return self.value_type == ValueType.LONG_ARRAY or self.value_type == ValueType.LONG_ARRAY.value
    
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
    
    def get_mat4(self) -> List[List[float]]:
        if self.is_mat4() and isinstance(self.value, list):
            return self.value
        raise ValueError(f"GenericParameter is not a valid MAT4, it's {self.value_type.name}")
    
    def get_long_array(self) -> List[int]:
        if self.is_long_array() and isinstance(self.value, list):
            return self.value
        raise ValueError(f"GenericParameter is not a valid LONG_ARRAY, it's {self.value_type.name}")
