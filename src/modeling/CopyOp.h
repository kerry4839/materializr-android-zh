#pragma once
#include "../core/Operation.h"
#include "../core/Document.h"
#include <TopoDS_Shape.hxx>

class CopyOp : public Operation {
public:
    CopyOp();
    ~CopyOp() override = default;

    void setSourceBodyId(int id);
    void setOffset(double dx, double dy, double dz);

    // Getters
    int getSourceBodyId() const { return m_sourceBodyId; }
    int getCreatedBodyId() const { return m_createdBodyId; }

    // Operation interface
    bool execute(Document& doc) override;
    bool undo(Document& doc) override;
    std::string name() const override { return "Duplicate"; }
    std::string description() const override;
    void renderProperties() override;
    std::string typeId() const override { return "copy"; }
    // Reload support (full history replay): every step must come back as a
    // real editable op, never a frozen ReplayOp.
    std::string serializeParams() const override;
    bool deserializeParams(const std::string& blob) override;
    bool rehydrateFromReload(const ReloadState& state, Document& doc) override;
    OperationDiff captureDiff() const override;

private:
    int m_sourceBodyId = -1;
    double m_dx = 2.0, m_dy = 0.0, m_dz = 0.0;
    int m_createdBodyId = -1;
};
