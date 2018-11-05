# How to contribute

Hi, and thanks for considering contributing to Limon Engine! Since the engine is mostly a single developer effort, it doesn't have many strict rules, but I would like share as much information as possible to make it easier.

First of all, if you need help:

1. You can check the documentation at: https://limonengine.readthedocs.io/
2. You can ask about it at Discord: https://discord.gg/gqprbFd
3. You can create a ticket on GitHub: https://github.com/enginmanap/limonEngine/issues

If you want to contribute, you can by doing (from easier to harder): 

1. Giving a star in GitHub!
2. Follow on [Youtube channel](https://www.youtube.com/channel/UC4XRCKUL22KLPQPW_L9B5gQ). Don't worry, I don't share even one video per month.
3. Join [Discord](https://discord.gg/gqprbFd) to chat
4. Download latest release and run the engine. Open issues if you see any.
5. Build the engine for another Linux distribution. It would be superb if you can PR a build file for Debian, or fedora etc.
6. Fix Github issues. I marked some of them as beginner friendly, you can start from them. 
7. Fix source tagged issues. I tag issues with "FIXME" as I see them, most of the time that means I think it is wrong, but didn't have time to figure out how it should be improved.

## Coding conventions

  * Indent using four spaces.
  * Variable names uses camel case, start with lowercase, like someVariable.
  * Class and struct names uses camel case, start with upper case, like SomeClass.
  * Constants should be upper case with underscores between words, like SOME_CONSTANT.
  * WPut spaces after list items and method parameters (`[1, 2, 3]`, not `[1,2,3]`), around operators (`x += 1`, not `x+=1`), and around hash arrows.
  * Use meaningful variable names, even if they are long. don't use "var" instead of variable.
  * Avoid multiple inheritence. If you have to, only one of the parents can have variables.
  
Thanks, and happy hacking!
