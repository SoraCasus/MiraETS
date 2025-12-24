#include <Mira/ETS/Traits.hpp>
#include <iostream>
#include <vector>

/**
 * @file main.cpp
 * @brief Static Polymorphism example for Mira ETS.
 * 
 * This sample demonstrates how to use Mixins and Traits to compose entity behavior
 * at compile-time without the overhead of virtual functions.
 */

using namespace Mira::ETS;

// Define a custom entity that uses the MovableTrait mixin.
// MIRA_TRAIT uses [[no_unique_address]] to ensure the trait takes no space if it's empty.
struct Robot {
    float X = 0, Y = 0;
    float Vx = 1, Vy = 1;

    // The MovableTrait provides a standardized Update logic for types that 
    // have X, Y, Vx, and Vy members.
    MIRA_TRAIT( MovableTrait, Movement );

    void
    Update( float dt ) {
        // We pass *this to the trait's Update method.
        Movement.Update( *this, dt );
    }
};

// Another entity type with different data but compatible with MovableTrait.
struct Cloud {
    float X = 100, Y = 100;
    float Vx = -0.1f, Vy = 0.05f;

    MIRA_TRAIT( MovableTrait, Movement );

    void
    Update( float dt ) {
        Movement.Update( *this, dt );
    }
};

int
main() {
    std::cout << "--- Mira ETS: Static Polymorphism Example ---" << std::endl;

    Robot myRobot;
    Cloud myCloud;

    std::cout << "Initial Robot position: (" << myRobot.X << ", " << myRobot.Y << ")" << std::endl;
    std::cout << "Initial Cloud position: (" << myCloud.X << ", " << myCloud.Y << ")" << std::endl;

    // Update for 2 seconds
    float dt = 2.0f;
    std::cout << "\nUpdating entities for " << dt << " seconds..." << std::endl;

    myRobot.Update( dt );
    myCloud.Update( dt );

    std::cout << "New Robot position: (" << myRobot.X << ", " << myRobot.Y << ")" << std::endl;
    std::cout << "New Cloud position: (" << myCloud.X << ", " << myCloud.Y << ")" << std::endl;

    // Static polymorphism allows us to write generic functions that work with 
    // any type satisfying a concept, without virtual dispatch.
    auto PrintPos = []( auto& entity ) {
        std::cout << "Entity position: (" << entity.X << ", " << entity.Y << ")" << std::endl;
    };

    std::cout << "\nPrinting positions generically:" << std::endl;
    PrintPos( myRobot );
    PrintPos( myCloud );

    return 0;
}