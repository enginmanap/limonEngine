from typing import List
from generic_parameter import GenericParameter


class TriggerInterface:
    """
    Base class for game triggers.
    Implement this to create custom trigger behaviors.
    """
    def get_parameters(self) -> List[GenericParameter]:
        """
        Returns the parameters required by this trigger.
        Returns:
            List[GenericParameter]: List of parameter definitions
        """
        raise NotImplementedError("get_parameters() not implemented")

    def run(self, parameters: List[GenericParameter]) -> bool:
        """
        Execute the trigger's action.
        Args:
            parameters: List of GenericParameter objects with values
        Returns:
            bool: True if the trigger executed successfully, False otherwise
        """
        raise NotImplementedError("run() not implemented")

    def get_results(self) -> List[GenericParameter]:
        """
        This method is accessed through API to check state of triggers.
        Valid to return empty list, if no other customization is using the state.
        Returns:
            List[GenericParameter]: List of result values
        """
        raise NotImplementedError("get_results() not implemented")

    def get_name(self) -> str:
        """
        Get the name of this trigger type.
        Returns:
            str: The trigger's name
        """
        raise NotImplementedError("get_name() not implemented")
