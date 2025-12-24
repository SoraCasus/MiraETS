//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//
#include <gtest/gtest.h>
#include "Mira/ETS/SparseSet.hpp"
#include <string>

using namespace Mira::ETS;

TEST( SparseSetTests, InsertAndRetrieve ) {
    SparseSet < float > health;
    health.Insert( 10, 100.0F );
    health.Insert( 5, 50.0F );

    ASSERT_TRUE( health.Contains( 10 ) );
    ASSERT_TRUE( health.Contains( 5 ) );
    ASSERT_FALSE( health.Contains( 1 ) );

    EXPECT_FLOAT_EQ( health.Get( 10 ), 100.0F );
    EXPECT_FLOAT_EQ( health.Get( 5 ), 50.0F );
}

TEST( SparseSetTests, Removal ) {
    SparseSet < int > items;
    items.Insert( 1, 100 );
    items.Insert( 2, 200 );
    items.Insert( 3, 300 );

    items.Remove( 2 );

    ASSERT_FALSE( items.Contains( 2 ) );
    ASSERT_TRUE( items.Contains( 1 ) );
    ASSERT_TRUE( items.Contains( 3 ) );

    EXPECT_EQ( items.Get( 3 ), 300 );
    EXPECT_EQ( items.Get( 1 ), 100 );
    EXPECT_EQ( items.Size(), 2 );
}

TEST( SparseSetTests, ContiguousIteration ) {
    SparseSet < int > items;
    items.Insert( 1, 100 );
    items.Insert( 2, 200 );
    items.Insert( 3, 300 );

    int sum = 0;
    for ( int i : items.GetData() ) {
        sum += i;
    }
    EXPECT_EQ( sum, 600 );
}

TEST( SparseSetTests, GenerationValidation ) {
    SparseSet < std::string > names;

    // Entity 1, Gen 0
    EntityID e1_g0 = 1;
    names.Insert( e1_g0, "Original" );

    // Entity 1, Gen 1 (ABA simulation)
    EntityID e1_g1 = ( 1ULL << 32 ) | 1ULL;

    EXPECT_TRUE( names.Contains( e1_g0 ) );
    EXPECT_FALSE( names.Contains( e1_g1 ) );

    names.Remove( e1_g1 ); // Should NOT remove e1_g0
    EXPECT_TRUE( names.Contains( e1_g0 ) );
    EXPECT_EQ( names.Size(), 1 );

    names.Insert( e1_g1, "New" );

    EXPECT_FALSE( names.Contains( e1_g0 ) );
    EXPECT_TRUE( names.Contains( e1_g1 ) );
    EXPECT_EQ( names.Get( e1_g1 ), "New" );
}

TEST( SparseSetTests, PagedAllocation ) {
    SparseSet < int > data;

    EntityID e_far = 1000000; // High index to force multiple pages
    data.Insert( e_far, 42 );

    EXPECT_TRUE( data.Contains( e_far ) );
    EXPECT_EQ( data.Get( e_far ), 42 );
    EXPECT_EQ( data.Size(), 1 );
}

TEST( SparseSetTests, CoverageBoost_Extra ) {
    SparseSet < int > set;

    // 1. Remove from non-existent page
    set.Remove( 999999 );

    // 2. Remove non-existent entity from existing page
    set.Insert( 1, 100 );
    set.Remove( 2 );

    // 3. Update existing component
    set.Insert( 1, 200 );
    EXPECT_EQ( set.Get( 1 ), 200 );

    // 4. Remove last element (to cover the 'else' of swap-and-pop check)
    set.Insert( 3, 300 );
    set.Remove( 3 ); // 3 is the last element
    EXPECT_FALSE( set.Contains( 3 ) );
    EXPECT_TRUE( set.Contains( 1 ) );

    // 5. Contains from non-existent page
    EXPECT_FALSE( set.Contains( 888888 ) );

    // 6. SparseSet coverage for various types
    SparseSet < float > fset;
    fset.Insert( 1, 1.0f );
    fset.Insert( 1, 2.0f );
    fset.Remove( 1 );
    fset.Remove( 1 );
    fset.Remove( 999999 );

    SparseSet < std::string > sset;
    sset.Insert( 1, "hello" );
    sset.Insert( 1, "world" );
    sset.Remove( 1 );

    // 7. Iterators and Size
    set.Begin();
    set.End();
    set.Size();
    set.GetData();
    const auto& constSet = set;
    constSet.GetData();
}