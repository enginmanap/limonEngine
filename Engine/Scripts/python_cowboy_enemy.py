#
# Python port of samples/CowboyEnemyAI.cpp. Behaviour is kept identical to the C++ actor.
# Registers as the actor type "PythonCowboyEnemy" (auto-discovery uses the class name),
# distinct from the C++ "Cowboy Enemy" so both can coexist in the editor.
#
import random
from enum import Enum

import limon
from actor_interface import ActorInterface
from generic_parameter import GenericParameter, RequestParameterType, ValueType
from vec3 import Vec3


class State(Enum):
    DEAD = 0
    IDLE = 1
    WALKING = 2
    RUNNING = 3
    SHOOTING = 4
    MELEE = 5
    KNEELING_DOWN = 6
    STANDING_UP = 7
    KNEEL_SHOOTING = 8
    KNEEL_IDLE = 9
    HIT = 10
    SCRIPTED = 11


class Gun(Enum):
    PISTOL = 0
    RIFLE = 1
    SHOTGUN = 2


class PythonCowboyEnemy(ActorInterface):
    """
    Cowboy Enemy can be doing one of these:
    1) Scripted loop
    2) idling until player detection
    3) walking to player if player is close
    4) hitting player with melee if player is hitting close
    5) running to player if player is at medium distance
    6) Shooting player
    7) kneeling to shoot
    8) kneel shooting
    9) standing up

    kneeling is to be selected by random, only if player is medium distance and visible
    """

    # Constants (mirror the C++ const members)
    DETECTION_DISTANCE = 100  # too little because of testing
    MELEE_DISTANCE = 6
    RUN_DISTANCE = 25

    def __init__(self, actor_id: int, limon_api):
        super().__init__(actor_id, limon_api)

        self.current_state = State.IDLE
        self.before_state = State.IDLE  # Shooting and Hit use this to return
        self.current_gun = Gun.RIFLE

        self.current_animation_name = ""
        self.current_animation_finished = False

        self.route_to_request = []
        self.route_get_time = 0
        self.route_requested = False

        self.last_setup_time = 0
        self.last_shoot_time = 0
        self.shooting_stage = 0
        self.last_walk_direction = Vec3(0.0, 0.0, 0.0)

        self.walk_speed = 0.065
        self.run_speed = 0.14

        # Parameters exposed as settings
        self.shoot_chance = 0.85
        self.kneel_down_chance = 0.005
        self.kneel_stay_chance = 0.0025
        self.min_shoot_time_wait = 1000
        self.hit_points = 100
        self.gun_damage = 15

        self.latest_information = None

        # Per-instance zone names so each enemy appears separately in the profiler.
        self._play_zone_name = f"CowboyEnemyAI::play[{actor_id}]"
        self._interaction_zone_name = f"CowboyEnemyAI::interaction[{actor_id}]"

    def get_name(self) -> str:
        return "Python Cowboy Enemy"

    # ------------------------------------------------------------------ helpers
    @staticmethod
    def _to_vec3(point) -> Vec3:
        """Normalise a route node (engine sends glm::vec3 as a dict {x,y,z}) into a Vec3."""
        if isinstance(point, Vec3):
            return point
        if isinstance(point, dict):
            return Vec3(point["x"], point["y"], point["z"])
        if hasattr(point, "x"):
            return Vec3(point.x, point.y, point.z)
        return Vec3(point[0], point[1], point[2])

    @staticmethod
    def _vec4(vec3: Vec3, w: float = 0.0):
        return limon.Vec4(vec3.x, vec3.y, vec3.z, w)

    # ------------------------------------------------------------------ main update
    def play(self, time: int, information) -> None:
        with self.limon_api.profile_scope(self._play_zone_name):
            self.latest_information = information

            self.last_setup_time = time
            if information.route_ready:
                self.route_to_request = [self._to_vec3(node) for node in information.route_to_request]
                if information.route_found and self.route_to_request:
                    self.last_walk_direction = self.route_to_request[0] - self.get_position() - Vec3(0.0, 2.0, 0.0)
                self.route_get_time = time
                self.route_requested = False

            is_player_visible = (information.can_see_player_directly and information.is_player_front
                                 and not information.player_dead
                                 and information.player_distance < self.DETECTION_DISTANCE)

            self.current_animation_name = self.limon_api.get_model_animation_name(self.model_id)
            self.current_animation_finished = self.limon_api.get_model_animation_finished(self.model_id)

            # there is a special case, where the player dies while actor is kneeling. He must get up.
            if self.current_state == State.SCRIPTED:
                pass  # not implemented
            elif information.player_dead:
                if self.current_state != State.DEAD:
                    self.transition_to_idle(information)
            else:
                self._run_state_machine(information, is_player_visible)

    def _run_state_machine(self, information, is_player_visible) -> None:
        state = self.current_state

        if state == State.SCRIPTED or state == State.DEAD:
            return  # do nothing

        elif state == State.IDLE:
            if is_player_visible:
                if information.player_distance < self.MELEE_DISTANCE:
                    self.transition_to_melee(information)
                elif information.player_distance < self.RUN_DISTANCE:
                    self.transition_to_walk(information)
                else:
                    self.transition_to_run(information)
            else:
                self.transition_to_idle(information)

        elif state == State.WALKING:
            if information.player_distance < self.MELEE_DISTANCE:
                self.transition_to_melee(information)
            elif information.player_distance < self.RUN_DISTANCE:
                # possible to Shoot, check last shoot time, and that player can be seen and in front
                print("Checking if can Shoot")
                print(f"last_shoot_time: {self.last_shoot_time}, min_shoot_time_wait: {self.min_shoot_time_wait}, last_setup_time: {self.last_setup_time}")
                print(f"can_see_player_directly: {information.can_see_player_directly}, cosine_between_player: {information.cosine_between_player}")
                print(f"player_distance: {information.player_distance}")
                print(f"shoot_chance: {self.shoot_chance}")
                if ((self.last_shoot_time + self.min_shoot_time_wait) < self.last_setup_time
                        and information.can_see_player_directly and information.cosine_between_player > 0.9):
                    if random.random() < self.shoot_chance:
                        self.transition_to_shoot(information)
                else:
                    self.transition_to_walk(information)
            else:
                self.transition_to_run(information)

        elif state == State.RUNNING:
            if information.player_distance < self.MELEE_DISTANCE:
                self.transition_to_melee(information)
            elif information.player_distance < self.RUN_DISTANCE:
                self.transition_to_walk(information)
            else:
                if is_player_visible:
                    if ((self.last_shoot_time + self.min_shoot_time_wait) < self.last_setup_time
                            and information.can_see_player_directly
                            and information.cosine_between_player > 0.9
                            and random.random() > self.shoot_chance):
                        self.transition_to_shoot(information)
                    elif random.random() < self.kneel_down_chance:
                        self.transition_to_kneel(information)
                    else:
                        self.transition_to_run(information)
                else:
                    self.transition_to_run(information)

        elif state == State.SHOOTING:
            if self.current_animation_finished and self.shooting_stage == 3:  # multi stage shooting handling
                self.shooting_stage = 0  # reset shooting
                if self.before_state in (State.KNEEL_IDLE, State.KNEEL_SHOOTING, State.STANDING_UP):
                    self.transition_to_kneel_idle(information)
                else:
                    # same as IDLE
                    if information.player_distance < self.MELEE_DISTANCE:
                        self.transition_to_melee(information)
                    elif information.player_distance < self.RUN_DISTANCE:
                        self.transition_to_walk(information)
                    else:
                        self.transition_to_run(information)
            else:
                self.transition_to_shoot(information)

        elif state == State.MELEE:
            if self.current_animation_finished:
                if information.player_distance < self.MELEE_DISTANCE:
                    self.transition_to_melee(information)
                else:
                    self.transition_to_walk(information)
            # If animation not finished, don't do anything

        elif state == State.KNEELING_DOWN:
            if self.current_animation_finished:
                self.transition_to_kneel_idle(information)
            # If animation not finished, don't do anything

        elif state == State.KNEEL_IDLE:
            if is_player_visible:
                if (self.last_shoot_time + self.min_shoot_time_wait < self.last_setup_time
                        and random.random() > self.shoot_chance):
                    self.transition_to_kneel_shoot(information)
                else:
                    self.transition_to_kneel_idle(information)
            else:
                self.transition_to_stand_up(information)

        elif state == State.KNEEL_SHOOTING:
            if self.current_animation_finished:
                if is_player_visible:
                    if random.random() < self.kneel_stay_chance:
                        self.transition_to_kneel_shoot(information)
                    else:
                        self.transition_to_stand_up(information)
                else:
                    self.transition_to_stand_up(information)

        elif state == State.STANDING_UP:
            if self.current_animation_finished:
                self.transition_to_run(information)  # since we know player is distant

        elif state == State.HIT:
            if self.current_animation_finished:  # same as shoot
                self.limon_api.set_model_animation_speed(self.model_id, 1.0)
                if self.before_state in (State.KNEEL_IDLE, State.KNEEL_SHOOTING, State.STANDING_UP):
                    self.transition_to_kneel_idle(information)
                else:
                    # same as IDLE
                    if information.player_distance < self.MELEE_DISTANCE:
                        self.transition_to_melee(information)
                    elif information.player_distance < self.RUN_DISTANCE:
                        self.transition_to_walk(information)
                    else:
                        self.transition_to_run(information)

    # ------------------------------------------------------------------ interaction
    def interaction(self, interaction_information) -> bool:
        with self.limon_api.profile_scope(self._interaction_zone_name):
            if not interaction_information or len(interaction_information) < 1:
                return False

            try:
                got_hit = interaction_information[0].value == "GOT_HIT"
            except Exception:
                got_hit = False

            if got_hit:
                if self.hit_points < 20:
                    self.hit_points = 0
                else:
                    self.hit_points = self.hit_points - 20
                if self.hit_points == 0:
                    self.transition_to_dead()
                else:
                    self.transition_to_hit()
                return True
            return False

    # ------------------------------------------------------------------ parameters
    def get_parameters(self):
        parameters = []

        hit_point_parameter = GenericParameter(RequestParameterType.FREE_NUMBER, "Hit points", ValueType.LONG,
                                               int(self.hit_points), True)
        parameters.append(hit_point_parameter)

        kneel_down = GenericParameter(RequestParameterType.FREE_NUMBER, "AI kneel down chance in %", ValueType.DOUBLE,
                                      float(self.kneel_down_chance), True)
        parameters.append(kneel_down)

        kneel_stay = GenericParameter(RequestParameterType.FREE_NUMBER, "AI kneel stay chance in %", ValueType.DOUBLE,
                                      float(self.kneel_stay_chance), True)
        parameters.append(kneel_stay)

        min_shoot_wait = GenericParameter(RequestParameterType.FREE_NUMBER, "Wait until shoot again (in ms.)",
                                          ValueType.LONG, int(self.min_shoot_time_wait), True)
        parameters.append(min_shoot_wait)

        gun_damage = GenericParameter(RequestParameterType.FREE_NUMBER, "Damage of Gun", ValueType.LONG,
                                      int(self.gun_damage), True)
        parameters.append(gun_damage)

        # First gun type entry holds the currently selected one, the rest list the options.
        selected_gun_name = {Gun.PISTOL: "Pistol", Gun.RIFLE: "Rifle", Gun.SHOTGUN: "Shotgun"}[self.current_gun]
        parameters.append(GenericParameter(RequestParameterType.MULTI_SELECT, "Gun type", ValueType.STRING,
                                           selected_gun_name, True))
        parameters.append(GenericParameter(RequestParameterType.MULTI_SELECT, "Gun type", ValueType.STRING,
                                           "Pistol", True))
        parameters.append(GenericParameter(RequestParameterType.MULTI_SELECT, "Gun type", ValueType.STRING,
                                           "Rifle", True))
        parameters.append(GenericParameter(RequestParameterType.MULTI_SELECT, "Gun type", ValueType.STRING,
                                           "Shotgun", True))

        return parameters

    def set_parameters(self, parameters):
        if not parameters:
            return
        gun_type_set = False
        for parameter in parameters:
            if parameter.description == "Hit points":
                self.hit_points = int(parameter.value)
            elif parameter.description == "AI kneel down chance in %":
                self.kneel_down_chance = float(parameter.value)
            elif parameter.description == "AI kneel stay chance in %":
                self.kneel_stay_chance = float(parameter.value)
            elif parameter.description == "Wait until shoot again (in ms.)":
                self.min_shoot_time_wait = int(parameter.value)
            elif parameter.description == "Damage of Gun":
                self.gun_damage = int(parameter.value)
            elif parameter.description == "Gun type" and not gun_type_set:
                if parameter.value == "Pistol":
                    self.current_gun = Gun.PISTOL
                elif parameter.value == "Rifle":
                    self.current_gun = Gun.RIFLE
                elif parameter.value == "Shotgun":
                    self.current_gun = Gun.SHOTGUN
                else:
                    print("Gun type didn't match possible values, assuming pistol.")
                    self.current_gun = Gun.PISTOL
                gun_type_set = True

    # ------------------------------------------------------------------ state transitions
    def transition_to_melee(self, information) -> None:
        self.turn_face_to_player(information)
        # since transition requested, we know player is near
        if self.current_gun == Gun.PISTOL:
            self.limon_api.set_model_animation_with_blend(self.model_id, "Pistol Whip|", False)
            damage = 10
            melee = GenericParameter(value_type=ValueType.STRING, value="MELEE_PLAYER", is_set=True)
            damage_param = GenericParameter(value_type=ValueType.LONG, value=damage, is_set=True)
            self.limon_api.interact_with_player([melee, damage_param])
            self.current_state = State.MELEE
        else:  # RIFLE or SHOTGUN
            if ((self.last_shoot_time + self.min_shoot_time_wait) < self.last_setup_time
                    and information.can_see_player_directly and information.cosine_between_player > 0.9
                    and random.random() < self.shoot_chance):
                self.transition_to_shoot(information)
            else:
                self.transition_to_walk(information)  # couldn't find a good animation for rifle.

    def transition_to_walk(self, information) -> None:
        self.turn_face_to_player(information)
        # ask for route to player if we need the data
        if (not self.route_to_request
                or ((self.route_get_time == 0 or self.route_get_time + 1000 < self.last_setup_time)
                    and not self.route_requested)):
            self.information_request.route_to_player = True  # ask for a route to player
            self.route_requested = True
        # now we are walking, move along the route
        if self.route_to_request:
            if self.current_state != State.WALKING:
                if self.current_gun == Gun.PISTOL:
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Pistol Walk|")
                else:
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Walk|")
            distance_to_route_node = (self.get_position() + Vec3(0.0, 2.0, 0.0) - self.route_to_request[0]).length_squared()
            if distance_to_route_node < 0.1:  # if reached first element
                self.route_to_request.pop(0)
                if self.route_to_request:
                    self.last_walk_direction = self.route_to_request[0] - self.get_position() - Vec3(0.0, 2.0, 0.0)
                else:
                    self.last_walk_direction = Vec3(0.0, 0.0, 0.0)
            move_direction = self.last_walk_direction * self.walk_speed
            self.limon_api.add_object_translate(self.model_id, self._vec4(move_direction))
            self.current_state = State.WALKING

    def transition_to_run(self, information) -> None:
        self.turn_face_to_player(information)
        if self.current_state != State.RUNNING:
            if self.current_gun == Gun.PISTOL:
                self.limon_api.set_model_animation_with_blend(self.model_id, "Pistol Run|")
            else:  # RIFLE or SHOTGUN
                self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Run|")

        # ask for route to player if we need the data
        if (not self.route_to_request
                or ((self.route_get_time == 0 or self.route_get_time + 1000 < self.last_setup_time)
                    and not self.route_requested)):
            self.information_request.route_to_player = True  # ask for a route to player
            self.route_requested = True
        # now we are walking, move along the route
        if self.route_to_request:
            distance_to_route_node = (self.get_position() + Vec3(0.0, 2.0, 0.0) - self.route_to_request[0]).length_squared()
            if distance_to_route_node < 0.1:  # if reached first element
                self.route_to_request.pop(0)
                if self.route_to_request:
                    self.last_walk_direction = self.route_to_request[0] - self.get_position() - Vec3(0.0, 2.0, 0.0)
                else:
                    self.last_walk_direction = Vec3(0.0, 0.0, 0.0)
            move_direction = self.last_walk_direction * self.run_speed
            self.limon_api.add_object_translate(self.model_id, self._vec4(move_direction))

        self.current_state = State.RUNNING

    def transition_to_kneel(self, information) -> None:
        self.turn_face_to_player(information)
        if self.current_gun == Gun.PISTOL:
            pass  # pistol kneel is not allowed, do nothing
        else:  # RIFLE or SHOTGUN
            self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Kneel From Stand|", False)
            self.current_state = State.KNEELING_DOWN

    def transition_to_kneel_idle(self, information) -> None:
        self.turn_face_to_player(information)
        if self.current_gun == Gun.PISTOL:
            print("Kneeling Pistol Cowboy shouldn't have happened.")
        else:  # RIFLE or SHOTGUN
            self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Kneel Idle|")
            self.current_state = State.KNEEL_IDLE

    def transition_to_kneel_shoot(self, information) -> None:
        self.turn_face_to_player(information)
        if self.current_gun == Gun.PISTOL:
            print("Kneeling Pistol shoot Cowboy shouldn't have happened.")
        else:  # RIFLE or SHOTGUN
            self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Kneel Fire|", False)
            self.play_shoot_sound(self.current_gun)
            self.shoot_player(information.player_distance)
            self.current_state = State.KNEEL_SHOOTING
        self.last_shoot_time = self.last_setup_time

    def transition_to_stand_up(self, information) -> None:
        self.turn_face_to_player(information)
        if self.current_gun == Gun.PISTOL:
            print("Kneeling Pistol standup Cowboy shouldn't have happened.")
        else:  # RIFLE or SHOTGUN
            self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Kneel To Stand|", False)
            self.current_state = State.STANDING_UP

    def transition_to_shoot(self, information) -> None:
        self.turn_face_to_player(information)
        if self.current_gun == Gun.PISTOL:
            self.limon_api.set_model_animation_with_blend(self.model_id, "Pistol Run 2|")  # FIXME couldn't find the correct animation
            self.play_shoot_sound(self.current_gun)
            self.shoot_player(information.player_distance)
        else:  # RIFLE or SHOTGUN
            # Rifle shooting has 3 animations. Flag the state so it won't change before all 3 finish.
            if self.shooting_stage == 0:
                self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Down To Aim|", False)
                self.shooting_stage = 1
            elif self.shooting_stage == 1:
                if self.current_animation_finished:
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Idle Fire|", False)
                    self.play_shoot_sound(self.current_gun)
                    self.shoot_player(information.player_distance)
                    self.shooting_stage = 2
            elif self.shooting_stage == 2:
                if self.current_animation_finished:
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Aim To Down|", False)
                    self.shooting_stage = 3
            self.current_state = State.SHOOTING
        self.last_shoot_time = self.last_setup_time

    def play_shoot_sound(self, gun_type) -> None:
        random_value = random.random()
        sound_names = {Gun.PISTOL: "pistol", Gun.RIFLE: "rifle", Gun.SHOTGUN: "shotgun"}
        if random_value < 0.33:
            suffix = ""
        elif random_value < 0.66:
            suffix = "2"
        else:
            suffix = "3"
        sound_path = f"./Data/Sounds/guns/{sound_names[gun_type]}{suffix}.wav"
        self.limon_api.play_sound(sound_path, self._vec4(self.get_position()), False, False)

    def shoot_player(self, player_distance) -> None:
        if self.latest_information is None or not self.latest_information.can_see_player_directly:
            # player got out of sight after shooting started; play sounds/animations but don't hurt them.
            return

        # add missing based on distance.
        if player_distance > 100.0:
            return  # miss for certain
        if random.random() < (player_distance / 100.0):
            return  # miss

        shoot = GenericParameter(value_type=ValueType.STRING, value="SHOOT_PLAYER", is_set=True)
        damage_param = GenericParameter(value_type=ValueType.LONG, value=int(self.gun_damage), is_set=True)
        # 2 is the offset of model. 3d modeller should give this.
        source_position = self.get_position() + Vec3(0.0, 2.0, 0.0)
        position_param = GenericParameter(value_type=ValueType.VEC4,
                                          value=[source_position.x, source_position.y, source_position.z, 0.0],
                                          is_set=True)
        self.limon_api.interact_with_player([shoot, damage_param, position_param])

    def transition_to_idle(self, information) -> None:
        if self.current_state != State.IDLE:
            if self.current_gun == Gun.PISTOL:
                self.limon_api.set_model_animation_with_blend(self.model_id, "Pistol Idle|")
            else:  # RIFLE or SHOTGUN
                value = random.random()
                if value > 0.33:
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Idle|")
                elif value > 0.66:
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Idle 2|")
                else:
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Idle 3|")
            self.current_state = State.IDLE

    def turn_face_to_player(self, information) -> None:
        # face the player
        if information.is_player_left:
            if information.cosine_between_player_for_side < 0.65:
                self.limon_api.add_object_orientation(self.model_id, limon.Vec4(0.0, 0.030, 0.0, 1.0))
            elif information.cosine_between_player_for_side < 0.95:
                self.limon_api.add_object_orientation(self.model_id, limon.Vec4(0.0, 0.015, 0.0, 1.0))
        if information.is_player_right:
            # turn just a little bit to right
            if information.cosine_between_player_for_side < 0.65:
                self.limon_api.add_object_orientation(self.model_id, limon.Vec4(0.0, -0.030, 0.0, 1.0))
            elif information.cosine_between_player_for_side < 0.95:
                self.limon_api.add_object_orientation(self.model_id, limon.Vec4(0.0, -0.015, 0.0, 1.0))

    def transition_to_hit(self) -> None:
        # since hit has priority over everything, make sure shooting is not left in the middle
        self.shooting_stage = 0
        self.limon_api.set_model_animation_speed(self.model_id, 1.5)
        if self.current_gun == Gun.PISTOL:
            self.limon_api.set_model_animation_with_blend(self.model_id, "Pistol Idle Hit Reaction|", False)
            self.current_state = State.HIT
        else:  # RIFLE or SHOTGUN
            if self.current_state in (State.KNEEL_SHOOTING, State.KNEEL_IDLE, State.STANDING_UP):
                self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Kneel Hit Reaction|", False)
                self.current_state = State.HIT
            elif self.current_state == State.WALKING:
                self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Walk Hit Reaction|", False)
                self.current_state = State.HIT
            elif self.current_state == State.RUNNING:
                self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Run Hit Reaction|", False)
                self.current_state = State.HIT
            elif self.current_state in (State.IDLE, State.MELEE, State.KNEELING_DOWN, State.SHOOTING):
                self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Idle Hit Reaction|", False, 1000)
                self.current_state = State.HIT
            # HIT, DEAD, SCRIPTED: do nothing

    def transition_to_dead(self) -> None:
        if self.current_state != State.HIT and self.current_state != State.DEAD:
            if self.current_gun == Gun.PISTOL:
                self.limon_api.set_model_animation_with_blend(self.model_id, "Generic Dying|", False, 500)
                self.current_state = State.DEAD
            else:  # RIFLE or SHOTGUN
                if self.current_state in (State.KNEEL_SHOOTING, State.KNEEL_IDLE, State.STANDING_UP):
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Kneel Death|", False)
                    self.current_state = State.DEAD
                elif self.current_state in (State.WALKING, State.RUNNING, State.IDLE, State.MELEE,
                                            State.KNEELING_DOWN, State.SHOOTING):
                    self.limon_api.set_model_animation_with_blend(self.model_id, "Rifle Death|", False)
                    self.current_state = State.DEAD
                # HIT, DEAD, SCRIPTED: do nothing


# Register the actor type
def register_as_actor(actor_map):
    """Register this actor with the engine."""
    actor_map["PythonCowboyEnemy"] = lambda actor_id, api: PythonCowboyEnemy(actor_id, api)
