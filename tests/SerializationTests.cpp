#include <gtest/gtest.h>
#include <Mira/ETS/World.hpp>
#include <Mira/ETS/Serialization.hpp>
#include <sstream>

using namespace Mira::ETS;

struct Vec2 {
    float x, y;
};

struct Name {
    std::string value;
};

TEST( SerializationTest, BasicSerialization ) {
    World world;
    EntityID e1 = world.CreateEntity();
    world.AddComponent < Vec2 >( e1, { 1.0f, 2.0f } );
    world.AddComponent < Name >( e1, { "Entity1" } );

    SerializationContext ctx;
    ctx.Register < Vec2 >( "Vec2",
                           []( const Vec2& v, std::ostream& os ) {
                               os << "{\"x\":" << v.x << ",\"y\":" << v.y << "}";
                           },
                           []( const simdjson::dom::element& el ) {
                               return Vec2{ ( float ) el[ "x" ].get_double(), ( float ) el[ "y" ].get_double() };
                           }
    );
    ctx.Register < Name >( "Name",
                           []( const Name& n, std::ostream& os ) {
                               os << "\"" << n.value << "\"";
                           },
                           []( const simdjson::dom::element& el ) {
                               std::string_view sv;
                               auto unused = el.get( sv );
                               ( void ) unused;
                               return Name{ std::string( sv ) };
                           }
    );

    std::stringstream ss;
    ctx.Serialize( world, ss );
    std::string json = ss.str();

    // Check if JSON contains expected parts
    EXPECT_NE( json.find( "\"Vec2\"" ), std::string::npos );
    EXPECT_NE( json.find( "\"Name\"" ), std::string::npos );
    EXPECT_NE( json.find( "\"Entity1\"" ), std::string::npos );

    World newWorld;
    ctx.Deserialize( newWorld, json );

    // Verify new world state and IDs
    EXPECT_TRUE( newWorld.IsAlive( e1 ) );
    bool found = false;
    newWorld.SystemUpdate( [&]( const Vec2& v, const Name& n ) {
        EXPECT_FLOAT_EQ( v.x, 1.0f );
        EXPECT_FLOAT_EQ( v.y, 2.0f );
        EXPECT_EQ( n.value, "Entity1" );
        found = true;
    } );
    EXPECT_TRUE( found );
}

TEST( SerializationTest, EmptyWorld ) {
    World world;
    SerializationContext ctx;
    std::stringstream ss;
    ctx.Serialize( world, ss );
    EXPECT_EQ( ss.str(), "{\"entities\":[]}" );

    World newWorld;
    ctx.Deserialize( newWorld, ss.str() );
    EXPECT_EQ( newWorld.GetEntityCount(), 0 );
}

TEST( SerializationTest, InvalidJson ) {
    World world;
    SerializationContext ctx;
    auto res = ctx.Deserialize( world, "{ invalid json }" );
    EXPECT_FALSE( res.Success() );
    EXPECT_EQ( res.code, ErrorCode::InvalidJson );
}

TEST( SerializationTest, PartialComponents ) {
    World world;
    EntityID e1 = world.CreateEntity();
    world.AddComponent < Vec2 >( e1, { 1.0f, 2.0f } );

    SerializationContext ctx;
    ctx.Register < Vec2 >( "Vec2",
                           []( const Vec2& v, std::ostream& os ) {
                               os << "{\"x\":" << v.x << ",\"y\":" << v.y << "}";
                           },
                           []( const simdjson::dom::element& el ) {
                               return Vec2{ ( float ) el[ "x" ].get_double(), ( float ) el[ "y" ].get_double() };
                           }
    );

    std::stringstream ss;
    ctx.Serialize( world, ss );

    World newWorld;
    ctx.Deserialize( newWorld, ss.str() );

    EXPECT_TRUE( newWorld.HasComponent < Vec2 >( newWorld.GetEntityAt( 0 ) ) );
}

TEST( SerializationTest, UnknownComponent ) {
    World world;
    SerializationContext ctx;
    // Should just ignore unknown components
    ctx.Deserialize( world, "{\"entities\":[{\"id\":0,\"components\":{\"Unknown\":{}}}]}" );
    EXPECT_EQ( world.GetEntityCount(), 1 );
}

TEST( SerializationTest, MissingFields ) {
    World world;
    SerializationContext ctx;

    // Missing entities field - should just return
    ctx.Deserialize( world, "{\"something\":[]}" );
    EXPECT_EQ( world.GetEntityCount(), 0 );

    // Missing id in entity - should skip entity
    ctx.Deserialize( world, "{\"entities\":[{\"components\":{}}]}" );
    EXPECT_EQ( world.GetEntityCount(), 0 );

    // Missing components in entity - should create entity but no components
    ctx.Deserialize( world, "{\"entities\":[{\"id\":0}]}" );
    EXPECT_EQ( world.GetEntityCount(), 1 );
}

TEST( SerializationTest, MissingEntities ) {
    World world;
    SerializationContext ctx;
    auto res = ctx.Deserialize( world, "{\"something\":[]}" );
    EXPECT_FALSE( res.Success() );
    EXPECT_EQ( res.code, ErrorCode::MissingField );
}

TEST( SerializationTest, CustomErrorReporter ) {
    class MockReporter : public ErrorReporter {
    public:
        int count = 0;

        void Report( const Result& result ) override {
            count++;
        }
    };

    World world;
    SerializationContext ctx;
    MockReporter reporter;
    ctx.SetErrorReporter( &reporter );

    ctx.Deserialize( world, "{ invalid }" );
    EXPECT_EQ( reporter.count, 1 );
}

TEST( SerializationTest, MultipleEntities ) {
    World world;
    EntityID e1 = world.CreateEntity();
    world.CreateEntity();
    EntityID e3 = world.CreateEntity();
    world.AddComponent < Vec2 >( e3, { 3.0f, 4.0f } );

    world.DestroyEntity( e1 ); // Test gap in entity indices

    SerializationContext ctx;
    ctx.Register < Vec2 >( "Vec2",
                           []( const Vec2& v, std::ostream& os ) {
                               os << "{\"x\":" << v.x << ",\"y\":" << v.y << "}";
                           },
                           []( const simdjson::dom::element& el ) {
                               return Vec2{ ( float ) el[ "x" ].get_double(), ( float ) el[ "y" ].get_double() };
                           }
    );

    std::stringstream ss;
    ctx.Serialize( world, ss );

    World newWorld;
    ctx.Deserialize( newWorld, ss.str() );
    EXPECT_EQ( newWorld.GetEntityCount(), 3 ); // index 2 is the max
    EXPECT_FALSE( newWorld.IsAlive( e1 ) );
    EXPECT_TRUE( newWorld.IsAlive( e3 ) );
}

TEST( SerializationTest, BinarySerialization ) {
    World world;
    EntityID e1 = world.CreateEntity();
    world.AddComponent < Vec2 >( e1, { 1.0f, 2.0f } );
    world.AddComponent < Name >( e1, { "Entity1" } );

    SerializationContext ctx;
    ctx.RegisterBinary < Vec2 >( "Vec2",
                                 []( const Vec2& v, std::ostream& os ) {
                                     os.write( reinterpret_cast < const char* >( &v ), sizeof( v ) );
                                 },
                                 []( std::istream& is ) {
                                     Vec2 v;
                                     is.read( reinterpret_cast < char* >( &v ), sizeof( v ) );
                                     return v;
                                 }
    );
    ctx.RegisterBinary < Name >( "Name",
                                 []( const Name& n, std::ostream& os ) {
                                     uint32_t len = static_cast < uint32_t >( n.value.size() );
                                     os.write( reinterpret_cast < const char* >( &len ), sizeof( len ) );
                                     os.write( n.value.data(), len );
                                 },
                                 []( std::istream& is ) {
                                     uint32_t len;
                                     is.read( reinterpret_cast < char* >( &len ), sizeof( len ) );
                                     std::string s( len, '\0' );
                                     is.read( &s[ 0 ], len );
                                     return Name{ s };
                                 }
    );

    std::stringstream ss;
    ctx.SerializeBinary( world, ss );

    World newWorld;
    ctx.DeserializeBinary( newWorld, ss );

    EXPECT_TRUE( newWorld.IsAlive( e1 ) );
    bool found = false;
    newWorld.SystemUpdate( [&]( const Vec2& v, const Name& n ) {
        EXPECT_FLOAT_EQ( v.x, 1.0f );
        EXPECT_FLOAT_EQ( v.y, 2.0f );
        EXPECT_EQ( n.value, "Entity1" );
        found = true;
    } );
    EXPECT_TRUE( found );
}