#include "../plugin/PluginMacro.h"
#include "../i18n.h"
#include "../plugin/PluginContext.h"
#include "../plugin/InteractiveTool.h"
#include "../plugin/PluginRegistry.h"
#include "../core/Document.h"
#include "../core/History.h"
#include "../core/SelectionManager.h"
#include "../modeling/TransformOp.h"
#include "../viewport/Camera.h"
#include <TopoDS_Shape.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <glm/glm.hpp>
#include <cstdio>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

using namespace materializr;

class GizmoDragTool : public InteractiveTool {
public:
    explicit GizmoDragTool(int bodyId, const TopoDS_Shape& originalShape, glm::vec3 delta)
        : m_bodyId(bodyId), m_originalShape(originalShape), m_totalDelta(delta) {}

    void begin(PluginContext&) override {}
    bool update(PluginContext&) override { return !m_done; }

    void commit(PluginContext& ctx) override {
        try {
            TopoDS_Shape finalShape = ctx.document().getBody(m_bodyId);
            ctx.document().updateBody(m_bodyId, m_originalShape);

            Bnd_Box origBox, finalBox;
            BRepBndLib::Add(m_originalShape, origBox);
            BRepBndLib::Add(finalShape, finalBox);
            double ox1,oy1,oz1,ox2,oy2,oz2, fx1,fy1,fz1,fx2,fy2,fz2;
            origBox.Get(ox1,oy1,oz1,ox2,oy2,oz2);
            finalBox.Get(fx1,fy1,fz1,fx2,fy2,fz2);
            double dx = ((fx1+fx2)-(ox1+ox2))/2;
            double dy = ((fy1+fy2)-(oy1+oy2))/2;
            double dz = ((fz1+fz2)-(oz1+oz2))/2;

            auto op = std::make_unique<TransformOp>();
            op->setBodyId(m_bodyId);
            op->setType(TransformType::Translate);
            op->setTranslation(dx, dy, dz);
            ctx.history().pushOperation(std::move(op), ctx.document());
            ctx.markMeshesDirty();
        } catch (...) {}
        m_done = true;
    }

    void cancel(PluginContext& ctx) override {
        if (m_bodyId >= 0 && !m_originalShape.IsNull()) {
            try {
                ctx.document().updateBody(m_bodyId, m_originalShape);
                ctx.markMeshesDirty();
            } catch (...) {}
        }
        m_done = true;
    }

    bool handleInput(PluginContext& ctx, const ToolInputEvent& event) override {
        if (event.type == ToolInputEvent::KeyPress && event.key == 526) {
            cancel(ctx);
            return true;
        }
        return false;
    }

    void renderOverlay(PluginContext&) override {}
    std::string name() const override { return materializr::tr("Gizmo Drag"); }

private:
    int m_bodyId = -1;
    TopoDS_Shape m_originalShape;
    glm::vec3 m_totalDelta{0};
    bool m_done = false;
};

} // anonymous namespace

REGISTER_PLUGIN(GizmoDrag, [](PluginContext& ctx) {
    ctx.registerCommand({"Move (Gizmo)", "W", [](PluginContext&) {
        // Gizmo mode switching is handled by Application's existing gizmo code.
        // This plugin provides the commit-to-history logic when integrated.
    }});
})
