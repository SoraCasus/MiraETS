#include <Mira/ETS/World.hpp>
#include <iostream>
#include <string>

/**
 * @file main.cpp
 * @brief Basic ECS usage example for Mira ETS.
 * 
 * This sample demonstrates the core functionality of the Entity Component System:
 * 1. Creating a World.
 * 2. Defining components as simple structs.
 * 3. Creating entities and attaching components.
 * 4. Querying entities using Views.
 * 5. Using system_update for automated queries.
 */

using namespace Mira::ETS;

// Components are plain old data (POD) structs.
// No special base class is required.

struct Position {
    float X, Y;
};

struct Velocity {
    float Dx, Dy;
};

struct Name {
    std::string Value;
};

int
main() {
    // The World is the central container for all entities and components.
    World world;

    std::cout << "--- Mira ETS: ECS Basics Example ---" << std::endl;

    // 1. Create entities
    // Entities are just 64-bit identifiers (EntityID).
    auto player = world.CreateEntity();
    auto enemy = world.CreateEntity();
    auto npc = world.CreateEntity();

    // You can also create many entities at once for better performance
    auto bullets = world.CreateEntitiesBulk( 10 );

    // 2. Add components to entities
    world.AddComponent( player, Position{ 0.0f, 0.0f } );
    world.AddComponent( player, Velocity{ 1.0f, 1.0f } );
    world.AddComponent( player, Name{ "Player" } );

    world.AddComponent( enemy, Position{ 10.0f, 10.0f } );
    world.AddComponent( enemy, Velocity{ -0.5f, 0.0f } );
    world.AddComponent( enemy, Name{ "Enemy" } );

    world.AddComponent( npc, Position{ 5.0f, 5.0f } );
    world.AddComponent( npc, Name{ "Villager" } ); // NPC has no velocity

    // 3. Query entities using a View
    // Views allow you to iterate over entities that have a specific set of components.
    // Mira ETS optimizes this to iterate over the smallest set of components involved.
    std::cout << "\nInitial positions:" << std::endl;
    world.GetView < Position, Name >().Each( []( Position& pos, Name& name ) {
        std::cout << "Entity '" << name.Value << "' is at (" << pos.X << ", " << pos.Y << ")" << std::endl;
    } );

    // 4. Update logic using SystemUpdate
    // SystemUpdate automatically deduces the required components from the lambda signature.
    std::cout << "\nUpdating positions based on velocity..." << std::endl;
    world.SystemUpdate( []( Position& pos, Velocity& vel ) {
        pos.X += vel.Dx;
        pos.Y += vel.Dy;
    } );

    // 5. Verify updates
    std::cout << "\nPositions after update:" << std::endl;
    world.GetView < Position, Name >().Each( []( Position& pos, Name& name ) {
        std::cout << "Entity '" << name.Value << "' is now at (" << pos.X << ", " << pos.Y << ")" << std::endl;
    } );

    // 6. Removing components and destroying entities
    // Mira ETS optimizes DestroyEntity to only touch the component stores 
    // that the entity actually uses, making it very efficient even with 
    // many registered component types.
    std::cout << "\nDestroying Enemy and removing NPC Name..." << std::endl;
    world.DestroyEntity( enemy );
    world.RemoveComponent < Name >( npc );

    std::cout << "\nFinal world state (entities with Position):" << std::endl;
    // We can iterate just with Position
    int count = 0;
    world.GetView < Position >().Each( [&count]( Position& ) {
        count++;
    } );
    std::cout << "Number of entities with Position: " << count << std::endl;

    return 0;
}