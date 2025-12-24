/**
 * Mira ETS Example 06: Observer System
 * 
 * This example demonstrates the reactive programming capabilities of Mira ETS.
 * Observers allow you to react to component lifecycle events:
 * - Added: Triggered when a component is attached to an entity.
 * - Removed: Triggered when a component is removed or the entity is destroyed.
 * - Modified: Triggered when a component is updated via PatchComponent.
 */

#include <Mira/ETS/World.hpp>
#include <iostream>
#include <string>

using namespace Mira::ETS;

// --- Components ---

struct Transform {
    float x, y;
};

struct PhysicsProxy {
    int proxyId;
    std::string debugName;
};

struct Health {
    int value;
};

// --- Mock Physics Engine ---
class PhysicsEngine {
public:
    int
    CreateProxy( float x, float y ) {
        static int nextId = 100;
        std::cout << "[Physics] Created proxy at (" << x << ", " << y << ") with ID " << nextId << std::endl;
        return nextId++;
    }

    void
    DestroyProxy( int id ) {
        std::cout << "[Physics] Destroyed proxy with ID " << id << std::endl;
    }

    void
    UpdateProxy( int id, float x, float y ) {
        std::cout << "[Physics] Updated proxy " << id << " to (" << x << ", " << y << ")" << std::endl;
    }
};

int
main() {
    World world;
    PhysicsEngine physics;

    std::cout << "--- Initializing Observers ---" << std::endl;

    // 1. Reactive Physics Proxy Management
    // Automatically create a physics proxy when a Transform is added to an entity
    world.OnEvent < Transform >( ComponentEvent::Added, [&]( EntityID id, Transform& transform ) {
        int proxyId = physics.CreateProxy( transform.x, transform.y );
        world.AddComponent < PhysicsProxy >( id, { proxyId, "Entity_" + std::to_string( id ) } );
        std::cout << "Observer: Added PhysicsProxy to Entity " << id << std::endl;
    } );

    // 2. Reactive Cleanup
    // Automatically destroy the physics proxy when the Transform is removed or the entity is destroyed
    world.OnEvent < PhysicsProxy >( ComponentEvent::Removed, [&]( EntityID id, PhysicsProxy& proxy ) {
        physics.DestroyProxy( proxy.proxyId );
        std::cout << "Observer: Cleaned up PhysicsProxy for Entity " << id << std::endl;
    } );

    // 3. Reactive Updates
    // Synchronize physics when the Transform is modified
    world.OnEvent < Transform >( ComponentEvent::Modified, [&]( EntityID id, Transform& transform ) {
        if ( world.HasComponent < PhysicsProxy >( id ) ) {
            auto& proxy = world.GetComponent < PhysicsProxy >( id );
            physics.UpdateProxy( proxy.proxyId, transform.x, transform.y );
            std::cout << "Observer: Synchronized Physics for Entity " << id << std::endl;
        }
    } );

    // 4. Game Logic Observer
    // React to health changes (e.g., for UI or death logic)
    world.OnEvent < Health >( ComponentEvent::Modified, [&]( EntityID id, Health& health ) {
        std::cout << "Observer: Entity " << id << " health changed to " << health.value << std::endl;
        if ( health.value <= 0 ) {
            std::cout << "Observer: Entity " << id << " has died!" << std::endl;
            // world.DestroyEntity(id); // Careful with recursion if destroying in callback
        }
    } );

    std::cout << "\n--- Step 1: Creating Entity with Transform ---" << std::endl;
    EntityID player = world.CreateEntity();
    // Adding Transform will trigger the Added event, which adds a PhysicsProxy
    world.AddComponent < Transform >( player, { 10.0f, 5.0f } );

    std::cout << "\n--- Step 2: Modifying Transform ---" << std::endl;
    // Modifying Transform via PatchComponent triggers the Modified event
    world.PatchComponent < Transform >( player, []( Transform& t ) {
        t.x = 20.0f;
        t.y = 15.0f;
    } );

    std::cout << "\n--- Step 3: Health Logic ---" << std::endl;
    world.AddComponent < Health >( player, { 100 } );
    world.PatchComponent < Health >( player, []( Health& h ) {
        h.value -= 50;
    } );
    world.PatchComponent < Health >( player, []( Health& h ) {
        h.value -= 60; // Death!
    } );

    std::cout << "\n--- Step 4: Destroying Entity ---" << std::endl;
    // Destroying the entity triggers Removed events for all components
    world.DestroyEntity( player );

    return 0;
}