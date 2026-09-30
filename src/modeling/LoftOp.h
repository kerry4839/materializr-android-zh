#pragma once
#include "../core/Operation.h"
#include "../core/Document.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <vector>
#include <string>

class LoftOp : public Operation {
public:
    LoftOp();
    ~LoftOp() override = default;

    // Parameters
    void addProfile(const TopoDS_Wire& wire);
    // Profile with holes: `outer` is the outer boundary, `holes` the inner
    // boundary wires (e.g. the inner circle of a concentric pair). The holes
    // are lofted into their own inner solids and cut from the outer loft, so a
    // ring-section profile produces a tube instead of a solid cylinder.
    void addProfile(const TopoDS_Wire& outer, const std::vector<TopoDS_Wire>& holes);
    void clearProfiles();
    void setSolid(bool solid);   // true = solid, false = shell
    void setRuled(bool ruled);   // true = ruled surface, false = smooth
    // BRIDGE MODE: every section came from a face of ONE body. Instead of
    // adding the loft as a new body (which then cannot be unioned -- the fuse
    // of skin-tight tangent contact defeats OCCT at every setting), the loft
    // CONSUMES those faces: they are removed from the body, the loft walls are
    // sewn into the holes, and the body is replaced by the single merged
    // solid. No boolean runs at any point.
    void setBridge(int bodyId, const std::vector<TopoDS_Shape>& sourceFaces);

    // Getters
    bool isSolid() const { return m_solid; }
    bool isRuled() const { return m_ruled; }
    int profileCount() const { return static_cast<int>(m_profiles.size()); }

    // Operation interface
    bool execute(Document& doc) override;
    bool undo(Document& doc) override;
    std::string name() const override { return "Loft"; }
    std::string description() const override;
    void renderProperties() override;
    std::string typeId() const override { return "loft"; }
    // Reload support (full history replay): every step must come back as a
    // real editable op, never a frozen ReplayOp.
    std::string serializeParams() const override;
    bool deserializeParams(const std::string& blob) override;
    bool rehydrateFromReload(const ReloadState& state, Document& doc) override;
    OperationDiff captureDiff() const override;

private:
    std::vector<TopoDS_Wire> m_profiles;
    // Parallel to m_profiles: the hole wires for each profile (empty if none).
    // m_holeProfiles[i] are the holes of m_profiles[i].
    std::vector<std::vector<TopoDS_Wire>> m_holeProfiles;
    bool m_solid = true;
    bool m_ruled = false;
    // Bridge mode (see setBridge). previousShape backs undo.
    int m_bridgeBodyId = -1;
    std::vector<TopoDS_Shape> m_bridgeFaces;
    TopoDS_Shape m_bridgePreviousShape;
    bool m_bridged = false;   // this execute() replaced the body (vs added one)

    // For undo
    int m_createdBodyId = -1;
};
