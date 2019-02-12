---
layout: post
title: "Version 0.6 Released"
description: "Version 0.6 is released with support for shooter types"
date: "2019-02-12 21:07:52 +0300"
author: enginmanap
version: 0.6
categories: [News]
github_comments_issueid: 106
---


With more than 6 months of work, and more than 550 commits later, version 0.6 is released. This is the first release that enables shooter logic to be implemented solely using the engine provided facilities. Most important change is addition of "Player Extension" and "AI Actor" customisation points. With these, it is possible to implement shooting, being hit, chases etc. in a unique manner as you wish your game to be. Of course to enable wide variety of possibilities, API had to be improved, and it has tripled in size since 0.5 release. 

Other than that, Model groups and stacks are added. Model groups are a quality of life feature, makes use of editor easier. Object stacks on the other hand, used in game for various things, like attaching a gun to a models hand, to putting bullet holes to moving objects. It is also used on player so any model can be attached to player too.

Editor also get major improvements. The first ones to be recognized are, assets are now discovered automatically, and shown in a directory tree with search/filtering. It makes finding the asset you want much easier than old list interface. Also game objects in the world is now a tree, with main branches following object type like Model/GUI/Light etc.

Lastly, there is a tech demo to showcase current state of the engine, and what can be done using it. It is a western themed shooter. I am not an 3D artist, or sound designer, or map designer, so it might not be as fun as it could. Even this state of it is thanks to various people putting their art online. If I used an asset of yours, but didn't mention it, please contact me so I can fix it.

Thanks to:

- Skyboxes by Wello Soft (Wildan Mubarok).
- Synth studios for the 3D assets.
- mixamo.com for animations.
- Dan Knoflicek for the music.
- Daniel Simon for Train whistle sound.