"""
Limon Engine Script Interfaces

This package contains base classes for implementing custom behaviors in the Limon Engine.
"""

from .camera_attachment import CameraAttachment
from .trigger_interface import TriggerInterface
from .player_extension_interface import PlayerExtensionInterface
from .actor_interface import ActorInterface

__all__ = [
    'CameraAttachment',
    'TriggerInterface', 
    'PlayerExtensionInterface',
    'ActorInterface'
]
