#include <Mira/ETS/World.hpp>
#include <Mira/ETS/Prefab.hpp>
#include <Mira/ETS/Serialization.hpp>
#include <iostream>
#include <string>

using namespace Mira::ETS;

// --- Components ---

struct Position {
    float x, y;
};

struct Velocity {
    float vx, vy;
};

struct Health {
    int current;
    int max;
};

struct Name {
    std::string value;
};

// --- Main ---

int
main() {
    std::cout << "--- Mira ETS Prefab System Example ---" << std::endl;

    World world;
    SerializationContext context;

    // 1. Register components with the serialization context
    // This tells the system how to handle these components in JSON

    context.Register < Position >( "Position",
                                   []( const Position& p, std::ostream& os ) {
                                       os << "{\"x\":" << p.x << ",\"y\":" << p.y << "}";
                                   },
                                   []( const simdjson::dom::element& el ) {
                                       return Position{
                                           ( float ) el[ "x" ].get_double(), ( float ) el[ "y" ].get_double()
                                       };
                                   }
    );

    context.Register < Velocity >( "Velocity",
                                   []( const Velocity& v, std::ostream& os ) {
                                       os << "{\"vx\":" << v.vx << ",\"vy\":" << v.vy << "}";
                                   },
                                   []( const simdjson::dom::element& el ) {
                                       return Velocity{
                                           ( float ) el[ "vx" ].get_double(), ( float ) el[ "vy" ].get_double()
                                       };
                                   }
    );

    context.Register < Health >( "Health",
                                 []( const Health& h, std::ostream& os ) {
                                     os << "{\"current\":" << h.current << ",\"max\":" << h.max << "}";
                                 },
                                 []( const simdjson::dom::element& el ) {
                                     return Health{
                                         ( int ) el[ "current" ].get_int64(), ( int ) el[ "max" ].get_int64()
                                     };
                                 }
    );

    context.Register < Name >( "Name",
                               []( const Name& n, std::ostream& os ) {
                                   os << "\"" << n.value << "\"";
                               },
                               []( const simdjson::dom::element& el ) {
                                   return Name{ std::string( el.get_string().value() ) };
                               }
    );

    // 2. Initialize PrefabManager
    PrefabManager prefabManager( context );

    // 3. Define prefabs in JSON
    // In a real app, this would be loaded from a file
    std::string prefabJson = R"(
    {
        "Grunt": {
            "Name": "Enemy Grunt",
            "Position": {"x": 0, "y": 0},
            "Velocity": {"vx": 1, "vy": 0},
            "Health": {"current": 50, "max": 50}
        },
        "Commander": {
            "Name": "Enemy Commander",
            "Position": {"x": 0, "y": 0},
            "Velocity": {"vx": 0.5, "vy": 0.5},
            "Health": {"current": 200, "max": 200}
        },
        "StaticTree": {
            "Name": "Oak Tree",
            "Position": {"x": 100, "y": 100}
        }
    }
    )";

    std::cout << "Loading prefabs..." << std::endl;
    Result res = prefabManager.LoadPrefabs( prefabJson );
    if ( !res ) {
        std::cerr << "Failed to load prefabs: " << res.message << std::endl;
        return 1;
    }

    // 4. Instantiate entities from prefabs
    std::cout << "Instantiating entities..." << std::endl;

    EntityID e1 = prefabManager.Instantiate( "Grunt", world );
    EntityID e2 = prefabManager.Instantiate( "Commander", world );
    EntityID e3 = prefabManager.Instantiate( "Grunt", world ); // Another grunt
    EntityID e4 = prefabManager.Instantiate( "StaticTree", world );

    // 5. Customize an instance after instantiation
    world.PatchComponent < Position >( e3, []( Position& p ) {
        p.x = 20.0f; // Move the second grunt
    } );

    // 6. Display the world state
    std::cout << "\nWorld State after instantiation:" << std::endl;

    // We can iterate over entities that have both Name and Position
    auto view = world.GetView < Name, Position >();
    view.Each( [&]( Name& n, Position& p ) {
        std::cout << "Entity: " << n.value << " at (" << p.x << ", " << p.y << ")";
        std::cout << std::endl;
    } );

    // Demonstrate Health if present using SystemUpdate
    std::cout << "\nHealth of entities (only for those that have it):" << std::endl;
    world.SystemUpdate( []( Name& n, Health& h ) {
        std::cout << n.value << ": " << h.current << "/" << h.max << std::endl;
    } );

    return 0;
}