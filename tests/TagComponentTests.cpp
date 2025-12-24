//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

#include <gtest/gtest.h>
#include <Mira/ETS/World.hpp>
#include <Mira/ETS/SparseSet.hpp>

using namespace Mira::ETS;

// Define some tags
struct DeadTag {};

struct EnemyTag {};

struct BossTag {};

// Define a normal component
struct Position {
    float x, y;
};

TEST( TagComponentTest, AddRemoveTag ) {
    World world;
    auto entity = world.CreateEntity();

    EXPECT_FALSE( world.HasComponent < DeadTag >( entity ) );

    world.AddComponent( entity, DeadTag{} );
    EXPECT_TRUE( world.HasComponent < DeadTag >( entity ) );

    world.RemoveComponent < DeadTag >( entity );
    EXPECT_FALSE( world.HasComponent < DeadTag >( entity ) );
}

TEST( TagComponentTest, TagInView ) {
    World world;
    auto e1 = world.CreateEntity();
    auto e2 = world.CreateEntity();
    auto e3 = world.CreateEntity();

    world.AddComponent( e1, Position{ 10, 10 } );
    world.AddComponent( e1, DeadTag{} );

    world.AddComponent( e2, Position{ 20, 20 } );
    // e2 has no DeadTag

    world.AddComponent( e3, Position{ 30, 30 } );
    world.AddComponent( e3, DeadTag{} );

    int count = 0;
    world.GetView < Position, DeadTag >().Each( [&]( Position& pos, DeadTag& tag ) {
        count++;
        // Check if it's either e1 or e3
        EXPECT_TRUE( pos.x == 10.0f || pos.x == 30.0f );
    } );

    EXPECT_EQ( count, 2 );
}

TEST( TagComponentTest, MultipleTags ) {
    World world;
    auto e1 = world.CreateEntity();

    world.AddComponent( e1, EnemyTag{} );
    world.AddComponent( e1, BossTag{} );

    EXPECT_TRUE( world.HasComponent < EnemyTag >( e1 ) );
    EXPECT_TRUE( world.HasComponent < BossTag >( e1 ) );

    int count = 0;
    world.GetView < EnemyTag, BossTag >().Each( [&]( EnemyTag&, BossTag& ) {
        count++;
    } );

    EXPECT_EQ( count, 1 );
}

TEST( TagComponentTest, TagMemoryOptimization ) {
    // This test mostly ensures that the specialized SparseSet compiles and works.
    // We can check if it uses the specialization by looking at the Size of the store.

    SparseSet < DeadTag > tagStore;
    tagStore.Insert( 1, DeadTag{} );
    tagStore.Insert( 2, DeadTag{} );

    EXPECT_EQ( tagStore.Size(), 2 );
    EXPECT_TRUE( tagStore.Contains( 1 ) );
    EXPECT_TRUE( tagStore.Contains( 2 ) );

    // Verify Get returns a reference
    DeadTag& t1 = tagStore.Get( 1 );
    DeadTag& t2 = tagStore.Get( 2 );

    // For tags, they should refer to the same static instance
    EXPECT_EQ( &t1, &t2 );

    tagStore.Remove( 1 );
    EXPECT_EQ( tagStore.Size(), 1 );
    EXPECT_FALSE( tagStore.Contains( 1 ) );
}

TEST( TagComponentTest, TagEvents ) {
    World world;
    auto entity = world.CreateEntity();
    bool added = false;
    bool removed = false;

    world.OnEvent < DeadTag >( ComponentEvent::Added, [&]( EntityID id, DeadTag& ) {
        EXPECT_EQ( id, entity );
        added = true;
    } );

    world.OnEvent < DeadTag >( ComponentEvent::Removed, [&]( EntityID id, DeadTag& ) {
        EXPECT_EQ( id, entity );
        removed = true;
    } );

    world.AddComponent( entity, DeadTag{} );
    EXPECT_TRUE( added );

    world.RemoveComponent < DeadTag >( entity );
    EXPECT_TRUE( removed );
}