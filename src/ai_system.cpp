// internal
#include "ai_system.hpp"
#include "world_init.hpp"
#include <queue>

void AISystem::init(RenderSystem *renderSystem) {
	this->renderer_arg = renderSystem;
}

void AISystem::step(float elapsed_ms)
{
	// Find a player is it exists, else, do nothing
	if (registry.players.size() < 1)
	{
		return;
	}
	Entity &playerEntity = registry.players.entities[0];
	if (!registry.motions.has(playerEntity))
	{
		return;
	}
	Motion &playerMotion = registry.motions.get(playerEntity);

	for (Entity &enemy : registry.enemies.entities)
	{
		// State for roaming
		EnemyState& enemyState = registry.enemies.get(enemy).enemyState;
		if (enemyState == EnemyState::ROAMING) {
			if (registry.motions.has(enemy)) {
				Motion& motion = registry.motions.get(enemy);
				motion.velocity = vec2((uniform_dist(rng) - 0.5f)* 15.0f, (uniform_dist(rng) - 0.5f) * 15.0f);
				if (length(playerMotion.position - motion.position) < aggroDistance) {
					enemyState = EnemyState::PURSUING;
				}
			}
		// State for pursuing and shooting at player
		} else if (enemyState == EnemyState::PURSUING) {
			if (registry.bosses.has(enemy) && registry.reloadTimes.has(enemy) && registry.meleeAttacks.has(enemy)) {
				boss_enemy_pursue(enemy, elapsed_ms, playerMotion, enemyState);
            }
			else if (registry.reloadTimes.has(enemy))
			{
                ranged_enemy_pursue(enemy, elapsed_ms, playerMotion, enemyState);
            } 
			else if (registry.meleeAttacks.has(enemy))
			{
				Motion& enemyMotion = registry.motions.get(enemy);
				if (length(playerMotion.position - enemyMotion.position) > meleeDistance) {
					Pathfinder& pathfinder = registry.pathfinders.get(enemy);
					chase_with_a_star(pathfinder, elapsed_ms, playerMotion, enemyMotion);
				} else {
					enemyState = EnemyState::ATTACK;
				}
            }
		// State for avoiding obstacles MAY NOT NEED TO USE
		} else if (enemyState == EnemyState::AVOIDWALL) {
			if (registry.motions.has(enemy)) {
				Motion& enemyMotion = registry.motions.get(enemy);
				for (Entity &wall: registry.walls.entities) {
					Motion& wallMotion = registry.motions.get(wall);
					// If in collision course with the wall, go around it
					vec2 wallEnemyDelta = enemyMotion.position - wallMotion.position;
					if (length(abs(wallEnemyDelta)) > distanceToWalls) {
						printf("Distance to wall: %f %f\n", length(abs(wallEnemyDelta)), distanceToWalls);
						enemyState = EnemyState::PURSUING;
					}
				}
			}
		// State for attacking player
		} else if (enemyState == EnemyState::ATTACK) {
			if (registry.bosses.has(enemy) && registry.reloadTimes.has(enemy) && registry.meleeAttacks.has(enemy)) {
				Motion& enemyMotion = registry.motions.get(enemy);
				if (length(playerMotion.position - enemyMotion.position) < meleeDistance) {
					MeleeAttack &meleeAttack = registry.meleeAttacks.get(enemy);
					stop_and_melee(enemy, meleeAttack, elapsed_ms, playerMotion, playerEntity);
					enemyState = EnemyState::PURSUING;
				} else {
					ReloadTime &counter = registry.reloadTimes.get(enemy);
					stop_and_shoot(enemy, counter, elapsed_ms, playerMotion, !registry.necromancers.has(enemy));
				}
			}
			else if (registry.reloadTimes.has(enemy)) {
				ReloadTime &counter = registry.reloadTimes.get(enemy);
				stop_and_shoot(enemy, counter, elapsed_ms, playerMotion, false);
			} else if (registry.meleeAttacks.has(enemy)) {
				MeleeAttack &meleeAttack = registry.meleeAttacks.get(enemy);
				stop_and_melee(enemy, meleeAttack, elapsed_ms, playerMotion, playerEntity);
				enemyState = EnemyState::PURSUING;
			}
		} else if (enemyState == EnemyState::TELEPORTING) {
			if (registry.bosses.has(enemy)) {
				Teleporter& bossTeleport = registry.teleporters.get(enemy);
				Motion& enemyMotion = registry.motions.get(enemy);
				if (!registry.teleporting.has(enemy)) {
					Teleporting& teleporting = registry.teleporting.emplace(enemy);
					teleporting.starting_time = 0;
					bossTeleport.prevScale = enemyMotion.scale;
				}
				enemyMotion.velocity = vec2(0.0f, 0.0f);
				if (bossTeleport.animation_time > 0) {
					bossTeleport.animation_time -= elapsed_ms;
				} else {
					teleport_boss(enemy, playerMotion, enemyState);
					// Restore enemy motion
					enemyMotion.scale = bossTeleport.prevScale;
					registry.teleporting.remove(enemy);
					bossTeleport.animation_time = bossTeleport.max_teleport_time;
				}
            }
		} else if (enemyState == EnemyState::SPAWN_MINIONS) {
			Motion &enemyMotion = registry.motions.get(enemy);

			Necromancer& necroComp = registry.necromancers.get(enemy);
			necroComp.centerPosition = enemyMotion.position;
			necroComp.spawningMinions = true;

			// Reset time
            ReloadTime &counter = registry.reloadTimes.get(enemy);
            counter.counter_ms = original_ms;
			enemyState = EnemyState::PURSUING;
		}
	}

	// Spawn minions outside of loop, or it will crash
	for (Entity &necro: registry.necromancers.entities) {
		Necromancer& necroComp = registry.necromancers.get(necro);
		if (necroComp.spawningMinions) {
			spawn_minions(necroComp.centerPosition);
			necroComp.spawningMinions = false;
		}
	}

	// Deal with teleportation animation with Bezier Curve
	for (Entity& teleporting: registry.teleporting.entities) {
		Motion &bossMotion = registry.motions.get(teleporting);
		Teleporting &teleportingComp = registry.teleporting.get(teleporting);
		bossMotion.scale = bossMotion.scale * quadratic_bezier(teleportingComp.starting_time, teleportingComp.max_time);
		teleportingComp.starting_time += elapsed_ms;
	}
}

// Prevent collision with obstacles
bool wall_distance_helper(vec2 &position) {
	for (Entity &wall: registry.walls.entities) {
		Motion& wallMotion = registry.motions.get(wall);
		if (length(position - wallMotion.position) <  75.f) {
			return false;
		}
	}
	return true;
}

void AISystem::spawn_minions(glm::vec2 &position)
{
	vec2 possiblePositions[4] = {vec2(position.x - minionDistance, position.y),
		vec2(position.x + minionDistance, position.y),
		vec2(position.x, position.y - minionDistance),
		vec2(position.x, position.y + minionDistance)};

	for (int i = 0; i < 4; i++) {
		if (wall_distance_helper(possiblePositions[i])) {
			int minionTypeRand = rand() % 2;
			if (minionTypeRand == 0) {
				createMeleeMinion(renderer_arg, possiblePositions[i]);
			} else {
				createRangedMinion(renderer_arg, possiblePositions[i]);
			}
		}
	}
}

void AISystem::teleport_boss(Entity &enemy, Motion &playerMotion, EnemyState &enemyState)
{
    bool is_valid_spawn = false;
    vec2 spawn_pos;
    Motion &enemyMotion = registry.motions.get(enemy);

    while (!is_valid_spawn)
    {
        spawn_pos = {
            150.f + uniform_dist(rng) * (window_width_px - 300.f),
            150.f + uniform_dist(rng) * (window_height_px - 300.f)};
        is_valid_spawn = true;

        // Check if it collides with the player
        if (length(playerMotion.position - spawn_pos) < 100.0f)
        {
            is_valid_spawn = false;
        }

        for (Entity entity : registry.walls.entities)
        {
            Motion &wall_motion = registry.motions.get(entity);
            if (length(wall_motion.position - spawn_pos) < 100.f)
            {
                is_valid_spawn = false;
                break;
            }
        }
    }
    enemyMotion.position = spawn_pos;
	Pathfinder& pathfinder = registry.pathfinders.get(enemy);
	update_path(playerMotion, enemyMotion, pathfinder);
	if (registry.necromancers.has(enemy)) {
		enemyState = EnemyState::SPAWN_MINIONS;
	} else {
		enemyState = EnemyState::ATTACK;
	}
}

// Pursuing logic for a ranged enemy, including shoot
void AISystem::ranged_enemy_pursue(Entity &enemy, float elapsed_ms, Motion &playerMotion, EnemyState &enemyState)
{
    ReloadTime &counter = registry.reloadTimes.get(enemy);
    // to prevent overflow
    if (counter.counter_ms > 0)
    {
        counter.counter_ms -= elapsed_ms;
    }
    // context_chase(enemy, playerMotion);
	Pathfinder &pathfinder = registry.pathfinders.get(enemy);
	Motion& enemyMotion = registry.motions.get(enemy);
	// update the path with A* every few seconds
    chase_with_a_star(pathfinder, elapsed_ms, playerMotion, enemyMotion);


    if (!line_of_sight_check(enemy, playerMotion) && counter.counter_ms < 0)
    {
        enemyState = EnemyState::ATTACK;
    }
}

void AISystem::boss_enemy_pursue(Entity &enemy, float elapsed_ms, Motion &playerMotion, EnemyState &enemyState)
{
    ReloadTime &counter = registry.reloadTimes.get(enemy);
    // to prevent overflow
    if (counter.counter_ms > 0)
    {
        counter.counter_ms -= elapsed_ms;
    }

	Motion& enemyMotion = registry.motions.get(enemy);

    // context_chase(enemy, playerMotion);
	Pathfinder &pathfinder = registry.pathfinders.get(enemy);

	// update the path with A* every few seconds
    chase_with_a_star(pathfinder, elapsed_ms, playerMotion, enemyMotion);

	int attackRand = rand() % 2;
	if (attackRand == 0 && counter.counter_ms < 0) {
		enemyState = EnemyState::TELEPORTING;
	}
    if ((!line_of_sight_check(enemy, playerMotion) && counter.counter_ms < 0) || length(playerMotion.position - enemyMotion.position) < meleeDistance)
    {
		enemyState = EnemyState::ATTACK;
    }
}

void AISystem::chase_with_a_star(Pathfinder &pathfinder, float elapsed_ms, Motion &playerMotion, Motion &enemyMotion)
{
    if (pathfinder.refresh_rate > 0)
    {
        pathfinder.refresh_rate -= elapsed_ms;
    }
    else
    {
        update_path(playerMotion, enemyMotion, pathfinder);

        // reset timer
        pathfinder.refresh_rate = pathfinder.max_refresh_rate;
    }
	interpolate_pathfinding(enemyMotion, pathfinder, playerMotion);
}

void AISystem::update_path(Motion &playerMotion, Motion &enemyMotion, Pathfinder &pathfinder)
{
	if (registry.gridMaps.size() <= 0) {
		return;
	}
    auto &gridEntity = registry.gridMaps.entities[0];
    GridMap &gridMapComp = registry.gridMaps.get(gridEntity);
    auto &gridMap = gridMapComp.gridMap;
	if (gridMap.size() <= 0) {
		return;
	}
    vec2 playerToCoord = playerMotion.position / vec2(gridMapComp.mapWidth, gridMapComp.mapHeight);
	playerToCoord = clamp(playerToCoord, vec2(0.0), vec2(0.99));
    int x_player = (int)floor(playerToCoord.x * gridMap[0].size());
    int y_player = (int)floor(playerToCoord.y * gridMap.size());
    vec2 enemyToCoord = enemyMotion.position / vec2(gridMapComp.mapWidth, gridMapComp.mapHeight);
	enemyToCoord = clamp(enemyToCoord, vec2(0.0), vec2(0.99));
    uint x_enemy = (uint)floor(enemyToCoord.x * gridMap[0].size());
    uint y_enemy = (uint)floor(enemyToCoord.y * gridMap.size());
    // printf("player x: %d \n", x_player);
    // printf("player y: %d \n", y_player);
    // printf("%d: enemy y \n", y_enemy);
    // printf("%d: enemy x \n", x_enemy);
    astar_pathfinding(gridMapComp, &gridMap[y_enemy][x_enemy], &gridMap[y_player][x_player], pathfinder);
}

// Perform a light of sight check to see if there are any obstacles between the ranged enemy and the player
bool AISystem::line_of_sight_check(Entity &enemy, Motion &playerMotion) {
	Motion& enemyMotion = registry.motions.get(enemy);
	vec2 deltaEnemyPlayer = enemyMotion.position - playerMotion.position;
	for (Entity &obstacle: registry.walls.entities) {
		Motion &wallMotion = registry.motions.get(obstacle);
		if (line_box_collision(enemyMotion, wallMotion, deltaEnemyPlayer)) {
			return true;
		}
	}
	return false;
}

// Whether the line and AABB intersect, not really the smartest method, iterate through points on the line and return true if it intersects
bool AISystem::line_box_collision(Motion &enemyMotion, Motion &obstacleMotion, vec2 &directionDelta) {
	vec2 normalizedDelta = normalize(directionDelta);
	vec2 increment = los_increment * normalizedDelta;
	vec2 curr = vec2(0, 0);
	vec2 bounding_box = vec2(abs(obstacleMotion.scale.x), abs(obstacleMotion.scale.y));
	float left = obstacleMotion.position.x - bounding_box.x/2;
	float right = obstacleMotion.position.x + bounding_box.x/2;
	float bot = obstacleMotion.position.y + bounding_box.y/2;
	float top = obstacleMotion.position.y - bounding_box.y/2;
	// Added to prevent infinite loops
	int tries = 0;
	while ((length(curr) < length(directionDelta)) && tries < 1000) {
		curr += increment;
		tries += 1;
		vec2 curr_pos = enemyMotion.position + curr;
		if (curr_pos.x > left && curr_pos.x < right && curr_pos.y > top && curr_pos.y < bot) {
			return true;
		}
	}
	return false;
}

// Comparator
struct CompareCosts {
	bool operator()(GridNode* gn1, GridNode* gn2) {
		return gn1->fCost() > gn2->fCost();
	}
};

// Inspired by https://www.geeksforgeeks.org/a-search-algorithm/
// A star for smart chasing, requires grid system to work
void AISystem::astar_pathfinding(GridMap& grid, GridNode* startNode, GridNode* endNode, Pathfinder &pathfinder) {
	auto &gridMap = grid.gridMap;
	std::priority_queue<GridNode*, std::vector<GridNode*>, CompareCosts> openSet;
	std::vector<std::vector<bool>> closedSet(gridMap.size(), std::vector<bool>(gridMap[0].size(), false));
	startNode->gCost = 0.0f;
	openSet.push(startNode);
	while(!openSet.empty()) {
		// Get the top of the queue
		GridNode* curr = openSet.top();
		openSet.pop();

		// Create the path
		// Within one block away
		bool withinRange = 
			(curr->coord == endNode->coord) || 
			(abs(curr->coord.x - endNode->coord.x) == 1 && curr->coord.y == endNode->coord.y) || 
			(abs(curr->coord.y - endNode->coord.y) == 1 && curr->coord.x == endNode->coord.x) || 
			(abs(curr->coord.x - endNode->coord.x) == 1 && abs(curr->coord.y - endNode->coord.y) == 1);
		if (withinRange) {
			std::vector<GridNode*> path;
			GridNode* tp = curr;
			while (tp != nullptr) {
                path.push_back(tp);
                tp = tp->parentNode;
            }
            std::reverse(path.begin(), path.end());
			// Remove the first element to prevent jittering
			path.erase(path.begin());
			pathfinder.path = path;
			reset_grid(grid);
			return;
		}

		closedSet[curr->coord.y][curr->coord.x] = true;
		
		// Possible directions
		std::vector<ivec2> directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}, {1, 1}, {1,-1}, {-1, -1}, {-1, 1}};
		for (auto &direction: directions) {
			ivec2 nextCoord = curr->coord + direction;

            if (nextCoord.x < 0 || nextCoord.y < 0 || nextCoord.x >= gridMap[0].size() || nextCoord.y >= gridMap.size())
                continue;

			GridNode* neighbor = &grid.gridMap[nextCoord.y][nextCoord.x];

			// Give enemy some space
			bool notWalkable = neighbor->notWalkable;
			for (auto &wallDirection: directions) {
				ivec2 nextNextCoord = nextCoord + wallDirection;
				if (nextNextCoord.x < 0 || nextNextCoord.y < 0 || nextNextCoord.x >= gridMap[0].size() || nextNextCoord.y >= gridMap.size())
                	continue;
				GridNode* neighborNeighbor = &grid.gridMap[nextNextCoord.y][nextNextCoord.x];
				if (neighborNeighbor->notWalkable) {
					notWalkable = true;
				}
				if (!neighbor->notWalkable && (neighborNeighbor->coord == endNode->coord)) {
					notWalkable = false;
					break;
				}
			}

			// Avoid walls and previously visited
			if (notWalkable || closedSet[nextCoord.y][nextCoord.x]) {
				continue;
			}

			float tempGCost = curr->gCost + 1.0f;
			if (tempGCost < neighbor->gCost) {
				neighbor->gCost = tempGCost;
				neighbor->hCost = length((vec2) nextCoord - (vec2) endNode->coord);
				neighbor->parentNode = curr;
				openSet.push(neighbor);
			}
		}
	}
	reset_grid(grid);
}

void AISystem::reset_grid(GridMap &gridMap) {
	auto &gridMatrix = gridMap.gridMap;
	for (int i = 0; i < gridMatrix[0].size(); i++) {
		for (int j = 0; j < gridMatrix.size(); j++) {
			auto &gridNode = gridMatrix[j][i];
			gridNode.gCost = 1e9;
			gridNode.hCost = 0.0f;
			gridNode.parentNode = nullptr;
		}
	}
}

// Go along the path
void AISystem::interpolate_pathfinding(Motion &enemyMotion, Pathfinder &pathfinder, Motion &playerMotion) {
	if (pathfinder.path.size() > 0) {
		auto &closestPoint = pathfinder.path.front();
		vec2 &gridPosition = closestPoint->position;
		vec2 delta = enemyMotion.position - gridPosition;
		vec2 direction = normalize(gridPosition - enemyMotion.position);
		enemyMotion.velocity = direction * meleeEnemySpeed;
		vec2 angleDelta = normalize(enemyMotion.position - playerMotion.position);
		enemyMotion.angle = atan2(-angleDelta.y, -angleDelta.x);
		if (length(delta) < 2.0f) {
			pathfinder.path.erase(pathfinder.path.begin()); 
		}
	} else {
		enemyMotion.velocity = {0.0f, 0.0f};
	}
}

// DEPRECATED
// For smart context chasing, good for avoiding obstacles, although still robotic
void AISystem::context_chase(Entity &enemy,  Motion &playerMotion) {
	std::vector<vec2> directions = {vec2(0, 1), vec2(0, -1), vec2(1, 0), vec2(-1, 0), normalize(vec2(-1, 1)), normalize(vec2(1,1)), normalize(vec2(-1,-1)), normalize(vec2(1,-1))};

	Motion &enemyMotion = registry.motions.get(enemy);
    float enemySpeed = 0.f;
    if (registry.meleeAttacks.has(enemy)) {
        enemySpeed = meleeEnemySpeed;
    }
    else {
        enemySpeed = rangedEnemySpeed;
    }

	vec2 enemyPlayerDelta = enemyMotion.position - playerMotion.position;
	// Stop enemy from colliding with player
	if (length(enemyPlayerDelta) < minDistanceToPlayer) {
		enemyMotion.velocity = vec2(0.0f, 0.0f);
		return;
	}
	vec2 angleVector = -normalize(enemyPlayerDelta);
	std::vector<float> interestVector;
	for (auto &direction: directions) {
		interestVector.push_back(direction.x * angleVector.x + direction.y * angleVector.y);
	}
	std::vector<float> dangerVector(interestVector.size());

	float minDistance = std::numeric_limits<float>::max();
	for (Entity &wall: registry.walls.entities) {
		Motion& wallMotion = registry.motions.get(wall);
		vec2 wallEnemyDelta = enemyMotion.position - wallMotion.position;
		float distanceToWall = length(wallEnemyDelta);
		
		// move away from the wall if it is the closest
		if (distanceToWall < minDistance) {
			minDistance = distanceToWall;

			vec2 danger_delta = -normalize(wallEnemyDelta) * obstacleForce / (distanceToWall - max(wallMotion.scale.x, wallMotion.scale.y));
			for (int i = 0; i < (int)dangerVector.size(); i++) {
				dangerVector[i] = danger_delta.x * directions[i].x + directions[i].y * danger_delta.y;
			}
		}
	}

	// Avoid collisions with other enemies, when too close
	for (Entity &other: registry.enemies.entities) {
		if (other != enemy) {
			Motion &otherMotion = registry.motions.get(other);
			vec2 enemyEnemyDelta = enemyMotion.position - otherMotion.position;
			float distanceToEnemy = length(enemyEnemyDelta);
			if (distanceToEnemy < distanceBetweenEnemies) {
				vec2 danger_delta = -normalize(enemyEnemyDelta) * enemyForce / distanceToEnemy;
				for (int i = 0; i < (int)dangerVector.size(); i++) {
					dangerVector[i] += danger_delta.x * directions[i].x + directions[i].y * danger_delta.y;
				}
			}
		}
	}

	// Context Vector
	vec2 sumVelocity = vec2(0,0);
	std::vector<float> contextVector(interestVector.size());
	for (int i = 0; i < (int)interestVector.size(); i++) {
		contextVector[i] = interestVector[i] - dangerVector[i];
		sumVelocity += contextVector[i] * directions[i];
	}
	enemyMotion.velocity = normalize(sumVelocity) * enemySpeed;
	enemyMotion.angle = atan2(-enemyPlayerDelta.y, -enemyPlayerDelta.x);
}

// Stop, winds up, and performs a melee attack on the player
void AISystem::stop_and_melee(Entity &enemy, MeleeAttack &counter, float elapsed_ms, Motion &playerMotion, Entity &playerEntity) {
	if (registry.motions.has(enemy)) 
	{
		counter.windup -= elapsed_ms;
		Motion& enemyMotion = registry.motions.get(enemy);
		enemyMotion.velocity = vec2(0.0f, 0.0f);

		bool causeDamage = true;

		for (Entity entity : registry.powerUps.entities) {
			PowerUp &powerUp = registry.powerUps.get(entity);
			if (powerUp.active && powerUp.type == PowerUpType::INVINCIBILITY) {
				causeDamage = false;
			}
		}

		if (counter.windup < 0) {
			if (registry.healths.has(playerEntity ) && causeDamage) {
				Health &playerHealth = registry.healths.get(playerEntity);
				Motion &playerMotion = registry.motions.get(playerEntity);
				playerHealth.value -= counter.damage;
                int w, h;
                glfwGetWindowSize(renderer_arg->getWindow(), &w, &h);
                int cameraOffsetX = w/2 - playerMotion.position.x;
                // Motion.position assumes top right is (window_width_px, window_height_px) when the y axis is actually flipped, so negative offset
                int cameraOffsetY = -(h/2 - (h - playerMotion.position.y));
                vec2 cameraOffset = vec2(cameraOffsetX, cameraOffsetY);
				createText(renderer_arg, "-" + std::to_string(counter.damage), playerMotion.position + cameraOffset, 1.25f, {1.f, 0.f, 0.133f});
				if (registry.damageEffect.has(playerEntity)) {
					DamageEffect &effect = registry.damageEffect.get(playerEntity);
					effect.is_attacked = true;
				}
			}
			counter.windup = counter.windupMax;
		}
	}
}

// Stops and shoots at the enemy at a certain rate
void AISystem::stop_and_shoot(Entity &enemy, ReloadTime &counter, float elapsed_ms, Motion &playerMotion, bool boss)
{
    if (registry.motions.has(enemy))
    {
        counter.take_aim_ms -= elapsed_ms;
		counter.shoot_rate -= elapsed_ms;
        Motion &enemyMotion = registry.motions.get(enemy);
        enemyMotion.velocity = vec2(0.0f, 0.0f);

		if (counter.shoot_rate < 0) {
			if (boss) {
				shotgun_enemy(enemyMotion, playerMotion, counter);
			} 
			else 
			{
				single_shot_enemy(enemyMotion, playerMotion, counter);
			}
        }

        if (counter.take_aim_ms < 0)
        {
			Enemy &enemyState = registry.enemies.get(enemy);
			counter.shoot_rate = shoot_rate;
            counter.counter_ms = original_ms;
            counter.take_aim_ms = take_aim_ms;
			enemyState.enemyState = EnemyState::PURSUING;
        }
    }
}

// Do a single shot at the player
void AISystem::single_shot_enemy(Motion &enemyMotion, Motion &playerMotion, ReloadTime &counter)
{
    vec2 angleVector = normalize(enemyMotion.position - playerMotion.position);
    float angle = atan2(angleVector.y, angleVector.x);
    createProjectile(renderer_arg, enemyMotion.position, angle, false);
    counter.shoot_rate = shoot_rate;
};

// Do a spread out shotgun shot at the player
void AISystem::shotgun_enemy(Motion &enemyMotion, Motion &playerMotion, ReloadTime &counter)
{
    vec2 angleVector = normalize(enemyMotion.position - playerMotion.position);
    float angle = atan2(angleVector.y, angleVector.x);
	float aim_angle = atan2(-angleVector.y, -angleVector.x);
	enemyMotion.angle = aim_angle;
    createProjectile(renderer_arg, enemyMotion.position, angle, false);
	createProjectile(renderer_arg, enemyMotion.position, angle + shotgun_angle, false);
	createProjectile(renderer_arg, enemyMotion.position, angle - shotgun_angle, false);
    counter.shoot_rate = shoot_rate;
};

// DEPRECATED: Extremely simple chase that goes to the players direction, does the Roomba thing when approaching a wall
void AISystem::simple_chase(float elapsed_ms, Motion &playersMotion)
{
	elapsed_ms += 0.f; // to hide errors
	for (uint i = 0; i < registry.enemies.size(); i++)
	{
		Entity &curr_entity = registry.enemies.entities[i];
		simple_chase_enemy(curr_entity, playersMotion);
	}
}

// DEPRECATED: Extremely simple chase that goes to the players direction
void AISystem::simple_chase_enemy(Entity &enemy, Motion &playersMotion)
{
	if (registry.motions.has(enemy))
	{
		Motion &enemyMotion = registry.motions.get(enemy);
        float enemySpeed = 0.f;
        if (registry.meleeAttacks.has(enemy)) {
            enemySpeed = meleeEnemySpeed;
        }
        else {
            enemySpeed = rangedEnemySpeed;
        }

		for (Entity &wall: registry.walls.entities) {
			Motion& wallMotion = registry.motions.get(wall);
			vec2 wallEnemyDelta = enemyMotion.position - wallMotion.position;
			// Go directly at the player
			vec2 angleVector = -normalize(enemyMotion.position - playersMotion.position + followingConstant * playersMotion.velocity);
			float angle = atan2(angleVector.y, angleVector.x);
            enemyMotion.velocity = angleVector * enemySpeed;
			enemyMotion.angle = angle;

			if (length(abs(wallEnemyDelta)) < distanceToWalls) {
				Enemy &enemyComp = registry.enemies.get(enemy);
				enemyComp.enemyState = EnemyState::AVOIDWALL;
				printf("State %d", (int)enemyComp.enemyState);
				if (angleVector.x < angleVector.y) {
					enemyMotion.velocity = vec2(angleVector.x * enemySpeed, 0.0f);
				} else {
					enemyMotion.velocity = vec2(0.0f, angleVector.y * enemySpeed);
				}
			}
		}
	}
};

// Apply Quadratic Bezier multiplier for the boss
vec2 AISystem::quadratic_bezier(float t, float max_time) {
	float t_norm = t/max_time;
	float not_t = min((max_time - t)/max_time, 1.0f);

	// Starting position is 0.5
	vec2 p0 = vec2(0.7f, 0.7f);
	vec2 p1 = vec2(0.3f, 3.0f);
	vec2 p2 = vec2(0.2, 0.2f);
	vec2 bezier_calc = not_t * not_t * p0 + 2 * not_t * t_norm * p1 + t_norm * t_norm * p2;
	return bezier_calc;
};
