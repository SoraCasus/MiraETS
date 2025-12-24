#include <gtest/gtest.h>
#include <Mira/ETS/World.hpp>

using namespace Mira::ETS;

struct Position {
    float x, y;
};

struct Velocity {
    float vx, vy;
};

TEST( ObserverTests, OnComponentAdded ) {
    World world;
    EntityID entity = world.CreateEntity();

    bool called = false;
    world.OnEvent < Position >( ComponentEvent::Added, [&]( EntityID id, Position& pos ) {
        EXPECT_EQ( id, entity );
        EXPECT_EQ( pos.x, 10.0f );
        called = true;
    } );

    world.AddComponent < Position >( entity, { 10.0f, 20.0f } );
    EXPECT_TRUE( called );
}

TEST( ObserverTests, OnComponentRemoved ) {
    World world;
    EntityID entity = world.CreateEntity();
    world.AddComponent < Position >( entity, { 10.0f, 20.0f } );

    bool called = false;
    world.OnEvent < Position >( ComponentEvent::Removed, [&]( EntityID id, Position& pos ) {
        EXPECT_EQ( id, entity );
        EXPECT_EQ( pos.x, 10.0f );
        called = true;
    } );

    world.RemoveComponent < Position >( entity );
    EXPECT_TRUE( called );
}

TEST( ObserverTests, OnComponentModified ) {
    World world;
    EntityID entity = world.CreateEntity();
    world.AddComponent < Position >( entity, { 10.0f, 20.0f } );

    bool called = false;
    world.OnEvent < Position >( ComponentEvent::Modified, [&]( EntityID id, Position& pos ) {
        EXPECT_EQ( id, entity );
        EXPECT_EQ( pos.x, 30.0f );
        called = true;
    } );

    world.PatchComponent < Position >( entity, []( Position& pos ) {
        pos.x = 30.0f;
    } );
    EXPECT_TRUE( called );
}

TEST( ObserverTests, DestroyEntityTriggersRemoved ) {
    World world;
    EntityID entity = world.CreateEntity();
    world.AddComponent < Position >( entity, { 10.0f, 20.0f } );
    world.AddComponent < Velocity >( entity, { 1.0f, 1.0f } );

    bool posRemoved = false;
    bool velRemoved = false;

    world.OnEvent < Position >( ComponentEvent::Removed, [&]( EntityID, Position& ) {
        posRemoved = true;
    } );
    world.OnEvent < Velocity >( ComponentEvent::Removed, [&]( EntityID, Velocity& ) {
        velRemoved = true;
    } );

    world.DestroyEntity( entity );

    EXPECT_TRUE( posRemoved );
    EXPECT_TRUE( velRemoved );
}

TEST( ObserverTests, MultipleObservers ) {
    World world;
    EntityID entity = world.CreateEntity();

    int callCount = 0;
    world.OnEvent < Position >( ComponentEvent::Added, [&]( EntityID, Position& ) {
        callCount++;
    } );
    world.OnEvent < Position >( ComponentEvent::Added, [&]( EntityID, Position& ) {
        callCount++;
    } );

    world.AddComponent < Position >( entity, { 0, 0 } );
    EXPECT_EQ( callCount, 2 );
}

TEST( ObserverTests, PatchNonExistent ) {
    World world;
    EntityID entity = world.CreateEntity();

    bool called = false;
    world.OnEvent < Position >( ComponentEvent::Modified, [&]( EntityID, Position& ) {
        called = true;
    } );

    world.PatchComponent < Position >( entity, []( Position& pos ) {
        pos.x = 10;
    } );
    EXPECT_FALSE( called );
}

TEST( ObserverTests, RemoveNonExistent ) {
    World world;
    EntityID entity = world.CreateEntity();

    bool called = false;
    world.OnEvent < Position >( ComponentEvent::Removed, [&]( EntityID, Position& ) {
        called = true;
    } );

    world.RemoveComponent < Position >( entity );
    EXPECT_FALSE( called );
}

TEST( ObserverTests, MultipleComponentsMultipleEvents ) {
    World world;
    EntityID entity = world.CreateEntity();

    int added = 0;
    int modified = 0;
    int removed = 0;

    world.OnEvent < Position >( ComponentEvent::Added, [&]( EntityID, Position& ) {
        added++;
    } );
    world.OnEvent < Position >( ComponentEvent::Modified, [&]( EntityID, Position& ) {
        modified++;
    } );
    world.OnEvent < Position >( ComponentEvent::Removed, [&]( EntityID, Position& ) {
        removed++;
    } );

    world.AddComponent < Position >( entity, { 0, 0 } );
    world.PatchComponent < Position >( entity, []( Position& p ) {
        p.x = 1;
    } );
    world.RemoveComponent < Position >( entity );

    EXPECT_EQ( added, 1 );
    EXPECT_EQ( modified, 1 );
    EXPECT_EQ( removed, 1 );
}