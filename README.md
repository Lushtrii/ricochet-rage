# Team01

Credits:

Title font: https://www.fontspace.com/megamax-jonathan-too-font-f124011

Button font: https://fonts.google.com/specimen/Squada+One

Polygon Triangulation Library: https://github.com/mapbox/earcut.hpp

Shadows implementation inspired by: https://ncase.me/sight-and-light/

Background image: https://wallpapercave.com/pixel-space-wallpapers

Floor texture: https://piiixl.itch.io/textures3

## M4 Features

### Changes from feedback

After we received feedback from our crossplays along with how our team felt about the game, we incorporated changes to address the feedback and improve our game.

- Added Level progress indicator - One of the biggest requests we got was for an indication of how long the level is. To address this, we added a text ui component that displays the number of remaining enemies in the level.
- Added Level transition phase - Players sometimes felt the level transitions were too abrupt, so we added a level completion animation + sound effect to give the player a minibreak and clearly indicate the level has been completed.
- Added Unique bullet color for enemy - Players expressed that with levels that had ranged enemies, they could not tell which bullets were from enemies vs their own, and since only enemy bullets do damage, being able to distinguish the two is important. Therefore, we added a unique bullet color just for enemies (red because its dangerous) and changed our player's "super-charged" bullet color from red to blue.
- Changed player health bar to green: Another suggestion was for the player's health bar to be green to distinguish it from the enemies' health bars, so we changed the player's healthbar color to green. 
- Game balancing - After our team added all our gameplay-driven features, the devs play-tested the game to find values that allowed for a challenging, but reasonable difficulty while playing the game. Afterwards, we had a peer test it, and added further tweaks based off their feedback. See GAMETWEAKS.md for information on specific changes.
- Removed mouse gestures - We found that when healing through mouse gestures was available, some players would hide in a corner and heal. We wanted a more active playing experience, so we removed the gestures and added the life-stealing powerup to help with healing instead.
