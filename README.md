# Team01

Credits:

Title font: https://www.fontspace.com/megamax-jonathan-too-font-f124011

Button font: https://fonts.google.com/specimen/Squada+One

Polygon Triangulation Library: https://github.com/mapbox/earcut.hpp

Shadows implementation inspired by: https://ncase.me/sight-and-light/

Background image: https://wallpapercave.com/pixel-space-wallpapers

Floor texture: https://piiixl.itch.io/textures3

## M3 Features
- Different Game Rooms
    - The game consists of three levels with different enemy types and different number of enemies - clear all levels to win the game.
- Complex Prescribed Motion (Basic Creative Feature)
    - The enemy boss teleports from time to time. The animation for the teleportation applies a Quadratic Bezier Curve multiplier to the scale of the boss. This is non-linear and requires 3 set points. As a result, the boss appears to be warping into nothing when it is teleporting.
- Mouse Gestures (Basic Creative Feature)
    - For the player to heal, the player can now draw on the screen with the right click or left click + left control. The player can draw a heart and heal 50 HP. It shows the HP healed if successful.
- Dynamic Shadows (Advanced Creative Feature)
    - All moving character entities now cast dynamic shadows relative to a light source positioned in the center of the room. Using a ray casting method inspired by https://ncase.me/sight-and-light/, these shadows will update every time an entity moves, and stretch as far as the room walls. 
