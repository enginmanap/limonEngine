---
layout: post
title: "Poorer mans character controller"
description: "How Limon engine simulates character with Bullet"
date: "2017-12-30 17:44:00 +0300"
author: enginmanap
version: 0.0.2
categories: [technical]
tags: bullet physics,character controller
---

Hi,

As I am trying to improve the engine, I tried out different maps I find online, and one of the biggest issues I face was, almost all of the maps I can find has bumps, stairs etc. on the ground. This was a major issue because I was using a capsule shape for player simulation, and it became stuck in all the places. It was impossible to go up stairs too.

I check out the internet to find out what other people do to fix this issue, and simply put, it is putting a spring below the player. Since I am using Bullet physics, I thought it should be easy enough. It turns out it is not, and I spend almost 3 weeks trying to figure out whats going on and how to implement what I want.

Disclaimer: Biggest issue here is, Bullet documentation is not explaining anything about springs. So I tried different configurations to match my needs, I don't claim this is the correct implementation, or this is how Bullet is intended the usage, but it works good enough, so here we go.

Before adding a spring, I was using a 2 units high capsule, I made that 1 unit before adding a spring.
The stiffness and damping are set.
Then I add a spring, the class to use is btGeneric6DofSpring2Constraint. It can be used with one or two rigid bodies. Documentation says if used with only one, the other end of spring is not loose, it is attached to the world itself. It is not clear what that means, my trials points that the other end is attache to 0,0,0 point, even if there is nothing there. Second parameter is where the rigid body spring will attach, I set it to lower end. 
After that I set all the limits to between (1.0,0.0). As you can see the lower limit is higher than the higher limit. This means there is no constrain on the axis. three settings are for x,,y,z axis.
After that the equilibrium point is set. That is the point where the spring should be resting (as far as I can see), my first instict was setting this to 1 unit high of the ground, but it did not work, because the spring tends to move player up and down, instead of going to rest right away. I played aroud spring settings but that didn't help. At the end I set it to a very low point. I wanted to set it lower limit of float, but Bullet acts up if anything is at the limits, so I set it to -9999. this is clearly not a final value, I will get the AABB of the world and set accordingly on a future revision.
There is also another setting, bt constrain stop cfm. I am not sure what it does, or if does anything, but Bullet author suggests it on a forum post, says it fixes stuck at the limits issues, so I set it too.
Don't forget to add the constraint to the bullet dynamics world.

The source code looks like this:
```
    spring = new btGeneric6DofSpring2Constraint(
            *player,
            btTransform(btQuaternion::getIdentity(), { 0.0f, -1.0f, 0.0f })
    );
    spring->setStiffness(1,  35.0f);
    spring->setDamping  (1,  1.0f);

    spring->setLinearLowerLimit(btVector3(1.0f, 1.0f, 1.0f));
    spring->setLinearUpperLimit(btVector3(0.0f, 0.0f, 0.0f));
    //spring->setEquilibriumPoint(1,  std::numeric_limits<float>::lowest());// if this is used, spring does not act as it should
    spring->setEquilibriumPoint(1,  -9999);//TODO this should be set the low Y of AABB of world.
    spring->setParam(BT_CONSTRAINT_STOP_CFM, 1.0e-5f, 5);
    spring->setEnabled(false);//don't enable, player might be set to fall at the beginning
```    

   
I am sending rays from below the player to the world, to check if there is something below. Result of this check is stored on a boolean "onAir", I am using this value on movement code, if onAir, I don't register movement keys. Before the switch, I didn't do anything to slow the player down if no movement keys pressed, because friction between the player and what ever below it slowed player down enough, but after putting the spring, friction was gone, and player was sliding everywhere, so I put a check and slowed down every step, dividing current speed by 2.5. This is no where realistic, but it felt good, so didn't tried any harder.
```
    if (direction == NONE) {
        player->setLinearVelocity(player->getLinearVelocity() / slowDownFactor);
        return;
        }
```

if the player jumps, I disable the constraint, tried disabling only the spring, but that made constraint to apply a lot of force, pushing player to the sky.

```
    switch (direction) {
        case UP:
            player->setLinearVelocity(player->getLinearVelocity() + GLMConverter::GLMToBlt(up * options->getJumpFactor()));
            spring->setEnabled(false);
            break;
```  

and at last, what to do if player is just standing, or moving around. in that case, I am using the same rays I cast to determine if player is on air, to find out what is the closest objects distance to player, and using this information to feed constrain limits to spring, so it can push player up if there is a bump underneath. If there is nothing, it means player should fall, so I am disabling the constaint.
The hardest thing to calculate was what should be the limits. It turns out, constraint does not update itself using player position, so start position should be used for calculations.
since the other end it at 0,0,0, the highest point can be used directly. and adding 1 because the calculation uses center point, but we want to use the capsule shape. 
After that updating the limits constraint should use, with this standing point. I am making player stand 1 unit hight, and allowing it to be pushed 1 unit high at most. 

```
    if(!onAir) {
        springStandPoint = highestPoint + 1.0f - startPosition.y;
        spring->setLimit(1,springStandPoint + 1.0f, springStandPoint + 2.0f);
        spring->setEnabled(true);
    } else {
        spring->setEnabled(false);
    }
```  

Until this point, the player mass was not effective, but at this point it started effecting the movement, so I set it to 75 (it was 1), so the result will be more realistic.

Full source code can be seen at src/Camera.cpp

Is this the final, and everything works as it should? Sadly, no. there are 2 issues:
1. If player jumps on an really steep surface, sometimes spring creates too much force, throwing player up several units(meters for the scale)
2. Player can't jump while going up/down steep surfaces, because onAir flag constantly changing, and as I said before, the engine ignores input if onAir.

I am writing this down even with this issues, because I don't think these issues are caused by the spring, but it just allowed a deeper issue with control logic. I believe a single flag (onAir) is not enough information to disable player input or disable/enable spring on its own. So there is room for improvement, just in another place.

PS: for a better view, and inspiration, please check out Evan Todds:
http://etodd.io/2015/04/03/poor-mans-character-controller/