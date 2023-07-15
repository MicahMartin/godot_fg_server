// Copied from godot-cpp/test/src and modified.

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "FightingGameServer.h"

/// @file
/// Register our classes with Godot.

static FightingGameServer* _singleton;
namespace
{
    /// @brief Called by Godot to let us register our classes with Godot.
    ///
    /// @param p_level the level being initialized by Godot
    ///
    /// @see GDExtensionInit
    void initializeExtension( godot::ModuleInitializationLevel p_level )
    {
        if ( p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE )
        {
            return;
        }

        godot::ClassDB::register_class<FightingGameServer>();
        _singleton = memnew(FightingGameServer);
        godot::Engine::get_singleton()->register_singleton("FightingGameServer", 
            FightingGameServer::getSingleton());

        // Register singleton
    }

    /// @brief Called by Godot to let us do any cleanup.
    ///
    /// @see GDExtensionInit
    void uninitializeExtension( godot::ModuleInitializationLevel p_level )
    {
        if ( p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE )
        {
            return;
        }
        godot::Engine::get_singleton()->unregister_singleton("FightingGameServer");
        memdelete(_singleton); // Segmentation fault with and without this line.
    }
}

extern "C"
{
    /// @brief This is the entry point for the shared library.
    ///
    /// @note The name of this function must match the "entry_symbol" in
    /// templates/template.*.gdextension.in
    ///
    /// @param p_interface the interface (need more info)
    /// @param p_library the library (need more info)
    /// @param r_initialization the intialization (need more info)
    ///
    /// @returns GDExtensionBool
    GDExtensionBool GDE_EXPORT FightingGameServerInit( const GDExtensionInterface *p_interface,
                                                GDExtensionClassLibraryPtr p_library,
                                                GDExtensionInitialization *r_initialization )
    {

        {
            godot::GDExtensionBinding::InitObject init_obj( p_interface, p_library,
                                                            r_initialization );

            init_obj.register_initializer( initializeExtension );
            init_obj.register_terminator( uninitializeExtension );
            init_obj.set_minimum_library_initialization_level(
                godot::MODULE_INITIALIZATION_LEVEL_SCENE );

            return init_obj.init();
        }
    }
}
