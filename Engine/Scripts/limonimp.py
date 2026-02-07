# limonimp.py
import limon
from generic_parameter import RequestParameterType, GenericParameter


"""
    This is an example of a trigger interface implementation through Python.
    
"""
class MyTrigger(limon.TriggerInterface):
    def __init__(self, limon_api):
        super().__init__(limon_api)
        self._limon_api = limon_api

    def get_parameters(self):
        param = GenericParameter()
        param.request_type = RequestParameterType.FREE_TEXT
        param.description = "Test trigger"
        param.value = "Hello"
        return [param]

    def run(self, parameters):
        print("Trigger activated with parameters:", parameters)
        try:
            # Create a Vec4 object
            print("Attempting to create Vec4...")
            vec = limon.Vec4()
            vec.x = 0.1
            vec.y = 0.1
            vec.z = 0.1
            vec.w = 0.0
            print(f"Created Vec4: {vec.x}, {vec.y}, {vec.z}, {vec.w}")

            # Call the method with Vec4
            print("Calling add_object_translate with Vec4...")
            result = self._limon_api.add_object_translate(5, vec)
            print(f"add_object_translate result: {result}")
            vec2 = limon.Vec4()
            vec2.x = 1.1
            vec2.y = 1.1
            vec2.z = 1.1
            vec2.w = 0.0
            result2 = self._limon_api.add_object_scale(5, vec2)
            print(f"add_object_scale result: {result2}")
            vec3 = limon.Vec4()
            vec3.x = 0.0
            vec3.y = -0.079
            vec3.z = 0.0
            vec3.w = 0.007
            result3 = self._limon_api.add_object_orientation(5, vec3)
            print(f"add_object_orientation result: {result3}")
        except Exception as e:
            print(f"Error in run method: {type(e).__name__}: {e}")
            import traceback
            traceback.print_exc()
        return True

    def get_results(self):
        return []

    def get_name(self):
        return "MyTrigger"
