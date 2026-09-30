#pragma once
#include "../core/Operation.h"
#include "../core/Document.h"
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

class AlignOp : public Operation {
public:
    AlignOp();
    ~AlignOp() override = default;

    void setBodyId(int id);
    void setSourcePoint(const gp_Pnt& pt);
    void setTargetPoint(const gp_Pnt& pt);

    // Getters
    int getBodyId() const { return m_bodyId; }

    // Operation interface
    bool execute(Document& doc) override;
    bool undo(Document& doc) override;
    std::string name() const override { return "Align"; }
    std::string description() const override;
    void renderProperties() override;
    std::string typeId() const override { return "align"; }
    // Reload support (full history replay): every step must come back as a
    // real editable op, never a frozen ReplayOp.
    std::string serializeParams() const override;
    bool deserializeParams(const std::string& blob) override;
    bool rehydrateFromReload(const ReloadState& state, Document& doc) override;
    OperationDiff captureDiff() const override;

private:
    int m_bodyId = -1;
    gp_Pnt m_source;
    gp_Pnt m_target;
    TopoDS_Shape m_previousShape;
};
