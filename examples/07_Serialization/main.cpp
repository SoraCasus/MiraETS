#include <Mira/ETS/World.hpp>
#include <Mira/ETS/Serialization.hpp>
#include <iostream>
#include <string>
#include <sstream>

using namespace Mira::ETS;

// Define some components
struct Position {
    float x, y;
};

struct Player {
    std::string name;
    int score;
};

int
main() {
    World world;

    // Create some entities
    EntityID player1 = world.CreateEntity();
    world.AddComponent < Position >( player1, { 10.0f, 20.0f } );
    world.AddComponent < Player >( player1, { "Alice", 100 } );

    EntityID player2 = world.CreateEntity();
    world.AddComponent < Position >( player2, { 50.0f, 60.0f } );
    world.AddComponent < Player >( player2, { "Bob", 200 } );

    // Setup serialization context
    SerializationContext ctx;

    // Register Position component
    ctx.Register < Position >( "Position",
                               []( const Position& p, std::ostream& os ) {
                                   os << "{\"x\":" << p.x << ",\"y\":" << p.y << "}";
                               },
                               []( const simdjson::dom::element& el ) {
                                   return Position{
                                       ( float ) el[ "x" ].get_double(),
                                       ( float ) el[ "y" ].get_double()
                                   };
                               }
    );

    // Register Player component
    ctx.Register < Player >( "Player",
                             []( const Player& p, std::ostream& os ) {
                                 os << "{\"name\":\"" << p.name << "\",\"score\":" << p.score << "}";
                             },
                             []( const simdjson::dom::element& el ) {
                                 std::string_view name;
                                 auto error = el[ "name" ].get( name );
                                 if ( error ) name = "Unknown";
                                 return Player{
                                     std::string( name ),
                                     ( int ) el[ "score" ].get_int64()
                                 };
                             }
    );

    // Register Binary serializers for even better performance
    ctx.RegisterBinary < Position >( "Position",
                                     []( const Position& p, std::ostream& os ) {
                                         os.write( reinterpret_cast < const char* >( &p ), sizeof( p ) );
                                     },
                                     []( std::istream& is ) {
                                         Position p;
                                         is.read( reinterpret_cast < char* >( &p ), sizeof( p ) );
                                         return p;
                                     }
    );

    ctx.RegisterBinary < Player >( "Player",
                                   []( const Player& p, std::ostream& os ) {
                                       uint32_t len = static_cast < uint32_t >( p.name.size() );
                                       os.write( reinterpret_cast < const char* >( &len ), sizeof( len ) );
                                       os.write( p.name.data(), len );
                                       os.write( reinterpret_cast < const char* >( &p.score ), sizeof( p.score ) );
                                   },
                                   []( std::istream& is ) {
                                       uint32_t len;
                                       is.read( reinterpret_cast < char* >( &len ), sizeof( len ) );
                                       std::string name( len, '\0' );
                                       is.read( &name[ 0 ], len );
                                       int score;
                                       is.read( reinterpret_cast < char* >( &score ), sizeof( score ) );
                                       return Player{ name, score };
                                   }
    );

    // Serialize world to a string
    std::stringstream ss;
    ctx.Serialize( world, ss );
    std::string savedData = ss.str();

    std::cout << "Serialized World State (JSON):\n" << savedData << "\n\n";

    // Create a new world and deserialize
    World newWorld;
    Result res = ctx.Deserialize( newWorld, savedData );
    if ( !res ) {
        std::cerr << "Deserialization failed: " << res.message << std::endl;
        return 1;
    }

    std::cout << "Entities in new world (from JSON):\n";
    newWorld.SystemUpdate( []( const Player& p, const Position& pos ) {
        std::cout << "Player: " << p.name << ", Score: " << p.score
                << " at (" << pos.x << ", " << pos.y << ")\n";
    } );

    // Demonstrate Binary Serialization
    std::stringstream binaryStream;
    ctx.SerializeBinary( world, binaryStream );
    std::string binaryData = binaryStream.str();
    std::cout << "\nBinary Serialization complete. Size: " << binaryData.size() << " bytes\n";

    World binaryWorld;
    binaryStream.seekg( 0 );
    ctx.DeserializeBinary( binaryWorld, binaryStream );

    std::cout << "Entities in new world (from Binary):\n";
    binaryWorld.SystemUpdate( []( const Player& p, const Position& pos ) {
        std::cout << "Player: " << p.name << ", Score: " << p.score
                << " at (" << pos.x << ", " << pos.y << ")\n";
    } );

    // Verify IDs are preserved
    if ( newWorld.IsAlive( player1 ) && newWorld.IsAlive( player2 ) ) {
        std::cout << "\nSuccess: Entity IDs were preserved during serialization!\n";
    }

    return 0;
}