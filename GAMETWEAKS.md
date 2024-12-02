# Gameplay Tweaks

## Tweak character speed

Change:

Player speed = 100 -> 250
Melee enemy speed = 100 -> 176
Ranged enemy speed = 50 -> 125

With the size of the room, we increased the player speed to make room navigation a bit easier while not making the game too easy. Enemies were a bit too slow to be challenging.

## Nerf direct projectile attacks

Change:

Player 0-bounce hit damage = 10 -> 1

Since the game is called Ricochet Rage, the goal is to do cool ricochets to kill enemies. However, it was still somewhat easy to play the game while aiming directly at enemies. This change is designed to make it close to impossible to win by playing this way.

## Buff boss health

Change:

Cowboy Boss Health: 300 -> 1000
Necromancer Boss Health: 300 -> 1200

The bosses in our game felt too easy to kill and too similar to the other enemies. This change makes them feel more boss-like.

## Nerf sight line of ranged enemies:

Change:

Ranged enemy max sight distance: Unlimited -> 350

Some of the hardest levels in our game were the ones where ranged enemies could spawn, and they were already quite slow. Multiple ranged enemies could shoot at you even without appearing in the camera view, which was extremely challenging to deal with and resulted in too many bullets to avoid. Therefore, we reduced the distance ranged enemies can see so that they need to be closer to the player to fire. 

## Reduce corridor spawn chance

Change:

Corridor spawn frequency absolute value: 1 -> 0.25

Our random room generation has multiple room feature types, one being the "corridor", or a narrow hallway-like area. The trouble is, it is quite easy to"camp" these corridors and get multi-bounce ricochets off the walls into incoming enemies, which combined with the multi-bounces' extremely high damage value makes the level almost trivial. The multi-bounce ricochets are intended to be difficult to execute, and these corridors make it too easy to do them, so they have become much rarer.

## Reduce ranged projectile per aim
Change:

Everytime the enemy stops to aim, they will now only be able to shoot once rather than three times per aim.

Ranged enemies were much harder to deal with than melee enemies, so we reduced the number of projectiles they can fire to make them more comparable in difficulty to melee.

## Cap total enemies in the room

Change:

LevelStruct Format: Level num, numMelees, numRanged, numBosses, maxActiveMelee, maxActiveRanged, waveSize, waveSpawnFrequency

Before:
    LevelStruct level_1 = {1, 5, 0, 0, 1, 5, 500};
    LevelStruct level_2 = {2, 0, 5, 0, 1, 5, 450};
    LevelStruct level_3 = {3, 10, 10, 0, 1, 20, 450};
    LevelStruct level_4 = {4, 15, 15, 0, 1, 30, 400};
    LevelStruct level_5 = {5, 15, 15, 1, 1, 30, 400};
    LevelStruct level_6 = {6, 20, 20, 0, 1, 40, 400};
    LevelStruct level_7 = {7, 25, 25, 0, 1, 50, 350};
    LevelStruct level_8 = {8, 30, 30, 0, 1, 60, 350};
    LevelStruct level_9 = {9, 40, 40, 0, 1, 80, 300};
    LevelStruct level_10 = {10, 50, 50, 0, 250};

Now:
    LevelStruct level_1 = {1, 5, 0, 0, 2, 0, 2, 5000};
    LevelStruct level_2 = {2, 0, 5, 0, 0, 2, 2, 4000};
    LevelStruct level_3 = {3, 10, 10, 0, 3, 2, 3, 5000};
    LevelStruct level_4 = {4, 10, 10, 0, 5, 2, 4, 3000};
    LevelStruct level_5 = {5, 0, 0, 1, 1, 0, 0, 250};
    LevelStruct level_6 = {6, 15, 15, 0, 8, 3, 4, 5000};
    LevelStruct level_7 = {7, 15, 15, 0, 10, 3, 4, 5000};
    LevelStruct level_8 = {8, 15, 15, 0, 10, 4, 5, 5000};
    LevelStruct level_9 = {9, 15, 15, 0, 10, 5, 5, 5000};
    LevelStruct level_10 = {10, 0, 0, 1, 1, 0, 0, 250};

We set some initial max enemy count values for initial play testing, but it became quickly clear that there were too many enemies for even the devs to deal with. These tweaks should make the game actually possible by limiting how many enemies can be spawned at any given time. 
