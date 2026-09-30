#pragma once
#include "../core/Operation.h"
#include "../core/Document.h"
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <string>

// Edit a closed cylindrical-or-conical region on any body by setting one or
// both of its END radii (the radii at the two circular edges). Internally
// this is always a "swap" boolean: build a CONE solid representing the OLD
// volume (the body's current cylinder/cone region), build a CONE solid for
// the NEW desired volume, then either fuse-old+cut-new (hole case) or
// cut-old+fuse-new (solid boundary case). When both end radii are equal the
// cone primitive degenerates to a cylinder, so the same op handles both
// re-diametering and creating funnels / partial cones from a previously
// straight feature. The axis is anchored at the V_min end of the affected
// face, so R1 corresponds to the BOTTOM (V_min) edge and R2 to the TOP
// (V_max) edge.
class ResizeCylindricalOp : public Operation {
public:
    void setBody(int bodyId);
    void setAxis(const gp_Ax2& axis);
    void setHeight(double h);
    void setOldRadii(double bottomR, double topR);
    void setNewRadii(double bottomR, double topR);
    void setIsHole(bool h);

    int    getBodyId() const { return m_bodyId; }
    bool   isHole()    const { return m_isHole; }
    double getOldBottom() const { return m_oldBottomR; }
    double getOldTop()    const { return m_oldTopR; }
    double getNewBottom() const { return m_newBottomR; }
    double getNewTop()    const { return m_newTopR; }

    bool execute(Document& doc) override;
    bool undo(Document& doc) override;
    std::string name() const override { return "Resize"; }
    std::string description() const override;
    void renderProperties() override;
    std::string typeId() const override { return "resize_cylindrical"; }
    OperationDiff captureDiff() const override;
    std::vector<int> plannedBodyIds() const override { return {m_bodyId}; }
    std::string serializeParams() const override;
    bool deserializeParams(const std::string& blob) override;
    bool rehydrateFromReload(const ReloadState& state, Document& doc) override;

private:
    int      m_bodyId = -1;
    gp_Ax2   m_axis;
    double   m_height      = 0.0;
    double   m_oldBottomR  = 0.0;
    double   m_oldTopR     = 0.0;
    double   m_newBottomR  = 0.0;
    double   m_newTopR     = 0.0;
    bool     m_isHole      = true;
    TopoDS_Shape m_previousShape; // for undo
};
