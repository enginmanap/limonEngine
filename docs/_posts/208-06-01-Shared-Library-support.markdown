---
layout: post
title: "Hello World"
description: "Shared libraries that Use loader Application methods"
date: "2018-06-01 00:26:13 +0300"
author: enginmanap
version: 0.0.4
categories: [status][How-to]
github_comments_issueid: 16
---

Last couple of weeks I was thinking on how to approach extensibility of the Limon Engine. I worked on a prototype that loads python code in to the engine, but skipping c++ plugins didn't feel right. I knew the basics, a library might be compiled for dynamic linking or static linking, I need to provide a header etc. But when I tried to implement it, I quickly realized, to create a shared library (.dll or .so), I will need to compile a whole lot of the engine itself with in the library, or linker won't be able to create a binary. 

At first I couldn't find whats wrong, and when I searched for how these use cases are handled in C++ world, the information I find seemed right, but I couldn't make it work. I later realized the issue was, all the information I get was about creating a dynamic loading library that another application uses, but the library itself is not depending on loader application. That is not the case for Limon Engine. The engine provides methods to manipulate game state, so the library must use engine, and engine should use the library.

## Problem 
So the issue here is basic knowledge, but the perspective is making it hard to realize.Starting from the beginning, for any method call, that methods object code must be linked to generate final binary. When you are linking against a library you want to use, like Bullet Physics, you need header files for your code to compile, but when linking you actually need compiled object files from Bullet physics. The good part is, Bullet physics don't need your code to compile, so it can be distributed with precompiled object files you can directly link, dynamic like a .dll or static like a .lib. But in our case Limon Engine extensions use Limon Engine API, and API uses internal methods, so extensions indirectly require whole engine at link time.

After realizing this, I implemented a shared library subsystem that "works" for my needs, but I didn't read how to do this from anywhere. It is entirely possible this is the worst way to do it, so if you have a better way, or if there is something that  I missed please let me know.

## Implementation

So what do we know? We need to seperate method calls extensions make, and the method calls Limon Engine actually runs. How can this be done? I choose to simply hide this fact on link time, and postpone the method call resolution to run time. C++ has this facility in the form of std::function. 