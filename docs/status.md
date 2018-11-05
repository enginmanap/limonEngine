---
layout: default
---

## About

Limon is a 3D FPS game engine. Main focus of its development is learning. It is not bleeding edge in any sense, but it is a functioning game engine none the less. 

It is based on Opengl 3.3 + SDL2, but many other libraries are used.

Latest release can be [downloaded here](https://github.com/enginmanap/limonEngine/releases/).

Here is the latest video of the engine:

[![Mayan Map with sound](http://img.youtube.com/vi/1OHS3TJ1q6o/0.jpg)](http://www.youtube.com/watch?v=1OHS3TJ1q6o)

# Features (as of 10 August 2018)

- Model loading using Assimp
- Skeletal animations
- Realtime shadows
- Rigid body physics
- 3D spatial sound
- Preliminary AI 
- In game map editor
- Trigger volumes
- API for Custom Trigger code
- Loading shared libraries that has Trigger code
- Creating Animations in editor

# Features to do
- There should be some AI
- Spot lights should be added.
- Debug draw should be improved
    * needs to have some other debug shapes, like sphere
    * Some cases require a duration
- For proper handling of opacity, we need to order the objects.
- object culling? We are rendering everything, which is not feasible on the long run.
- A file logger should be implemented,