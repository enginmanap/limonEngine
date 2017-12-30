---
layout: default
---

## About

Limon is a 3D FPS game engine. Main focus of its development is learning. It is not high performance, and will not be at least for seeable future. 

It is based on Opengl 3.3 + SDL2, but many other libraries are used.

Here is the latest video of the engine:

[![Third video](http://img.youtube.com/vi/OkMz1MGk9NE/0.jpg)](https://youtu.be/OkMz1MGk9NE)

# Features

- Model loading using Assimp
- Skeletal animations
- Realtime shadows
- Rigid body physics

# New Features!
- Preliminary AI

# Features to do
- World unloading should unload assets.
- Input should use callback functions when possible
- vbo should not be exposed
- We need a map editor using QT.
- There should be some AI
- Spot lights should be added.
- Debug draw should be improved
    * ~~Performance must be better. It is not working as it should right now~~. It is not severely effecting speed at test scenes.
    * needs to have some other debug shapes, like sphere
    * Some cases require a duration
- ~~There are points that does not use time provided by main loop. everything should use it.~~. only FPS calculation left, and it should not use game clock.
- a loading screen is needed. Right now if a map takes several seconds, only a white screen is shown
- World definition should be moved to seperate file/ Partially done, except gui elements.
- For proper handling of opacity, we need to order the objects.
- object culling? We are rendering everything, which is not feasible on the long run.
- there should be a console like quake.
- Options (like shadow res, screen res etc.) should be centralized
- Logging should be better
    * log level
    * log channel (like animation, physics)
    * log destination (file, console)
