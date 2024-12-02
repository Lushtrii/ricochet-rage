#pragma once

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"
#include <fstream>

// These are hardcoded to the dimensions of the entity texture
// BB = bounding box
const float PLAYER_BB_HEIGHT = 0.6f * 165.f;
const float PLAYER_BB_WIDTH = 0.6f * 165.f;

const float ENEMY_BB_HEIGHT = 0.6f * 165.f; // 1001
const float ENEMY_BB_WIDTH = 0.6f * 165.f;  // 870

const float PROJECTILE_BB_HEIGHT = 0.6f * 150.f;
const float PROJECTILE_BB_WIDTH = 0.6f * 75.f;

const float POWERUP_BB_HEIGHT = 100;
const float POWERUP_BB_WIDTH = 100;

extern LevelStruct* currLevelStruct;
// level num, num of melee, num of ranged, num of boss, ms between spawns
extern LevelStruct level_1; 
extern LevelStruct level_2; 
extern LevelStruct level_3; 
extern LevelStruct level_4; 
extern LevelStruct level_5; 
extern LevelStruct level_6; 
extern LevelStruct level_7; 
extern LevelStruct level_8; 
extern LevelStruct level_9; 
extern LevelStruct level_10; 
extern LevelStruct *levels[10];


enum TT // TT = TileType
{
    BEND,
    CORNER,
    CORRIDOR,
    DOOR,
    EMPTY,
    SIDE,
    T,
    TURN,
    WALL,
    WAY4,
};

struct pair_hash
{
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2> &p) const
    {
        // Combine the hashes of the two elements
        std::size_t h1 = std::hash<T1>{}(p.first);
        std::size_t h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1); // Combine hashes (bit-shift and XOR)
    }
};

void initLevels();

void SaveGameToFile(RenderSystem *renderer);
void writePart(std::ofstream &f, ComponentContainer<Motion> *container);
bool LoadGameFromFile(RenderSystem *renderer);
bool doesSaveFileExist(RenderSystem *renderer);

void NextRoom(RenderSystem *renderer, int seed);
void GenerateMap(RenderSystem *renderer, int seed);
Entity createTile(RenderSystem *renderer, vec2 pos, vec2 size, TT type);
void createGridNode(std::vector<std::vector<GridNode>> &gridMap, vec2 pos, vec2 size, int value);
// the player
Entity createPlayer(RenderSystem *renderer, vec2 pos);

Entity createMeleeEnemy(RenderSystem *renderer, vec2 position);

Entity createRangedEnemy(RenderSystem *renderer, vec2 position);

Entity createCowboyBossEnemy(RenderSystem *renderer, vec2 position);

// the game walls
Entity createWall(RenderSystem *renderer, vec2 position, vec2 size, float angle = 0);

Entity createHealthBar(RenderSystem *renderer, vec2 position, vec2 size, bool isPlayer);

Entity createProjectile(RenderSystem *renderer, vec2 pos, float angle, bool is_player_projectile, float speed = 500);

Entity createInvincibilityPowerUp(RenderSystem *renderer, vec2 position);

Entity createSuperBulletsPowerUp(RenderSystem *renderer, vec2 position);

Entity createHealthStealerPowerUp(RenderSystem *renderer, vec2 position);

Entity createText(RenderSystem *renderer, std::string text, vec2 position, float scale, vec3 color);

Entity createText(RenderSystem *renderer, std::string text, vec2 position, float scale, vec3 color, bool timed);

Entity createNecromancerEnemy(RenderSystem *renderer, vec2 position);

Entity createMeleeMinion(RenderSystem *renderer, vec2 position);

Entity createRangedMinion(RenderSystem *renderer, vec2 position);
