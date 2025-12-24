//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

#include <Mira/ETS/World.hpp>
#include <iostream>

/**
 * @file main.cpp
 * @brief Tag Components optimization example for Mira ETS.
 * 
 * This sample demonstrates Tag Components:
 * 1. Defining Tag components (empty structs).
 * 2. How Mira ETS optimizes memory for Tags by not allocating data arrays.
 * 3. Using Tags in Views for efficient filtering.
 * 4. Combining Tags with data-carrying components.
 */

using namespace Mira::ETS;

// 1. Define Tag components
// In Mira ETS, any empty struct is automatically treated as a Tag component.
// These components take up no space in the dense data arrays of the ECS.
struct PersistentTag {}; // Mark entity to not be destroyed during scene clear
struct EnemyTag {}; // Mark entity as an enemy
struct BossTag {}; // Mark entity as a boss
struct StunnedTag {}; // Mark entity as currently stunned

// Data components
struct Health {
    int Current;
    int Max;
};

struct Position {
    float X, Y;
};

int
main() {
    World world;

    std::cout << "--- Mira ETS: Tag Components Example ---" << std::endl;

    // 2. Create entities with various combinations of tags and data
    auto player = world.CreateEntity();
    world.AddComponent( player, Position{ 0.0f, 0.0f } );
    world.AddComponent( player, Health{ 100, 100 } );
    world.AddComponent( player, PersistentTag{} ); // Player is persistent

    auto minion = world.CreateEntity();
    world.AddComponent( minion, Position{ 5.0f, 5.0f } );
    world.AddComponent( minion, Health{ 20, 20 } );
    world.AddComponent( minion, EnemyTag{} );

    auto boss = world.CreateEntity();
    world.AddComponent( boss, Position{ 20.0f, 20.0f } );
    world.AddComponent( boss, Health{ 500, 500 } );
    world.AddComponent( boss, EnemyTag{} );
    world.AddComponent( boss, BossTag{} );

    // 3. Using Tags in Views
    std::cout << "\nFinding all enemies:" << std::endl;
    world.GetView < EnemyTag >().Each( [&]( EnemyTag& ) {
        std::cout << "Found an enemy!" << std::endl;
    } );

    std::cout << "\nFinding all bosses (must have both EnemyTag and BossTag):" << std::endl;
    world.GetView < EnemyTag, BossTag >().Each( [&]( EnemyTag&, BossTag& ) {
        std::cout << "Found a boss enemy!" << std::endl;
    } );

    // 4. Combining Tags and Data
    std::cout << "\nApplying damage to all enemies:" << std::endl;
    world.SystemUpdate( []( Health& hp, EnemyTag& ) {
        hp.Current -= 10;
        std::cout << "Enemy health reduced to " << hp.Current << std::endl;
    } );

    // 5. Reactive logic with Tags
    std::cout << "\nStunning the boss..." << std::endl;
    world.AddComponent( boss, StunnedTag{} );

    std::cout << "Checking for stunned entities:" << std::endl;
    world.GetView < StunnedTag >().Each( []( StunnedTag& ) {
        std::cout << "An entity is stunned!" << std::endl;
    } );

    // 6. Memory Optimization Note
    // Behind the scenes, Mira::ETS::SparseSet<EnemyTag> does not have a 
    // std::vector<EnemyTag> m_DenseData member. It only maintains the 
    // sparse and dense entity arrays to allow O(1) checks and O(N_tag) iteration.

    std::cout << "\nMemory Optimization:" << std::endl;
    std::cout << "PersistentTag is empty: " << std::boolalpha << std::is_empty_v < PersistentTag > << std::endl;
    std::cout << "Mira ETS automatically uses the optimized SparseSet for PersistentTag." << std::endl;

    return 0;
}