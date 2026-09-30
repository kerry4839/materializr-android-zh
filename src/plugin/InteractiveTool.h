#pragma once
#include <glm/glm.hpp>
#include <string>

namespace materializr {

class PluginContext;

struct ToolInputEvent {
    enum Type { MouseDown, MouseUp, MouseMove, MouseDrag, KeyPress, Scroll };
    Type type;
    glm::vec2 mousePos{0};
    glm::vec2 mouseDelta{0};
    int key = 0;
    float scroll = 0.0f;
    bool ctrl = false;
    bool shift = false;
    float viewportWidth = 0;
    float viewportHeight = 0;
};

class InteractiveTool {
public:
    virtual ~InteractiveTool() = default;

    virtual void begin(PluginContext& ctx) = 0;
    virtual bool update(PluginContext& ctx) = 0;
    virtual void commit(PluginContext& ctx) = 0;
    virtual void cancel(PluginContext& ctx) = 0;
    virtual bool handleInput(PluginContext& ctx, const ToolInputEvent& event) = 0;
    virtual void renderOverlay(PluginContext& ctx) = 0;
    virtual void renderViewport(PluginContext& ctx,
                                const glm::mat4& view, const glm::mat4& proj) {}
    virtual std::string name() const = 0;
};

} // namespace materializr
