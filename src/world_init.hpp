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

void SaveGameToFile(RenderSystem *renderer);
bool LoadGameFromFile(RenderSystem *renderer);
bool doesSaveFileExist(RenderSystem *renderer);

void GenerateMap(RenderSystem *renderer, int seed);
// the player
Entity createPlayer(RenderSystem *renderer, vec2 pos);

// the prey
Entity createMeleeEnemy(RenderSystem *renderer, vec2 position);
Entity createRangedEnemy(RenderSystem *renderer, vec2 position);
Entity createBossEnemy(RenderSystem *renderer, vec2 position);

// the game walls
Entity createWall(RenderSystem *renderer, vec2 position, vec2 size, float angle = 0);

Entity createHealthBar(RenderSystem *renderer, vec2 position, vec2 size);

Entity createProjectile(RenderSystem *renderer, vec2 pos, float angle, bool is_player_projectile, float speed = 500);

Entity createInvincibilityPowerUp(RenderSystem *renderer, vec2 position);

Entity createText(RenderSystem *renderer, std::string text, vec2 position, float scale, vec3 color);
