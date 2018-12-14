---
layout: post
title: "Limon Engine now has SSAO"
description: "How Limon implements SSAO with forward rendering"
date: "2018-12-14 00:17:13 +0300"
author: enginmanap
version: 0.5.2
categories: [technical]
github_comments_issueid: 72
---

TL;DR version: I won't remove the branch, you can check out all code changes step by step on "ssao" branch.

The features listed on the roadmap for 0.6 release is implemented, and I am trying to build a map as a user to find issues on it before releasing. Since the release focus is on enabling gunfights, I was looking out for asset suites to fit that and bought one with western theme. While building a map for showcasing, I realized places under shadow are just too dark for daytime environments. I thought it is not a big issue, Limon supports ambient factor too, but after testing it out, I realized on total shadow places, ambient light just doesn't have any details. For example check out the image below:

![ligth, light+ambient, ambient]({{ site.baseurl }}/assets/images/posts/55/combined.png)

Looking around rest of the map, the colors of the ambient only version is the one I want, but it has so little detail, doesn't look right. So I started reading about screen space ambient occlusion for improving ambient rendering.

The popular resources I found [John-Capman](http://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html) and [learnOpengl](https://learnopengl.com/Advanced-Lighting/SSAO) both uses deferred renderers for SSAO and doesn't contain any information about how a forward renderer might implement it. Apparently Crysis implemented SSAO originally with forward rendering, but there are no details about how it did it. I was not sure that SSAO is what I wanted, and if it would work as I expected, so I decided to test it with minimal effort. To see what it would look like, I add a depth pre-pass, and sample it within the current object render shader with the following code:
```
float occlusion = 0.0;
float uRadius = 5.0f;
float currentDepth = gl_FragCoord.z;
for(int i = -3; i < 3; i++) {
    for(int j=-3; j < 3; j++) {
        vec2 offset = gl_FragCoord.xy;
        offset.x += i*uRadius;
        offset.y += j*uRadius;
                        offset.x = offset.x / 1920;
                        offset.y = offset.y / 1080;
        float sampleDepth = texture(ssaoSampler, offset.xy).r;
        float rangeCheck = abs(currentDepth - sampleDepth) < 0.001 ? 1.0 : 0.0;

        occlusion += (sampleDepth <= currentDepth ? 1.0 : 0.0) * rangeCheck;
    }

}
occlusion = occlusion / 49;
``` 
It is just a makeshift thing that remotely resembles what we want, but it yields the following result:

![Makeshift SSAO]({{ site.baseurl }}/assets/images/posts/55/combined.png)

It has banding, and things are blurry but the details are improved tremendously, and seeing this result, I was convinced SSAO was what I want. I implemented a proper version that you can check out [the commit](https://github.com/enginmanap/limonEngine/commit/adc4756094112ca3dda7d23d6c4c8e6549b62d9e#diff-d7e2342af9ab281bdd46f5126c7daf96R155). 

![SSAO]({{ site.baseurl }}/assets/images/posts/55/ssaoWithoutBlurZoomed.png)

Results were promising, but performance was just terrible. Without SSAO and depth pre-pass I was getting +1000 FPS on my 1070, depth pre-pass lowered to ~850, but SSAO brings it down to ~170. I knew calculating it within the model rendering was not good, but didn't expect that kind of hit. Tried lowering sample count from 64 to more reasonable 16, but it caused banding like the first one, and since I am calculating it on the same pass with all coloring, there was no way to blur. OK, make it a post-processing effect. 

But how a forward renderer would do this? First I tried to search SSAO with forward render but didn't find anything, then I realized Godot is using forward renderer and checked their [documentation](https://godotengine.org/article/godot-3-renderer-design-explained). It confirmed what the name "Postprocessing effect" suggests. Render to texture, and pass again. Limon engine is already rendering multiple passes for shadow maps but didn't have separate passes for coloring other than them. To have proper SSAO, I implemented a [QuadRenderer](https://github.com/enginmanap/limonEngine/blob/master/src/PostProcess/QuadRenderBase.h) as the base for post-processing effects, and use it to have SSAO with blur. The commit about multi-pass is [here](https://github.com/enginmanap/limonEngine/commit/6ebfe205da841e24448a4813db2e70a461a73641). For a short explanation, coloring pass creates 3 textures: normal, full color+alpha, ambient. Postprocessing pass uses normal+depth to calculate occlusion factor. Another post-processing pass blurs the occlusion texture. The last pass removes ambient*occlusionfactor from full color and pushes it to screen.  

![SSAO with blur]({{ site.baseurl }}/assets/images/posts/55/ssaoWithBlurSM.png)

Finally, I tuned bias to get the result I wished. As the top of this post states, I don't remove branches on purpose if someone wants to check out how it was implemented. the branch name for all the changes is "ssao". You can ask for clarification using the comment ticket for anything. 