#include <Mira/ETS/Prefab.hpp>
#include <stdexcept>

namespace Mira::ETS {
    PrefabManager::PrefabManager( SerializationContext& context ) :
        m_Context( context ) {}

    Result
    PrefabManager::LoadPrefabs( const std::string& json ) {
        auto parser = std::make_unique < simdjson::dom::parser >();
        simdjson::dom::element doc;
        auto error = parser->parse( json ).get( doc );

        if ( error ) {
            Result res = Result::Error( ErrorCode::InvalidJson,
                                        "JSON parse error in LoadPrefabs: " + std::string(
                                            simdjson::error_message( error ) ) );
            if ( m_Reporter ) m_Reporter->Report( res );
            return res;
        }

        simdjson::dom::object obj;
        error = doc.get( obj );
        if ( error ) {
            Result res = Result::Error( ErrorCode::TypeMismatch, "Prefab JSON must be an object at the top level" );
            if ( m_Reporter ) m_Reporter->Report( res );
            return res;
        }

        for ( auto field : obj ) {
            std::string name( field.key );
            simdjson::dom::object prefab_data;
            if ( field.value.get( prefab_data ) == simdjson::SUCCESS ) {
                m_Prefabs[ name ] = prefab_data;
            } else {
                if ( m_Reporter ) m_Reporter->Report(
                    Result::Error( ErrorCode::TypeMismatch, "Prefab '" + name + "' data must be an object" ) );
            }
        }

        m_Parsers.push_back( std::move( parser ) );
        return Result::Ok();
    }

    EntityID
    PrefabManager::Instantiate( const std::string& prefabName, World& world ) {
        auto it = m_Prefabs.find( prefabName );
        if ( it == m_Prefabs.end() ) {
            if ( m_Reporter ) m_Reporter->Report(
                Result::Error( ErrorCode::UnknownPrefab, "Unknown prefab: " + prefabName ) );
            return k_NullIndex;
        }

        EntityID entity = world.CreateEntity();
        simdjson::dom::object components = it->second;

        for ( auto field : components ) {
            std::string_view compName = field.key;
            m_Context.DeserializeComponent( entity, world, std::string( compName ), field.value );
        }

        return entity;
    }
} // namespace Mira::ETS