#include <gtest/gtest.h>
#include <Mira/ETS/Prefab.hpp>
#include <Mira/ETS/World.hpp>

using namespace Mira::ETS;

struct Position {
    float x, y;
};

struct Health {
    int value;
};

class PrefabTest : public ::testing::Test {
protected:
    World world;
    SerializationContext ctx;

    void
    SetUp() override {
        ctx.Register < Position >( "Position",
                                   []( const Position& p, std::ostream& os ) {
                                       os << "{\"x\":" << p.x << ",\"y\":" << p.y << "}";
                                   },
                                   []( const simdjson::dom::element& el ) {
                                       return Position{
                                           ( float ) el[ "x" ].get_double(), ( float ) el[ "y" ].get_double()
                                       };
                                   }
        );

        ctx.Register < Health >( "Health",
                                 []( const Health& h, std::ostream& os ) {
                                     os << "{\"value\":" << h.value << "}";
                                 },
                                 []( const simdjson::dom::element& el ) {
                                     return Health{ ( int ) el[ "value" ].get_int64() };
                                 }
        );
    }
};

TEST_F( PrefabTest, BasicInstantiation ) {
    PrefabManager pm( ctx );

    std::string json = R"(
    {
        "Warrior": {
            "Position": {"x": 10, "y": 20},
            "Health": {"value": 100}
        },
        "Ghost": {
            "Position": {"x": 50, "y": 50}
        }
    }
    )";

    pm.LoadPrefabs( json );

    EntityID warrior = pm.Instantiate( "Warrior", world );
    ASSERT_NE( warrior, k_NullIndex );
    EXPECT_TRUE( world.HasComponent < Position >( warrior ) );
    EXPECT_TRUE( world.HasComponent < Health >( warrior ) );
    EXPECT_EQ( world.GetComponent < Position >( warrior ).x, 10 );
    EXPECT_EQ( world.GetComponent < Health >( warrior ).value, 100 );

    EntityID ghost = pm.Instantiate( "Ghost", world );
    ASSERT_NE( ghost, k_NullIndex );
    EXPECT_TRUE( world.HasComponent < Position >( ghost ) );
    EXPECT_FALSE( world.HasComponent < Health >( ghost ) );
    EXPECT_EQ( world.GetComponent < Position >( ghost ).x, 50 );
}

TEST_F( PrefabTest, UnknownPrefab ) {
    PrefabManager pm( ctx );
    pm.LoadPrefabs( "{}" );

    EntityID id = pm.Instantiate( "Unknown", world );
    EXPECT_EQ( id, k_NullIndex );
}

TEST_F( PrefabTest, InvalidJson ) {
    PrefabManager pm( ctx );
    auto res = pm.LoadPrefabs( "{ invalid }" );
    EXPECT_FALSE( res.Success() );
    EXPECT_EQ( res.code, ErrorCode::InvalidJson );

    res = pm.LoadPrefabs( "[]" );
    EXPECT_FALSE( res.Success() );
    EXPECT_EQ( res.code, ErrorCode::TypeMismatch );
}

TEST_F( PrefabTest, MultipleLoads ) {
    PrefabManager pm( ctx );
    pm.LoadPrefabs( R"({"A": {"Position": {"x": 1, "y": 1}}})" );
    pm.LoadPrefabs( R"({"B": {"Position": {"x": 2, "y": 2}}})" );

    EntityID a = pm.Instantiate( "A", world );
    EntityID b = pm.Instantiate( "B", world );

    EXPECT_NE( a, k_NullIndex );
    EXPECT_NE( b, k_NullIndex );
    EXPECT_EQ( world.GetComponent < Position >( a ).x, 1 );
    EXPECT_EQ( world.GetComponent < Position >( b ).x, 2 );
}

TEST_F( PrefabTest, UnknownComponentInPrefab ) {
    PrefabManager pm( ctx );
    pm.LoadPrefabs( R"({"Special": {"UnknownComp": {}, "Position": {"x": 5, "y": 5}}})" );

    EntityID id = pm.Instantiate( "Special", world );
    EXPECT_NE( id, k_NullIndex );
    EXPECT_TRUE( world.HasComponent < Position >( id ) );
    // Unknown component should just be skipped without crashing
}

TEST_F( PrefabTest, NonObjectPrefabData ) {
    PrefabManager pm( ctx );
    // Prefab B is not an object, so it should be skipped during loading
    pm.LoadPrefabs( R"({"A": {"Position": {"x": 1, "y": 1}}, "B": 123})" );

    EXPECT_NE( pm.Instantiate( "A", world ), k_NullIndex );
    EXPECT_EQ( pm.Instantiate( "B", world ), k_NullIndex );
}

TEST_F( PrefabTest, EmptyJson ) {
    PrefabManager pm( ctx );
    pm.LoadPrefabs( "{}" );
    EXPECT_EQ( pm.Instantiate( "Any", world ), k_NullIndex );
}

TEST_F( PrefabTest, NestedJsonNotObject ) {
    PrefabManager pm( ctx );
    // Top level must be an object
    auto res = pm.LoadPrefabs( "123" );
    EXPECT_FALSE( res.Success() );
    EXPECT_EQ( res.code, ErrorCode::TypeMismatch );

    res = pm.LoadPrefabs( "\"string\"" );
    EXPECT_FALSE( res.Success() );
    EXPECT_EQ( res.code, ErrorCode::TypeMismatch );
}

TEST_F( PrefabTest, ComponentDataNotObject ) {
    PrefabManager pm( ctx );
    // Even if component data is not an object, DeserializeComponent should handle it
    // depending on the registered deserializer.
    ctx.Register < int >( "Int",
                          []( const int& i, std::ostream& os ) {
                              os << i;
                          },
                          []( const simdjson::dom::element& el ) {
                              return ( int ) el.get_int64();
                          }
    );

    pm.LoadPrefabs( R"({"A": {"Int": 42}})" );
    EntityID a = pm.Instantiate( "A", world );
    EXPECT_NE( a, k_NullIndex );
    EXPECT_TRUE( world.HasComponent < int >( a ) );
    EXPECT_EQ( world.GetComponent < int >( a ), 42 );
}