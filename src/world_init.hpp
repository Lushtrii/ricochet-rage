#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"

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

void SaveGameToFile(RenderSystem *renderer);
bool LoadGameFromFile(RenderSystem *renderer);
bool doesSaveFileExist(RenderSystem *renderer);

void NextRoom(RenderSystem *renderer, int seed);
void GenerateMap(RenderSystem *renderer, int seed);
void createTile(RenderSystem *renderer, vec2 pos, vec2 size, TT type);
// the player
Entity createPlayer(RenderSystem *renderer, vec2 pos);

Entity createMeleeEnemy(RenderSystem *renderer, vec2 position);

Entity createRangedEnemy(RenderSystem *renderer, vec2 position);

Entity createCowboyBossEnemy(RenderSystem *renderer, vec2 position);

// the game walls
Entity createWall(RenderSystem *renderer, vec2 position, vec2 size, float angle = 0);

Entity createHealthBar(RenderSystem *renderer, vec2 position, vec2 size);

Entity createProjectile(RenderSystem *renderer, vec2 pos, float angle, bool is_player_projectile, float speed = 500);

Entity createInvincibilityPowerUp(RenderSystem *renderer, vec2 position);

Entity createSuperBulletsPowerUp(RenderSystem *renderer, vec2 position);

Entity createHealthStealerPowerUp(RenderSystem *renderer, vec2 position);

Entity createText(RenderSystem *renderer, std::string text, vec2 position, float scale, vec3 color);

Entity createText(RenderSystem *renderer, std::string text, vec2 position, float scale, vec3 color, bool timed);

Entity createNecromancerEnemy(RenderSystem *renderer, vec2 position);

Entity createMeleeMinion(RenderSystem *renderer, vec2 position);

Entity createRangedMinion(RenderSystem *renderer, vec2 position);
