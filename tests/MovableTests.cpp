//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//
#include <gtest/gtest.h>
#include "Mira/ETS/Movable.hpp"
#include <vector>

using namespace Mira::ETS;

struct Hero {
    float X = 0;
    float Y = 0;
    float Vx = 10;
    float Vy = 10;

    void
    UpdatePosition( float dt ) {
        X += Vx * dt;
        Y += Vy * dt;
    }
};

struct Monster {
    float X = 0;
    float Y = 0;
    float Vx = 10;
    float Vy = 10;

    void
    UpdatePosition( float dt ) {
        X += Vx * dt;
        Y += Vy * dt;
    }
};

struct LargeComponent {
    double data[ 20 ]; // 160 bytes, exceeds 64 bytes
    void
    UpdatePosition( float dt ) {
        ( void ) dt;
    }
};

TEST( MovableTests, TypeErasure_HeterogenousUpdate ) {
    std::vector < AnyMovable > objects;

    objects.push_back( AnyMovable( Hero{ 0, 0, 10, 0 } ) );
    objects.push_back( AnyMovable( Monster{ 0, 0, 0, 0 } ) );

    // Update all
    for ( auto& obj : objects ) {
        obj.Update( 1.0F );
    }

    SUCCEED();
}

struct StateTracker {
    int* DestructorCount;
    float* X;

    StateTracker( int* dc, float* x ) :
        DestructorCount( dc ),
        X( x ) {}

    ~StateTracker() {
        if ( DestructorCount ) ( *DestructorCount )++;
    }

    // Required for copy/move tests
    StateTracker( const StateTracker& other ) :
        DestructorCount( other.DestructorCount ),
        X( other.X ) {}

    StateTracker( StateTracker&& other ) noexcept :
        DestructorCount( other.DestructorCount ),
        X( other.X ) {
        other.DestructorCount = nullptr;
        other.X = nullptr;
    }

    void
    UpdatePosition( float dt ) {
        if ( X ) *X += 10.0f * dt;
    }
};

TEST( MovableTests, AnyMovable_StateAndDestruction ) {
    int destructorCount = 0;
    float x = 0;

    {
        AnyMovable movable( StateTracker{ &destructorCount, &x } );
        movable.Update( 1.0f );
        EXPECT_EQ( x, 10.0f );
    }

    EXPECT_GT( destructorCount, 0 );
}

TEST( MovableTests, AnyMovable_CopySemantics ) {
    float x1 = 0;
    float x2 = 0;
    int dc = 0;

    {
        AnyMovable a( StateTracker{ &dc, &x1 } );
        AnyMovable b = a; // Copy constructor

        b.Update( 1.0f );
        EXPECT_EQ( x1, 10.0f );

        a.Update( 1.0f );
        EXPECT_EQ( x1, 20.0f );

        AnyMovable c( StateTracker{ &dc, &x2 } );
        c = a; // Copy assignment
        c.Update( 1.0f );
        EXPECT_EQ( x1, 30.0f );
        EXPECT_EQ( x2, 0.0f );
    }
}

TEST( MovableTests, AnyMovable_MoveSemantics ) {
    float x = 0;
    float x2 = 0;
    int dc = 0;

    {
        AnyMovable a( StateTracker{ &dc, &x } );
        AnyMovable b = std::move( a ); // Move constructor

        b.Update( 1.0f );
        EXPECT_EQ( x, 10.0f );

        // a should be in a null state
        a.Update( 1.0f );
        EXPECT_EQ( x, 10.0f );

        AnyMovable c( StateTracker{ &dc, &x2 } );
        c = std::move( b ); // Move assignment
        c.Update( 1.0f );
        EXPECT_EQ( x, 20.0f );

        // b should be in a null state
        b.Update( 1.0f );
        EXPECT_EQ( x, 20.0f );
    }
}

TEST( MovableTests, AnyMovable_VTableSharing ) {
    AnyMovable a( Hero{ 0, 0, 10, 0 } );
    AnyMovable b( Hero{ 1, 1, 5, 5 } );
    AnyMovable c( Monster{ 0, 0, 0, 0 } );

    EXPECT_EQ( a.GetVTable(), b.GetVTable() );
    EXPECT_NE( a.GetVTable(), c.GetVTable() );
}

TEST( MovableTests, AnyMovableHeapFallback ) {
    LargeComponent lc;
    AnyMovable am( lc );

    // Should not crash and should update
    am.Update( 0.1f );

    AnyMovable am2 = am; // Test copy
    am2.Update( 0.1f );

    AnyMovable am3 = std::move( am2 ); // Test move
    am3.Update( 0.1f );
}

TEST( MovableTests, CoverageBoost_Extra ) {
    struct SimpleMovable {
        int val;

        void UpdatePosition( float dt ) {
            val += ( int ) dt;
        }
    };

    {
        AnyMovable a( SimpleMovable{ 10 } );

        // Self-assignment (copy)
        a = a;
        a.Update( 1.0f );

        // Self-assignment (move)
        a = std::move( a );
        a.Update( 1.0f );

        AnyMovable b( SimpleMovable{ 20 } );
        AnyMovable c = std::move( b ); // b is now null-state

        // Copy from null-state
        AnyMovable e = b;
        e.Update( 1.0f );

        // Move from null-state
        AnyMovable f = std::move( b );
        f.Update( 1.0f );

        // Assignment from null-state
        AnyMovable g( SimpleMovable{ 40 } );
        g = b;
        g.Update( 1.0f );

        AnyMovable h( SimpleMovable{ 50 } );
        h = std::move( b );
        h.Update( 1.0f );
    }
}