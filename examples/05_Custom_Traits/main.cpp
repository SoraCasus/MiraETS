#include <Mira/ETS/Concepts.hpp>
#include <Mira/ETS/World.hpp>
#include <iostream>
#include <string>
#include <concepts>

/**
 * @file main.cpp
 * @brief Custom Traits example for Mira ETS.
 * 
 * This sample demonstrates how to define new concepts and traits to extend 
 * the library's static polymorphism capabilities, and how to use them 
 * within the ECS (Entity Component System).
 */

using namespace Mira::ETS;

namespace MyGame {
    // 1. Define a concept that our new Trait will require.
    // Concepts allow us to specify what members a type must have.
    template< typename T >
    concept HasHealth = requires( T t ) {
        { t.Health } -> std::convertible_to < float >;
    };

    // 2. Define the Trait.
    // A Trait is usually a struct with template methods that take a 'Self' parameter.
    struct DamageableTrait {
        template< typename Self >
            requires HasHealth < std::remove_cvref_t < Self > >
        void
        TakeDamage( Self& self, float amount ) {
            self.Health -= amount;
            std::cout << "  - Log: Entity took " << amount << " damage. Remaining Health: " << self.Health << std::endl;
        }
    };

    // 3. Use the Trait in an entity type (Static Polymorphism).
    struct Soldier {
        std::string Name;
        float Health = 100.0f;

        // We compose the behavior into our struct.
        // MIRA_TRAIT ensures this takes 0 bytes if the trait is empty.
        MIRA_TRAIT( DamageableTrait, DamageLogic );

        void
        OnHit( float damage ) {
            // Forward the call to the trait, passing *this.
            DamageLogic.TakeDamage( *this, damage );
        }
    };

    // 4. Use the Trait in an ECS Component.
    struct HealthComponent {
        float Health;

        MIRA_TRAIT( DamageableTrait, DamageLogic );
    };

    struct NameComponent {
        std::string Value;
    };
}

int
main() {
    std::cout << "--- Mira ETS: Custom Traits Example ---" << std::endl;

    // Part A: Static Polymorphism usage
    std::cout << "\n[Part A] Static Polymorphism usage:" << std::endl;
    MyGame::Soldier soldier{ "Standard Infantry", 100.0f };
    soldier.OnHit( 30.0f );

    // Part B: Integration with ECS
    std::cout << "\n[Part B] Integration with ECS:" << std::endl;
    World world;

    auto e1 = world.CreateEntity();
    world.AddComponent( e1, MyGame::HealthComponent{ 500.0f } );
    world.AddComponent( e1, MyGame::NameComponent{ "Stone Wall" } );

    auto e2 = world.CreateEntity();
    world.AddComponent( e2, MyGame::HealthComponent{ 50.0f } );
    world.AddComponent( e2, MyGame::NameComponent{ "Wooden Fence" } );

    std::cout << "Simulating a 'Meteor' system damaging all entities with Health:" << std::endl;
    float meteorDamage = 40.0f;

    // We can use the trait within a system loop
    world.SystemUpdate( [meteorDamage]( MyGame::HealthComponent& hc, MyGame::NameComponent& nc ) {
        std::cout << "Meteor hits " << nc.Value << "!" << std::endl;
        hc.DamageLogic.TakeDamage( hc, meteorDamage );
    } );

    return 0;
}