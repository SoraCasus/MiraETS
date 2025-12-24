#include <Mira/ETS/Movable.hpp>
#include <iostream>
#include <vector>
#include <string>

/**
 * @file main.cpp
 * @brief Type Erasure example for Mira ETS.
 * 
 * This sample demonstrates the AnyMovable container which allows storing 
 * heterogeneous objects that satisfy a specific interface (UpdatePosition)
 * without requiring a common base class. It uses Small Buffer Optimization (SBO)
 * for performance.
 */

using namespace Mira::ETS;

// To be compatible with AnyMovable, a type must implement:
// void UpdatePosition(float dt)

struct Hero {
    std::string Name;
    float X = 0;

    void
    UpdatePosition( float dt ) {
        X += 10.0f * dt;
        std::cout << "Hero " << Name << " moved to X=" << X << std::endl;
    }
};

struct Monster {
    int ID;
    float X = 100;

    void
    UpdatePosition( float dt ) {
        X -= 5.0f * dt;
        std::cout << "Monster #" << ID << " moved to X=" << X << std::endl;
    }
};

// A large object that will exceed the 64-byte SBO buffer
struct LargeObject {
    double Data[ 10 ]; // 80 bytes
    float X = 0;

    void
    UpdatePosition( float dt ) {
        X += 1.0f * dt;
        std::cout << "LargeObject moved to X=" << X << " (Heap Allocated)" << std::endl;
    }
};

int
main() {
    std::cout << "--- Mira ETS: Type Erasure Example ---" << std::endl;

    // AnyMovable can hold any type that has an UpdatePosition(float) method.
    std::vector < AnyMovable > objects;

    // Small objects (Hero, Monster) will use the internal 64-byte buffer (SBO).
    objects.emplace_back( Hero{ "Arthur", 0.0f } );
    objects.emplace_back( Monster{ 42, 100.0f } );

    // Large objects will automatically fall back to heap allocation.
    objects.emplace_back( LargeObject{ {}, 50.0f } );

    std::cout << "\nUpdating all objects in a single loop:" << std::endl;
    for ( auto& obj : objects ) {
        obj.Update( 0.5f );
    }

    std::cout << "\nAnyMovable supports copy and move semantics:" << std::endl;
    AnyMovable copy = objects[ 0 ];
    copy.Update( 1.0f );

    return 0;
}