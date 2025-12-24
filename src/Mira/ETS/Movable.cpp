#include "Mira/ETS/Movable.hpp"

namespace Mira::ETS {
    AnyMovable::~AnyMovable() {
        if ( m_VTablePtr&& m_VTablePtr
        ->
        DestroyFn
        )
        {
            m_VTablePtr->DestroyFn( m_Ptr );
            if ( m_Ptr != &m_Buffer ) {
                ::operator delete( m_Ptr );
            }
        }
    }

    AnyMovable::AnyMovable( const AnyMovable& other ) :
        m_VTablePtr( other.m_VTablePtr ) {
        if ( m_VTablePtr&& m_VTablePtr
        ->
        CloneFn
        )
        {
            m_Ptr = &m_Buffer;
            m_VTablePtr->CloneFn( other.m_Ptr, &m_Ptr );
        }
    }

    AnyMovable&
    AnyMovable::operator=( const AnyMovable& other ) {
        if ( this != &other ) {
            if ( m_VTablePtr&& m_VTablePtr
            ->
            DestroyFn
            )
            {
                m_VTablePtr->DestroyFn( m_Ptr );
                if ( m_Ptr != &m_Buffer ) {
                    ::operator delete( m_Ptr );
                }
            }
            m_VTablePtr = other.m_VTablePtr;
            if ( m_VTablePtr&& m_VTablePtr
            ->
            CloneFn
            )
            {
                m_Ptr = &m_Buffer;
                m_VTablePtr->CloneFn( other.m_Ptr, &m_Ptr );
            }
        }
        return *this;
    }

    AnyMovable::AnyMovable( AnyMovable&& other ) noexcept :
        m_VTablePtr( other.m_VTablePtr ) {
        if ( m_VTablePtr&& m_VTablePtr
        ->
        MoveFn
        )
        {
            if ( other.m_Ptr == &other.m_Buffer ) {
                m_Ptr = &m_Buffer;
                m_VTablePtr->MoveFn( other.m_Ptr, &m_Ptr );
            } else {
                m_Ptr = other.m_Ptr;
                other.m_Ptr = &other.m_Buffer;
            }
            other.m_VTablePtr = nullptr;
        }
    }

    AnyMovable&
    AnyMovable::operator=( AnyMovable&& other ) noexcept {
        if ( this != &other ) {
            if ( m_VTablePtr&& m_VTablePtr
            ->
            DestroyFn
            )
            {
                m_VTablePtr->DestroyFn( m_Ptr );
                if ( m_Ptr != &m_Buffer ) {
                    ::operator delete( m_Ptr );
                }
            }
            m_VTablePtr = other.m_VTablePtr;
            if ( m_VTablePtr&& m_VTablePtr
            ->
            MoveFn
            )
            {
                if ( other.m_Ptr == &other.m_Buffer ) {
                    m_Ptr = &m_Buffer;
                    m_VTablePtr->MoveFn( other.m_Ptr, &m_Ptr );
                } else {
                    m_Ptr = other.m_Ptr;
                    other.m_Ptr = &other.m_Buffer;
                }
                other.m_VTablePtr = nullptr;
            }
        }
        return *this;
    }

    void
    AnyMovable::Update( float dt ) {
        if ( m_VTablePtr&& m_VTablePtr
        ->
        UpdateFn
        )
        {
            m_VTablePtr->UpdateFn( m_Ptr, dt );
        }
    }

    const void*
    AnyMovable::GetVTable() const {
        return m_VTablePtr;
    }
} // namespace Mira::ETS