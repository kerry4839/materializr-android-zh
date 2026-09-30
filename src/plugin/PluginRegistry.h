#pragma once
#include "Contributions.h"
#include "InteractiveTool.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace materializr {

class PluginContext;

struct PluginDescriptor {
    std::string name;
    std::function<void(PluginContext&)> init;
    std::function<void()> shutdown;
};

class PluginRegistry {
public:
    static PluginRegistry& instance();

    void add(PluginDescriptor desc);
    void initAll(PluginContext& ctx);
    void shutdownAll();

    std::vector<ToolbarContribution>& toolbarContributions();
    std::vector<CommandContribution>& commandContributions();
    std::vector<MenuContribution>& menuContributions();
    std::vector<IOFormatContribution>& ioFormats();
    std::vector<RenderPassContribution>& renderPasses();
    std::vector<PropertyContribution>& propertyContributions();
    std::vector<OverlayContribution>& overlayContributions();

    void activateTool(std::unique_ptr<InteractiveTool> tool, PluginContext& ctx);
    InteractiveTool* activeTool();
    void deactivateTool(PluginContext& ctx); // cancels the tool, then clears it
    // Clears the active tool WITHOUT calling cancel(). Used when a tool reports it
    // has finished (update() returned false): it already ran its own commit() or
    // cancel(), so cancelling again here would undo a just-committed operation.
    void finishActiveTool();

private:
    PluginRegistry() = default;

    std::vector<PluginDescriptor> m_plugins;
    std::vector<ToolbarContribution> m_toolbar;
    std::vector<CommandContribution> m_commands;
    std::vector<MenuContribution> m_menus;
    std::vector<IOFormatContribution> m_ioFormats;
    std::vector<RenderPassContribution> m_renderPasses;
    std::vector<PropertyContribution> m_properties;
    std::vector<OverlayContribution> m_overlays;
    std::unique_ptr<InteractiveTool> m_activeTool;
};

} // namespace materializr
