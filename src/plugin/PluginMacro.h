#pragma once
#include "PluginRegistry.h"
#include "PluginContext.h"

// Concatenation helpers for unique identifiers
#define _PLUGIN_CONCAT_INNER(a, b) a##b
#define _PLUGIN_CONCAT(a, b) _PLUGIN_CONCAT_INNER(a, b)

// REGISTER_PLUGIN("MyPlugin", [](materializr::PluginContext& ctx) { ... });
//
// Creates a static auto-registration object and a force-link symbol.
// The force-link function is called from ForceLink.cpp to prevent
// the linker from stripping the translation unit.
#define REGISTER_PLUGIN(pluginName, initFn) \
    namespace materializr { namespace force_link { \
        void _PLUGIN_CONCAT(forceLink_, pluginName)(); \
    }} \
    void materializr::force_link::_PLUGIN_CONCAT(forceLink_, pluginName)() {} \
    namespace { \
        struct _PLUGIN_CONCAT(_AutoReg_, pluginName) { \
            _PLUGIN_CONCAT(_AutoReg_, pluginName)() { \
                ::materializr::PluginRegistry::instance().add( \
                    {#pluginName, initFn, nullptr}); \
            } \
        } _PLUGIN_CONCAT(_s_autoReg_, pluginName); \
    }
