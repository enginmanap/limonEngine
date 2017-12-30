---
layout: post
title: "Hello World"
description: "First post and sum of status."
date: "2017-12-30 16:17:13 +0300"
author: enginmanap
version: 0.0.2
categories: [status]
---

Hi,

This is the first post, so let me explain what is the goal of this, and where am I at the moment.

This is a 3D FPS game engine project. It is not suppose to be great, but it should have the essential functionality that can support a game. I am trying to figure out what goes where, and reading books and blogs wouldn't cut it anymore. Current status can be summed up as:

* Window and input management using SDL2, multiplatform. (Regularly testing on GNU Linux, Windows 10 and Mac OS
* Rendering 3d world using models loaded using Assimp
* Dynamic shadow mapping for directional and point lights
* Skeletal animations. To test animations first animation is run repeatedly.
* Rigidbody physics using Bullet
* Basic GUI

To sum up, you can build a map, run around in it, push objects. 

I believe it can be used for a platformer if I can add these:
* Interactive UI
* A gameplay layer. Currently falling off an edge means you will fall indefinitely, and there is no push this button to activate door, etc.

For a more traditional FPS, we would need:
* AI.
* Guns + shooting
* Pickups