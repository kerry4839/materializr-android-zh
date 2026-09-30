#pragma once
#include "../core/Operation.h"
#include "../core/Document.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <string>

class SweepOp : public Operation {
public:
    SweepOp();
    ~SweepOp() override = default;

    // Parameters
    void setProfile(const TopoDS_Shape& profile); // face or wire
    void setPath(const TopoDS_Wire& path);

    // Operation interface
    bool execute(Document& doc) override;
    bool undo(Document& doc) override;
    std::string name() const override { return "Sweep"; }
    std::string description() const override;
    void renderProperties() override;
    std::string typeId() const override { return "sweep"; }
    // Reload support (full history replay): every step must come back as a
    // real editable op, never a frozen ReplayOp.
    std::string serializeParams() const override;
    bool deserializeParams(const std::string& blob) override;
    bool rehydrateFromReload(const ReloadState& state, Document& doc) override;
    OperationDiff captureDiff() const override;

private:
    TopoDS_Shape m_profile;
    TopoDS_Wire m_path;

    // For undo
    int m_createdBodyId = -1;
};
