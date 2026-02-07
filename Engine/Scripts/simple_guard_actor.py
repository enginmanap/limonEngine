from actor_interface import ActorInterface
from generic_parameter import GenericParameter, RequestParameterType, ValueType
import limon


class SimpleGuardActor(ActorInterface):
    """
    A simple guard AI that patrols and reacts to player presence.
    Demonstrates the ActorInterface functionality.
    """
    counter = 0

    def __init__(self, limon_api):
        super().__init__(limon_api)
        self._limon_api = limon_api
        self._state = "patrol"
        self._last_player_seen_time = 0
        self._alert_duration = 5000  # 5 seconds in milliseconds
        self._patrol_direction = 1
        
    def get_name(self) -> str:
        """Return the name of this actor."""
        return "SimpleGuardActor"
    
    def play(self, time: int, actor_information) -> None:
        """
        Main update function called each game tick.
        
        Args:
            time: Current game time in milliseconds
            actor_information: ActorInformation with environment state
        """
        self.counter += 1
        if self.counter % 120 == 0:
            # Check if player is detected
            if actor_information.can_see_player_directly and actor_information.is_player_front and not actor_information.player_dead:
                self._handle_player_sighted(time, actor_information)
            else:
                self._handle_patrol(time, actor_information)
    
    def interaction(self, interaction_data) -> bool:
        """
        Handle interaction with this actor. Might be sent from Player or other AI, or Triggers.
        
        Args:
            interaction_data: List of GenericParameter objects
            
        Returns:
            bool: True if interaction was successful
        """
        print(f"{self.get_name()}: Interaction received!")
        try:
            if interaction_data and len(interaction_data) > 0:
                for param in interaction_data:
                    if param.description == "action" and param.is_set:
                        action = param.value
                        if action == "alert":
                            self._state = "alert"
                            print(f"{self.get_name()}: Put on high alert!")
                            return True
                        elif action == "reset":
                            self._state = "patrol"
                            self._last_player_seen_time = 0
                            print(f"{self.get_name()}: Reset to patrol state")
                            return True
                        else:
                            print(f"{self.get_name()}: Unknown action '{action}'")
                            return False
                return True
            print(f"{self.get_name()}: No interaction data received")
            return False
        except Exception as e:
            print(f"{self.get_name()}: Error in interaction: {e}")
            return False

    def get_parameters(self):
        """
        Get configurable parameters for this actor.
        
        Returns:
            list: List of GenericParameter objects
        """
        params = []
        
        # Alert duration parameter
        alert_param = GenericParameter()
        alert_param.request_type = RequestParameterType.FREE_NUMBER
        alert_param.description = "alert_duration"
        alert_param.value_type = ValueType.DOUBLE
        alert_param.value = float(self._alert_duration)
        alert_param.is_set = True
        params.append(alert_param)
        
        # Initial state parameter
        state_param = GenericParameter()
        state_param.request_type = RequestParameterType.FREE_TEXT
        state_param.description = "initial_state"
        state_param.value_type = ValueType.STRING
        state_param.value = self._state
        state_param.is_set = True
        params.append(state_param)
        
        return params
    
    def set_parameters(self, parameters):
        """
        Set parameters for this actor.
        
        Args:
            parameters: List of GenericParameter objects
        """
        try:
            if not parameters:
                print(f"{self.get_name()}: No parameters provided")
                return
            for param in parameters:
                if param.description == "alert_duration" and param.is_set:
                    self._alert_duration = int(param.value)
                    print(f"{self.get_name()}: Alert duration set to {self._alert_duration}ms")
                    
                elif param.description == "initial_state" and param.is_set:
                    new_state = param.value
                    if new_state in ["patrol", "alert", "chase"]:
                        self._state = new_state
                        print(f"{self.get_name()}: Initial state set to {self._state}")
                    else:
                        print(f"{self.get_name()}: Invalid state '{new_state}', keeping current state")
                else:
                    print(f"{self.get_name()}: Unknown parameter '{param.description}'")
        except Exception as e:
            print(f"{self.get_name()}: Error setting parameters: {e}")

    def _handle_player_sighted(self, time: int, actor_information):
        """Handle when player is spotted."""
        self._last_player_seen_time = time
        
        # Determine behavior based on distance
        distance = actor_information.player_distance
        
        if distance < 5.0:
            self._state = "chase"
            print(f"{self.get_name()}: Player spotted at close range ({distance:.1f} units)! CHASING!")
            
            # Try to interact with player (attack/alert)
            interaction_params = []
            attack_param = GenericParameter()
            attack_param.request_type = RequestParameterType.FREE_TEXT
            attack_param.description = "action"
            attack_param.value = "attack"
            interaction_params.append(attack_param)
            
            # This would interact with nearby objects/players
            # self._limon_api.interact_with_ai(self.model_id, interaction_params)
            
        elif distance < 15.0:
            self._state = "alert"
            print(f"{self.get_name()}: Player detected at medium range ({distance:.1f} units)! On alert!")
            
        else:
            self._state = "alert"
            print(f"{self.get_name()}: Player spotted at far range ({distance:.1f} units)! Watching...")
        
        # Log player direction information
        player_dir = actor_information.player_direction
        if hasattr(player_dir, 'x'):
            print(f"{self.get_name()}: Player direction: ({player_dir.x:.2f}, {player_dir.y:.2f}, {player_dir.z:.2f})")
        else:
            print(f"{self.get_name()}: Player direction: ({player_dir['x']:.2f}, {player_dir['y']:.2f}, {player_dir['z']:.2f})")
        
        # Check relative position
        if actor_information.is_player_left:
            print(f"{self.get_name()}: Player is to the left")
        if actor_information.is_player_right:
            print(f"{self.get_name()}: Player is to the right")
        if actor_information.is_player_front:
            print(f"{self.get_name()}: Player is in front")
        if actor_information.is_player_back:
            print(f"{self.get_name()}: Player is behind")
    
    def _handle_patrol(self, time: int, actor_information):
        """Handle patrol behavior when player is not visible."""
        # Check if we should return to patrol from alert state
        if self._state == "alert" and (time - self._last_player_seen_time) > self._alert_duration:
            self._state = "patrol"
            print(f"{self.get_name()}: Lost track of player, returning to patrol")
            return
        
        if self._state == "chase" and (time - self._last_player_seen_time) > (self._alert_duration // 2):
            self._state = "alert"
            print(f"{self.get_name()():} Lost player during chase, searching...")
            return
        
        # Simple patrol behavior
        if self._state == "patrol":
            if time % 3000 < 50:  # Log every 3 seconds
                print(f"{self.get_name()}: Patrolling area... (direction: {'forward' if self._patrol_direction > 0 else 'backward'})")
                # Simulate direction change
                if time % 10000 < 50:  # Change direction every 10 seconds
                    self._patrol_direction *= -1
                    print(f"{self.get_name()}: Changing patrol direction")
        
        elif self._state == "alert":
            # Searching behavior
            if time % 2000 < 50:  # Log every 2 seconds
                print(f"{self.get_name()}: Searching for player...")
                
                # Check if we have a route to player (if pathfinding is available)
                if actor_information.route_found and actor_information.route_ready:
                    route = actor_information.route_to_request
                    print(f"{self.get_name()():} Route to player found with {len(route)} waypoints")
                else:
                    print(f"{self.get_name()}: No route to player available")


# Register the actor type
def register_as_actor(actor_map):
    """Register this actor with the engine."""
    actor_map["SimpleGuardActor"] = lambda id, api: SimpleGuardActor(api)
