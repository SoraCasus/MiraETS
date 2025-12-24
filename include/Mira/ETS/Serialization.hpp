//
// Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
//

/**
 * @file Serialization.hpp
 * @brief Serialization and deserialization utilities for the ETS world.
 */

#pragma once

#include <Mira/ETS/World.hpp>
#include <Mira/ETS/Errors.hpp>
#include <simdjson.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <ostream>
#include <typeindex>

namespace Mira::ETS {
    /**
     * @brief Context used to register component serializers and perform world serialization.
     */
    class SerializationContext {
    public:
        using SerializeFn = std::function < void( EntityID, World &, std::ostream & ) >;
        using DeserializeFn = std::function < void( EntityID, World &, const simdjson::dom::element & ) >;
        using BinarySerializeFn = std::function < void( EntityID, World &, std::ostream & ) >;
        using BinaryDeserializeFn = std::function < void( EntityID, World &, std::istream & ) >;

        /**
         * @brief Internal struct to hold serialization functions for a component type.
         */
        struct ComponentSerializer {
            std::string name;
            size_t id;
            SerializeFn serialize;
            DeserializeFn deserialize;
            BinarySerializeFn binarySerialize;
            BinaryDeserializeFn binaryDeserialize;
        };

        /**
         * @brief Register a component type for serialization.
         * @tparam T Component type
         * @param name Unique string identifier for this component type
         * @param serialize Function to serialize a single component of type T to JSON
         * @param deserialize Function to deserialize a single component of type T from JSON
         */
        template< typename T >
        void
        Register( const std::string& name,
                  std::function < void( const T &, std::ostream & ) > serialize,
                  std::function < T( const simdjson::dom::element & ) > deserialize ) {
            size_t cid = ComponentID < T >::Value();
            auto& serializer = m_SerializersByName[ name ];
            serializer.name = name;
            serializer.id = cid;
            serializer.serialize = [serialize]( EntityID id, World& world, std::ostream& os ) {
                serialize( world.GetComponent < T >( id ), os );
            };
            serializer.deserialize = [deserialize]( EntityID id, World& world, const simdjson::dom::element& el ) {
                world.AddComponent < T >( id, deserialize( el ) );
            };

            if ( cid >= m_SerializersByID.size() ) {
                m_SerializersByID.resize( cid + 1, nullptr );
            }
            m_SerializersByID[ cid ] = &serializer;
            m_ComponentTypeToName[ typeid( T ) ] = name;
        }

        /**
         * @brief Register binary serialization functions for a component type.
         * @tparam T Component type
         * @param name Unique string identifier for this component type (must match JSON name)
         * @param serialize Function to serialize a single component of type T to binary
         * @param deserialize Function to deserialize a single component of type T from binary
         */
        template< typename T >
        void
        RegisterBinary( const std::string& name,
                        std::function < void( const T &, std::ostream & ) > serialize,
                        std::function < T( std::istream & ) > deserialize ) {
            size_t cid = ComponentID < T >::Value();
            auto& serializer = m_SerializersByName[ name ];
            serializer.name = name;
            serializer.id = cid;
            serializer.binarySerialize = [serialize]( EntityID id, World& world, std::ostream& os ) {
                serialize( world.GetComponent < T >( id ), os );
            };
            serializer.binaryDeserialize = [deserialize]( EntityID id, World& world, std::istream& is ) {
                world.AddComponent < T >( id, deserialize( is ) );
            };

            if ( cid >= m_SerializersByID.size() ) {
                m_SerializersByID.resize( cid + 1, nullptr );
            }
            m_SerializersByID[ cid ] = &serializer;
            m_ComponentTypeToName[ typeid( T ) ] = name;
        }

        /**
         * @brief Serialize the entire world to an output stream in JSON format.
         * @param world World to serialize
         * @param os Output stream
         * @return Result of the operation
         */
        Result
        Serialize( World& world, std::ostream& os );

        /**
         * @brief Deserialize components into a world from a JSON string.
         * @param world World to populate
         * @param json JSON string
         * @return Result of the operation
         */
        Result
        Deserialize( World& world, const std::string& json );

        /**
         * @brief Serialize the entire world to an output stream in binary format.
         * @param world World to serialize
         * @param os Output stream
         * @return Result of the operation
         */
        Result
        SerializeBinary( World& world, std::ostream& os );

        /**
         * @brief Deserialize components into a world from a binary input stream.
         * @param world World to populate
         * @param is Input stream
         * @return Result of the operation
         */
        Result
        DeserializeBinary( World& world, std::istream& is );

        /**
         * @brief Deserializes a single component into an entity.
         * @param id Entity to add the component to
         * @param world World containing the entity
         * @param name Name of the component type
         * @param el simdjson element containing the component data
         * @return Result of the operation
         */
        Result
        DeserializeComponent( EntityID id, World& world, const std::string& name, const simdjson::dom::element& el );

        /**
         * @brief Set the error reporter for this context.
         * @param reporter Pointer to the error reporter
         */
        void
        SetErrorReporter( ErrorReporter* reporter ) {
            m_Reporter = reporter;
        }

    private:
        std::unordered_map < std::string, ComponentSerializer > m_SerializersByName;
        std::vector < ComponentSerializer* > m_SerializersByID;
        std::unordered_map < std::type_index, std::string > m_ComponentTypeToName;
        ErrorReporter* m_Reporter = &DefaultErrorReporter;
    };
} // namespace Mira::ETS