//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//
#include <gtest/gtest.h>
#include "Mira/ETS/Traits.hpp"

using namespace Mira::ETS;

TEST( TraitsTests, StaticPolymorphism_UpdatePosition ) {
    // Arrange
    GameEntity player( 1, 0.0F, 0.0F, 10.0F, 5.0F );

    // Act
    player.Update( 1.0F );

    EXPECT_FLOAT_EQ( player.X, 10.0F );
    EXPECT_FLOAT_EQ( player.Y, 5.0F );
}

TEST( TraitsTests, StaticPolymorphism_StatusString ) {
    // Arrange
    GameEntity player( 99, 10.0F, 20.0F, 0.0F, 0.0F );

    // Act
    std::string status = player.GetStatusString();

    EXPECT_EQ( status, "Entity[99] Pos: (10.00, 20.00)" );
}