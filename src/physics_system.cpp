// internal
#include "physics_system.hpp"
#include "tiny_ecs_registry.hpp"
#include "world_init.hpp"
#include <iostream>

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
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

    return motion1_right >  motion2_left && motion2_up < motion1_down && motion1_left < motion2_right && motion1_up < motion2_down;
}

void PhysicsSystem::step(float elapsed_ms)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	float step_seconds = elapsed_ms / 1000.f;

	for(uint i = 0; i< motion_registry.size(); i++)
	{
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];

		motion.last_physic_move = vec2(0,0);

		if (registry.dashes.has(entity)) {
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


	// Check for collisions between all moving entities
    ComponentContainer<Motion> &motion_container = registry.motions;
	for(uint i = 0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];

		
		// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
		for(uint j = i+1; j<motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			if (collides(motion_i, motion_j))
			{

				Entity entity_j = motion_container.entities[j];
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}
}
