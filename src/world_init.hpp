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

// the player
Entity createPlayer(RenderSystem *renderer, vec2 pos);

// the prey
Entity createEnemy(RenderSystem *renderer, vec2 position);

// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);

// the game walls
Entity createWall(RenderSystem *renderer, vec2 position, vec2 size, float angle);

Entity createProjectile(RenderSystem *renderer, vec2 pos, float angle, bool is_player_projectile, float speed = 500);
