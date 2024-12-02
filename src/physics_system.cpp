// internal
#include "physics_system.hpp"
#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"
#include <cstdlib>
#include <iostream>

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

vec2 PhysicsSystem::calculateVertexPos(const Motion& m, const TexturedVertex& tv) {
    Transform tr;
    tr.rotate(m.angle);
    tr.scale(m.scale);
    vec3 transformedPos = tr.mat * tv.position;
    return m.position + vec2(transformedPos.x, transformedPos.y);
}

bool PhysicsSystem::doesMeshCollide(const Motion& meshMotion, const std::vector<TexturedVertex>& meshVertices, const Motion& otherMotion) {
    for (const TexturedVertex& tv : meshVertices) {
        vec2 translatedPos = calculateVertexPos(meshMotion, tv);
        if (isPointInBox(translatedPos, otherMotion)) {
            return true;
        }
    }
    return false;
}

bool PhysicsSystem::isPointInBox(const vec2 point, const Motion& motion) {
    vec2 boundingBox = get_bounding_box(motion);
    float left = motion.position.x - boundingBox.x/2;
    float right = motion.position.x + boundingBox.x/2;
    float top = motion.position.y - boundingBox.y/2;
    float bot = motion.position.y + boundingBox.y/2;

    return (left <= point.x) && (point.x <= right) && (top <= point.y) && (point.y <= bot);
}

// Checks for collision between 2 bounding boxes
bool PhysicsSystem::collides(const Motion& motion1, const Motion& motion2)
{
    float motion1_left = motion1.position.x - abs(motion1.scale.x/2);
    float motion1_right = motion1.position.x + abs(motion1.scale.x/2);
    float motion1_up = motion1.position.y - abs(motion1.scale.y/2);
    float motion1_down = motion1.position.y + abs(motion1.scale.y/2);

    float motion2_left = motion2.position.x - abs(motion2.scale.x/2);
    float motion2_right = motion2.position.x + abs(motion2.scale.x/2);
    float motion2_up = motion2.position.y - abs(motion2.scale.y/2);
    float motion2_down = motion2.position.y + abs(motion2.scale.y/2);

    return motion1_right > motion2_left && motion2_up < motion1_down && motion1_left < motion2_right && motion1_up < motion2_down;
}

void PhysicsSystem::step(float elapsed_ms)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	float step_seconds = elapsed_ms / 1000.f;

    Entity playerEntity = registry.players.entities[0];

	for(uint i = 0; i< motion_registry.size(); i++)
	{
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
        bool isPlayerEntity = playerEntity == entity;

		motion.last_physic_move = vec2(0,0);

        // Optimize by assuming player is the only one with dash
		if (isPlayerEntity) {
			Dash& dash = registry.dashes.get(entity);

			if (dash.remaining_dash_time > 0) {
				motion.last_physic_move += dash.dash_direction * dash.remaining_dash_time * dash.intial_velocity * step_seconds;
				dash.remaining_dash_time -= step_seconds;
			}
			if (dash.charges != dash.max_dash_charges) {
				dash.recharge_timer -= step_seconds;
				if (dash.recharge_timer <= 0) {
					dash.charges++;
				}
			}
		}

		motion.last_physic_move += motion.velocity * step_seconds;	
		motion.position += motion.last_physic_move;
	}

    for (Motion& projectileMotion : registry.projectileMotions.components) {
		projectileMotion.last_physic_move = vec2(0,0);
		projectileMotion.last_physic_move += projectileMotion.velocity * step_seconds;	
		projectileMotion.position += projectileMotion.last_physic_move;
    }

    for (Motion& enemyMotion : registry.enemyMotions.components) {
		enemyMotion.last_physic_move = vec2(0,0);
		enemyMotion.last_physic_move += enemyMotion.velocity * step_seconds;	
		enemyMotion.position += enemyMotion.last_physic_move;
    }

	// Check for collisions between all moving entities
    ComponentContainer<Motion> &motion_container = registry.motions;
    Motion& playerMotion = registry.motions.get(registry.players.entities[0]);

    //Wall collisions
	for(Motion& wallMotion : registry.wallMotions.components)
	{
        //Player motion
        if (collides(playerMotion, wallMotion)) {
            registry.collisions.emplace_with_duplicates(wallMotion.entity, playerMotion.entity);
            registry.collisions.emplace_with_duplicates(playerMotion.entity, wallMotion.entity);

        }

		for(Motion& projectileMotion : registry.projectileMotions.components)
		{
			if (collides(projectileMotion, wallMotion))
			{
                const std::vector<TexturedVertex> meshVertices = registry.meshPtrs.get(projectileMotion.entity)->vertices;
                if (!doesMeshCollide(projectileMotion, meshVertices, wallMotion)) {
                    continue;
                }

				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(projectileMotion.entity, wallMotion.entity);
				registry.collisions.emplace_with_duplicates(wallMotion.entity, projectileMotion.entity);
			}
		}

		for(Motion& enemyMotion : registry.enemyMotions.components)
		{
			if (collides(enemyMotion, wallMotion))
			{
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(enemyMotion.entity, wallMotion.entity);
				registry.collisions.emplace_with_duplicates(wallMotion.entity, enemyMotion.entity);
			}
		}
	}

	for(Motion& enemyMotion : registry.enemyMotions.components)
	{
        //Player motion
        if (collides(enemyMotion, playerMotion)) {
            registry.collisions.emplace_with_duplicates(enemyMotion.entity, playerMotion.entity);
            registry.collisions.emplace_with_duplicates(playerMotion.entity, enemyMotion.entity);

        }
        for (Motion& enemyMotion2 : registry.enemyMotions.components) {
            if (enemyMotion.entity == enemyMotion2.entity) continue;

            if (collides(enemyMotion, enemyMotion2)) {
                registry.collisions.emplace_with_duplicates(enemyMotion.entity, enemyMotion2.entity);
                registry.collisions.emplace_with_duplicates(enemyMotion2.entity, enemyMotion.entity);

            }
        }

		for(Motion& projectileMotion : registry.projectileMotions.components)
		{
			if (collides(projectileMotion, enemyMotion))
			{
                const std::vector<TexturedVertex> meshVertices = registry.meshPtrs.get(projectileMotion.entity)->vertices;
                if (!doesMeshCollide(projectileMotion, meshVertices, enemyMotion)) {
                    continue;
                }
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(projectileMotion.entity, enemyMotion.entity);
				registry.collisions.emplace_with_duplicates(enemyMotion.entity, projectileMotion.entity);
			}
		}
	}

    for(Motion& projectileMotion : registry.projectileMotions.components)
    {
        if (collides(projectileMotion, playerMotion))
        {
            const std::vector<TexturedVertex> meshVertices = registry.meshPtrs.get(projectileMotion.entity)->vertices;
            if (!doesMeshCollide(projectileMotion, meshVertices, playerMotion)) {
                continue;
            }

            // Create a collisions event
            // We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
            registry.collisions.emplace_with_duplicates(projectileMotion.entity, playerMotion.entity);
            registry.collisions.emplace_with_duplicates(playerMotion.entity, projectileMotion.entity);
        }
	}

    for (Entity& e: registry.powerUps.entities) {
        Motion& powerUpMotion = registry.motions.get(e);
        if (collides(powerUpMotion, playerMotion)) {
            registry.collisions.emplace_with_duplicates(powerUpMotion.entity, playerMotion.entity);
            registry.collisions.emplace_with_duplicates(playerMotion.entity, powerUpMotion.entity);

        }
    }
}
