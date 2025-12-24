//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

/**
 * @file Prefab.hpp
 * @brief Prefab system for instantiating entities from templates.
 */

#pragma once

#include <Mira/ETS/Serialization.hpp>
#include <simdjson.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Mira::ETS {
    /**
     * @brief Manages entity templates (Prefabs) that can be instantiated with pre-configured component sets.
     */
    class PrefabManager {
    public:
        /**
         * @brief Construct a new Prefab Manager
         * @param context The serialization context used for component deserialization
         */
        explicit
        PrefabManager( SerializationContext& context );

        /**
         * @brief Load prefabs from a JSON string
         * @param json JSON string containing prefab definitions
         * @return Result of the operation
         * 
         * The JSON should be an object where each key is a prefab name and its value 
         * is an object containing component names and their data.
         * Example:
         * {
         *   "Player": {
         *     "Position": {"x": 0, "y": 0},
         *     "Velocity": {"x": 5, "y": 5}
         *   }
         * }
         */
        Result
        LoadPrefabs( const std::string& json );

        /**
         * @brief Instantiate a prefab into a world
         * @param prefabName The name of the prefab to instantiate
         * @param world The world to create the entity in
         * @return EntityID The ID of the newly created entity, or k_NullIndex if failed
         */
        EntityID
        Instantiate( const std::string& prefabName, World& world );

        /**
         * @brief Set the error reporter for this manager.
         * @param reporter Pointer to the error reporter
         */
        void
        SetErrorReporter( ErrorReporter* reporter ) {
            m_Reporter = reporter;
        }

    private:
        SerializationContext& m_Context;
        std::vector < std::unique_ptr < simdjson::dom::parser > > m_Parsers;
        std::unordered_map < std::string, simdjson::dom::object > m_Prefabs;
        ErrorReporter* m_Reporter = &DefaultErrorReporter;
    };
} // namespace Mira::ETS