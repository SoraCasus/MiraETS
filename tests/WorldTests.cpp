//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//
#include <gtest/gtest.h>
#include "Mira/ETS/World.hpp"

using namespace Mira::ETS;

TEST( WorldTests, SystemQuery_FiltersEntities ) {
    World world;

    // Entity 0: Has Pos + Vel (Should match)
    auto e1 = world.CreateEntity();
    world.AddComponent < float >( e1, 10.0f );
    world.AddComponent < int >( e1, 1 );

    // Entity 1: Has Pos only (Should NOT match)
    auto e2 = world.CreateEntity();
    world.AddComponent < float >( e2, 50.0f );

    // System Update (Should only affect e1)
    bool e2_touched = false;
    world.SystemUpdate( [&]( float& pos, int& vel ) {
        pos += ( float ) vel;
        if ( pos > 40.0f ) e2_touched = true; // Heuristic check
    } );

    // Verification
    EXPECT_FLOAT_EQ( world.GetComponent < float >( e1 ), 11.0f ); // 10 + 1
    EXPECT_FLOAT_EQ( world.GetComponent < float >( e2 ), 50.0f ); // Untouched
    EXPECT_FALSE( e2_touched );
}

TEST( WorldTests, EntityRecycling ) {
    World world;
    auto e1 = world.CreateEntity();
    uint32_t i1 = Internal::GetIndex( e1 );
    uint32_t g1 = Internal::GetGeneration( e1 );

    world.DestroyEntity( e1 );

    auto e2 = world.CreateEntity();
    uint32_t i2 = Internal::GetIndex( e2 );
    uint32_t g2 = Internal::GetGeneration( e2 );

    EXPECT_EQ( i1, i2 );
    EXPECT_EQ( g1 + 1, g2 );
}

TEST( WorldTests, ViewOptimization ) {
    World world;
    for ( int i = 0; i < 100; ++i ) {
        auto e = world.CreateEntity();
        if ( i % 10 == 0 ) {
            world.AddComponent < int >( e, i );
        }
        world.AddComponent < float >( e, ( float ) i );
    }

    // Querying (int, float) should iterate only 10 times because int is the smallest set
    int count = 0;
    world.GetView < int, float >().Each( [&]( int& i, float& f ) {
        count++;
    } );

    EXPECT_EQ( count, 10 );
}

TEST( WorldTests, ComponentIDScaling ) {
    World world;
    auto e = world.CreateEntity();
    world.AddComponent < int >( e, 1 );
    EXPECT_TRUE( world.HasComponent < int >( e ) );
}

TEST( WorldTests, CoverageBoost_Extra ) {
    World world;
    auto e1 = world.CreateEntity();

    // 1. HasComponent
    EXPECT_FALSE( world.HasComponent < int >( e1 ) );
    world.AddComponent < int >( e1, 42 );
    EXPECT_TRUE( world.HasComponent < int >( e1 ) );

    // 2. RemoveComponent
    world.RemoveComponent < int >( e1 );
    EXPECT_FALSE( world.HasComponent < int >( e1 ) );

    // 3. RemoveComponent non-existent
    world.RemoveComponent < int >( e1 );

    // 4. HasComponent out of bounds
    EXPECT_FALSE( world.HasComponent < float >( 999 ) );

    // 5. SystemUpdate explicit types
    world.AddComponent < float >( e1, 10.0f );
    int call_count = 0;
    world.SystemUpdate < float >( [&]( float& f ) {
        call_count++;
        f += 1.0f;
    } );
    EXPECT_EQ( call_count, 1 );
    EXPECT_FLOAT_EQ( world.GetComponent < float >( e1 ), 11.0f );

    // 6. Resize signatures
    world.AddComponent < int >( 10, 100 );
    EXPECT_EQ( world.GetComponent < int >( 10 ), 100 );

    // 7. RemoveComponent out of bounds
    world.RemoveComponent < int >( 999 );

    // 8. CreateEntity with ID
    EntityID customID = ( static_cast < EntityID >( 5 ) << 32 ) | 100;
    EntityID createdID = world.CreateEntity( customID );
    EXPECT_EQ( createdID, customID );
    EXPECT_TRUE( world.IsAlive( customID ) );
    EXPECT_EQ( Internal::GetGeneration( createdID ), 5 );
    EXPECT_EQ( Internal::GetIndex( createdID ), 100 );

    // 9. CreateEntity with existing alive ID
    EntityID sameID = world.CreateEntity( customID );
    EXPECT_EQ( sameID, customID );

    // 10. CreateEntity with same index but different generation
    EntityID newGenID = ( static_cast < EntityID >( 10 ) << 32 ) | 100;
    EntityID createdNewGen = world.CreateEntity( newGenID );
    EXPECT_EQ( createdNewGen, newGenID );
    EXPECT_TRUE( world.IsAlive( newGenID ) );
    EXPECT_FALSE( world.IsAlive( customID ) );
}

TEST( WorldTests, CreateEntitiesBulk ) {
    World world;

    // 1. Bulk create 10 entities
    auto entities = world.CreateEntitiesBulk( 10 );
    EXPECT_EQ( entities.size(), 10 );
    for ( auto e : entities ) {
        EXPECT_TRUE( world.IsAlive( e ) );
    }

    // 2. Destroy some entities to test recycling in bulk
    world.DestroyEntity( entities[ 1 ] );
    world.DestroyEntity( entities[ 3 ] );
    world.DestroyEntity( entities[ 5 ] );

    // 3. Bulk create 5 more entities (should recycle 3 and create 2 new)
    auto moreEntities = world.CreateEntitiesBulk( 5 );
    EXPECT_EQ( moreEntities.size(), 5 );

    // Verify recycling
    EXPECT_EQ( Internal::GetIndex( moreEntities[ 0 ] ), Internal::GetIndex( entities[ 5 ] ) );
    EXPECT_EQ( Internal::GetIndex( moreEntities[ 1 ] ), Internal::GetIndex( entities[ 3 ] ) );
    EXPECT_EQ( Internal::GetIndex( moreEntities[ 2 ] ), Internal::GetIndex( entities[ 1 ] ) );

    for ( auto e : moreEntities ) {
        EXPECT_TRUE( world.IsAlive( e ) );
    }

    // Verify new entities
    EXPECT_EQ( Internal::GetIndex( moreEntities[ 3 ] ), 10 );
    EXPECT_EQ( Internal::GetIndex( moreEntities[ 4 ] ), 11 );
}