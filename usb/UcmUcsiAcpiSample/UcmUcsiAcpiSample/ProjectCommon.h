/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ProjectCommon.h

Abstract:

    Common routines, types, macros, etc., for the UCM-UCSI project.

Environment:

    Kernel-mode.

--*/

#pragma once

// Code section placement macros.

#define PAGED_CODE_SEG __declspec(code_seg("PAGE"))
#define INIT_CODE_SEG __declspec(code_seg("INIT"))

// The above macros are more convenient to use, but use these instead if for some reason the
// functions can't be declared as paged/non-paged, e.g., if the function is virtual and it doesn't
// make sense for the overridden function to be declared paged/non-paged.

#define PAGED_CODE_SEG_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("PAGE"))

#define PAGED_CODE_SEG_END \
    __pragma(code_seg(pop))

// Custom assert macros that:
// - behave like NT_ASSERT for chk binaries
// - break into verifier if it is enabled, regardless of whether binary is chk or fre.
//
// The expression will *not* be evaluated if a failure will not cause a break, i.e., if not chk
// binary and verifier is not enabled. However, the code expression will always be compiled into the code.

#if DBG

#define UCMUCSI_ASSERTMSG(_exp, _msg) \
    __pragma(warning(suppress: __WARNING_TRUE_CONSTANT_EXPR_IN_AND)) \
    NT_VERIFYMSG(_msg, _exp)

#define UCMUCSI_ASSERT(_exp) \
    __pragma(warning(suppress: __WARNING_TRUE_CONSTANT_EXPR_IN_AND)) \
    NT_VERIFY(_exp)

#else // #if DBG

#define UCMUCSI_ASSERTMSG(_exp, _msg) \
    __pragma(warning(suppress: __WARNING_TRUE_CONSTANT_EXPR_IN_AND)) \
    WDFVERIFY(_exp)

#define UCMUCSI_ASSERT(_exp) \
    __pragma(warning(suppress: __WARNING_TRUE_CONSTANT_EXPR_IN_AND)) \
    WDFVERIFY(_exp)

#endif // #if DBG


// Intellisense definition for DbgRaiseAssertionFailure because for some reason Visual Studio can't
// find it.
#ifdef __INTELLISENSE__
#define DbgRaiseAssertionFailure() ((void) 0)
#endif


namespace WdfCpp
{

// General note: The class(es) below use SFINAE to set the object callbacks only if the derived
// class has methods with specific names.

// Templatized base class for creating C++ objects within the context-space of a WDFOBJECT.
template<
    typename ObjectType,    // The underlying WDF object type (e.g., WDFDEVICE).
    typename ContextType    // The context class type that derives from this.
    >
class ObjectContext
{
public:

    using HandleType = ObjectType;

    // Typedef for an object cleanup method. If you want to override the cleanup callback for
    // the WDF object, provide a method in the derived class with this type and name
    // "EvtObjectContextCleanup". You can omit it if you don't want to override cleanup.
    typedef
    _IRQL_requires_max_(DISPATCH_LEVEL)
    void
    EVT_OBJECT_CONTEXT_CLEANUP ();

    // Provide a method of this type and name "GetContextFromObject" in the derived class. The
    // implementation of this method would typically just call the context accessor method that
    // WDF defines for you. This is required; code will fail to compile if you do not provide this
    // method.
    typedef
    ContextType*
    GET_CONTEXT_FROM_OBJECT (
        _In_ ObjectType Object
        );

    ObjectContext&
    operator= (
        _In_ const ObjectContext&
        ) = delete;

    ObjectContext (
        _In_ const ObjectContext&
        ) = delete;

    // Call this static method to set the callback pointers in the WDF_OBJECT_ATTRIBUTES after it
    // has been initialized with WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE.
    static
    void
    ObjectAttributesInit (
        _Inout_ PWDF_OBJECT_ATTRIBUTES Attributes
        )
    {
        SetEvtObjectContextCleanup(Attributes, true);
        Attributes->EvtDestroyCallback = &EvtObjectContextDestroyThunk;
    }

    ObjectType
    GetObjectHandle ()
    {
        return m_Object;
    }

protected:

    ObjectContext (
        _In_ ObjectType WdfObject
        ) :
        m_Object(WdfObject)
    {
    }

    // Note that this is not virtual since we are using CRTP and hence know the exact derived
    // type statically.
    ~ObjectContext () = default;

    // Placement-new to construct the object inside the WDF context space.
    static
    void*
    operator new (
        _In_ size_t /* SizeInBytes */,
        _In_ void* ContextMemory
        )
    {
        // We already have the memory courtesy of WDF so we don't have to allocate anything.
        return ContextMemory;
    }

    static
    void
    operator delete (
        _In_ void* /* ContextMemory */
        )
    {
        // Since we didn't allocate the memory, don't try to deallocate it.
    }

private:

    // This method overload will get picked if the derived class has an "EvtObjectContextCleanup"
    // method. If it doesn't SFINAE will cause this overload to get discarded. The return type
    // exists simply to trigger SFINAE; we don't actually return anything useful. The second dummy
    // bool parameter exists simply to give this overload precendence over the next one when called
    // with a bool value.
    template<typename U = ContextType>
    static
    decltype(&U::EvtObjectContextCleanup)
    SetEvtObjectContextCleanup (
        _Inout_ PWDF_OBJECT_ATTRIBUTES Attributes,
        _In_ bool /* OverloadDisambiguator */
        )
    {
        Attributes->EvtCleanupCallback = &EvtObjectContextCleanupThunk;
        return nullptr;
    }

    // This method overload will get picked if the derived class does not have an
    // "EvtObjectContextCleanup" method.
    template<typename U = ContextType>
    static
    void
    SetEvtObjectContextCleanup (
        _Inout_ PWDF_OBJECT_ATTRIBUTES /* Attributes */,
        _In_ int /* OverloadDisambiguator */
        )
    {
    }

    static
    EVT_WDF_OBJECT_CONTEXT_DESTROY
    EvtObjectContextDestroyThunk;

    static
    EVT_WDF_OBJECT_CONTEXT_CLEANUP
    EvtObjectContextCleanupThunk;

    ObjectType m_Object;
};

template<typename ObjectType, typename ContextType>
inline
VOID
ObjectContext<ObjectType, ContextType>::EvtObjectContextDestroyThunk (
    WDFOBJECT Object
    )
{
    // Invoke the destructor by calling delete.
    ContextType* context = ContextType::GetContextFromObject(static_cast<ObjectType>(Object));
    delete context;
}

template<typename ObjectType, typename ContextType>
inline
VOID
ObjectContext<ObjectType, ContextType>::EvtObjectContextCleanupThunk (
    WDFOBJECT Object
    )
{
    ContextType* context = ContextType::GetContextFromObject(static_cast<ObjectType>(Object));
    context->EvtObjectContextCleanup();
}

}
