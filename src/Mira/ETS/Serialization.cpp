#include <Mira/ETS/Serialization.hpp>
#include <iostream>
#include <sstream>

namespace Mira::ETS {
    Result
    SerializationContext::Serialize( World& world, std::ostream& os ) {
        std::string buffer;
        buffer.reserve( 8192 );
        buffer.append( "{\"entities\":[" );

        bool firstEntity = true;
        uint32_t entityCount = static_cast < uint32_t >( world.GetEntityCount() );

        for ( uint32_t i = 0; i < entityCount; ++i ) {
            EntityID id = world.GetEntityAt( i );
            if ( world.IsAlive( id ) ) {
                if ( !firstEntity ) buffer.append( "," );
                firstEntity = false;

                buffer.append( "{\"id\":" );
                buffer.append( std::to_string( id ) );
                buffer.append( ",\"components\":{" );

                const auto& mask = world.GetEntityMask( id );
                bool firstComp = true;

                for ( size_t cid = 0; cid < m_SerializersByID.size(); ++cid ) {
                    if ( cid < mask.Size() && mask.Test( cid ) ) {
                        auto* serializer = m_SerializersByID[ cid ];
                        if ( serializer && serializer->serialize ) {
                            if ( !firstComp ) buffer.append( "," );
                            firstComp = false;

                            buffer.append( "\"" );
                            buffer.append( serializer->name );
                            buffer.append( "\":" );

                            os << buffer;
                            buffer.clear();

                            serializer->serialize( id, world, os );
                        }
                    }
                }
                buffer.append( "}}" );
            }

            if ( buffer.size() > 4096 ) {
                os << buffer;
                buffer.clear();
            }
        }

        buffer.append( "]}" );
        os << buffer;
        return Result::Ok();
    }

    Result
    SerializationContext::Deserialize( World& world, const std::string& json ) {
        simdjson::dom::parser parser;
        simdjson::dom::element doc;
        auto error = parser.parse( json ).get( doc );
        if ( error ) {
            Result res = Result::Error( ErrorCode::InvalidJson,
                                        "JSON parse error: " + std::string( simdjson::error_message( error ) ) );
            if ( m_Reporter ) m_Reporter->Report( res );
            return res;
        }

        simdjson::dom::array entities;
        error = doc[ "entities" ].get( entities );
        if ( error ) {
            Result res = Result::Error( ErrorCode::MissingField, "Missing 'entities' array in JSON" );
            if ( m_Reporter ) m_Reporter->Report( res );
            return res;
        }

        for ( auto entity_el : entities ) {
            uint64_t id_raw;
            if ( entity_el[ "id" ].get( id_raw ) ) {
                if ( m_Reporter ) m_Reporter->Report( Result::Error( ErrorCode::MissingField, "Entity missing 'id'" ) );
                continue;
            }

            EntityID id = static_cast < EntityID >( id_raw );
            EntityID newId = world.CreateEntity( id );

            simdjson::dom::object components;
            if ( entity_el[ "components" ].get( components ) ) {
                if ( entity_el[ "components" ].error() != simdjson::NO_SUCH_FIELD ) {
                    if ( m_Reporter )
                        m_Reporter->Report(
                            Result::Error( ErrorCode::TypeMismatch, "Entity 'components' must be an object" ) );
                }
                continue;
            }

            for ( auto field : components ) {
                std::string name( field.key );
                DeserializeComponent( newId, world, name, field.value );
            }
        }
        return Result::Ok();
    }

    Result
    SerializationContext::DeserializeComponent( EntityID id, World& world, const std::string& name,
                                                const simdjson::dom::element& el ) {
        auto it = m_SerializersByName.find( name );
        if ( it != m_SerializersByName.end() ) {
            if ( !it->second.deserialize ) {
                Result res = Result::Error( ErrorCode::InternalError,
                                            "Component registered but lacks JSON deserializer: " + name );
                if ( m_Reporter ) m_Reporter->Report( res );
                return res;
            }
            try {
                it->second.deserialize( id, world, el );
                return Result::Ok();
            } catch ( const std::exception& e ) {
                Result res = Result::Error( ErrorCode::InternalError,
                                            "Exception during deserialization of " + name + ": " + e.what() );
                if ( m_Reporter ) m_Reporter->Report( res );
                return res;
            }
        } else {
            Result res = Result::Error( ErrorCode::ComponentNotRegistered, "Component not registered: " + name );
            if ( m_Reporter ) m_Reporter->Report( res );
            return res;
        }
    }

    Result
    SerializationContext::SerializeBinary( World& world, std::ostream& os ) {
        uint32_t entityCount = 0;
        uint32_t totalEntities = static_cast < uint32_t >( world.GetEntityCount() );
        for ( uint32_t i = 0; i < totalEntities; ++i ) {
            if ( world.IsAlive( world.GetEntityAt( i ) ) ) entityCount++;
        }

        os.write( reinterpret_cast < const char* >( &entityCount ), sizeof( entityCount ) );

        for ( uint32_t i = 0; i < totalEntities; ++i ) {
            EntityID id = world.GetEntityAt( i );
            if ( world.IsAlive( id ) ) {
                os.write( reinterpret_cast < const char* >( &id ), sizeof( id ) );

                const auto& mask = world.GetEntityMask( id );
                uint32_t componentCount = 0;
                for ( size_t cid = 0; cid < m_SerializersByID.size(); ++cid ) {
                    if ( cid < mask.Size() && mask.Test( cid ) ) {
                        auto* serializer = m_SerializersByID[ cid ];
                        if ( serializer && serializer->binarySerialize ) {
                            componentCount++;
                        }
                    }
                }

                os.write( reinterpret_cast < const char* >( &componentCount ), sizeof( componentCount ) );

                for ( size_t cid = 0; cid < m_SerializersByID.size(); ++cid ) {
                    if ( cid < mask.Size() && mask.Test( cid ) ) {
                        auto* serializer = m_SerializersByID[ cid ];
                        if ( serializer && serializer->binarySerialize ) {
                            uint32_t nameLen = static_cast < uint32_t >( serializer->name.size() );
                            os.write( reinterpret_cast < const char* >( &nameLen ), sizeof( nameLen ) );
                            os.write( serializer->name.data(), nameLen );

                            serializer->binarySerialize( id, world, os );
                        }
                    }
                }
            }
        }
        return Result::Ok();
    }

    Result
    SerializationContext::DeserializeBinary( World& world, std::istream& is ) {
        uint32_t entityCount;
        if ( !is.read( reinterpret_cast < char* >( &entityCount ), sizeof( entityCount ) ) ) {
            return Result::Ok(); // Empty or failed to read
        }

        for ( uint32_t i = 0; i < entityCount; ++i ) {
            EntityID id;
            if ( !is.read( reinterpret_cast < char* >( &id ), sizeof( id ) ) ) break;

            EntityID newId = world.CreateEntity( id );

            uint32_t componentCount;
            if ( !is.read( reinterpret_cast < char* >( &componentCount ), sizeof( componentCount ) ) ) break;

            for ( uint32_t j = 0; j < componentCount; ++j ) {
                uint32_t nameLen;
                if ( !is.read( reinterpret_cast < char* >( &nameLen ), sizeof( nameLen ) ) ) break;

                std::string name( nameLen, '\0' );
                if ( !is.read( &name[ 0 ], nameLen ) ) break;

                auto it = m_SerializersByName.find( name );
                if ( it != m_SerializersByName.end() && it->second.binaryDeserialize ) {
                    it->second.binaryDeserialize( newId, world, is );
                } else {
                    Result res = Result::Error( ErrorCode::ComponentNotRegistered,
                                                "Component not registered or lacks binary deserializer: " + name );
                    if ( m_Reporter ) m_Reporter->Report( res );
                    return res;
                }
            }
        }
        return Result::Ok();
    }
} // namespace Mira::ETS
