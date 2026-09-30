#include "SeparateBodyOp.h"

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "../i18n.h"

int SeparateBodyOp::solidCount(const TopoDS_Shape& shape) {
    if (shape.IsNull()) return 0;
    int n = 0;
    for (TopExp_Explorer sx(shape, TopAbs_SOLID); sx.More(); sx.Next()) ++n;
    return n;
}

namespace {
double solidVolume(const TopoDS_Shape& s) {
    GProp_GProps g;
    BRepGProp::VolumeProperties(s, g);
    return g.Mass();
}
} // namespace

bool SeparateBodyOp::execute(Document& doc) {
    if (m_bodyId < 0) return false;
    try {
        m_previousShape = doc.getBody(m_bodyId);
        if (m_previousShape.IsNull()) return false;

        std::vector<TopoDS_Shape> solids;
        for (TopExp_Explorer sx(m_previousShape, TopAbs_SOLID); sx.More();
             sx.Next())
            solids.push_back(sx.Current());
        if (solids.size() < 2) return false; // nothing to separate

        // Largest lump keeps the original body; the rest split out. Sorting by
        // volume makes the choice deterministic across reload/redo.
        std::sort(solids.begin(), solids.end(),
                  [](const TopoDS_Shape& a, const TopoDS_Shape& b) {
                      return solidVolume(a) > solidVolume(b);
                  });

        const std::string base = doc.getBodyName(m_bodyId);
        doc.updateBody(m_bodyId, solids[0]);

        // One new body per remaining lump. Reuse stored ids on redo (pass the
        // old id to addOrPutBody so it restores the tombstoned body); allocate
        // fresh (-1) on the first run.
        for (size_t i = 1; i < solids.size(); ++i) {
            int id = (i - 1 < m_newBodyIds.size()) ? m_newBodyIds[i - 1] : -1;
            const std::string name =
                (base.empty() ? std::string("Body") : base) + " (" +
                std::to_string(i + 1) + ")";
            doc.addOrPutBody(id, solids[i], name);
            if (i - 1 < m_newBodyIds.size())
                m_newBodyIds[i - 1] = id;
            else
                m_newBodyIds.push_back(id);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool SeparateBodyOp::undo(Document& doc) {
    try {
        for (int id : m_newBodyIds)
            if (id >= 0) doc.removeBody(id);
        // Keep m_newBodyIds so redo reuses the same ids via addOrPutBody.
        if (m_bodyId >= 0 && !m_previousShape.IsNull())
            doc.updateBody(m_bodyId, m_previousShape);
        return true;
    } catch (...) {
        return false;
    }
}

std::string SeparateBodyOp::description() const {
    return materializr::trFormat("Separate body %d into %d bodies", m_bodyId,
                                 static_cast<int>(m_newBodyIds.size() + 1));
}

void SeparateBodyOp::renderProperties() {
    ImGui::Text("%s", materializr::tr("Separate"));
    ImGui::Separator();
    ImGui::Text(materializr::tr("Source body: %d"), m_bodyId);
    ImGui::Text(materializr::tr("Split into: %d bodies"),
                static_cast<int>(m_newBodyIds.size() + 1));
    if (!m_newBodyIds.empty()) {
        ImGui::Text("%s", materializr::tr("New bodies:"));
        for (int id : m_newBodyIds) ImGui::BulletText(materializr::tr("Body %d"), id);
    }
}

OperationDiff SeparateBodyOp::captureDiff() const {
    OperationDiff d;
    if (m_bodyId >= 0 && !m_previousShape.IsNull())
        d.modifiedBefore.push_back({m_bodyId, m_previousShape});
    for (int id : m_newBodyIds)
        if (id >= 0) d.created.push_back(id);
    return d;
}

std::string SeparateBodyOp::serializeParams() const {
    std::string blob = "body=" + std::to_string(m_bodyId) + ";new=";
    for (size_t i = 0; i < m_newBodyIds.size(); ++i) {
        if (i) blob += ",";
        blob += std::to_string(m_newBodyIds[i]);
    }
    return blob;
}

bool SeparateBodyOp::deserializeParams(const std::string& blob) {
    m_newBodyIds.clear();
    bool any = false;
    size_t pos = 0;
    while (pos < blob.size()) {
        size_t eq = blob.find('=', pos);
        if (eq == std::string::npos) break;
        size_t end = blob.find(';', eq);
        if (end == std::string::npos) end = blob.size();
        std::string key = blob.substr(pos, eq - pos);
        std::string val = blob.substr(eq + 1, end - eq - 1);
        if (key == "body") {
            m_bodyId = std::atoi(val.c_str());
            any = true;
        } else if (key == "new") {
            size_t p = 0;
            while (p < val.size()) {
                size_t comma = val.find(',', p);
                if (comma == std::string::npos) comma = val.size();
                std::string tok = val.substr(p, comma - p);
                if (!tok.empty()) m_newBodyIds.push_back(std::atoi(tok.c_str()));
                p = comma + 1;
            }
        }
        pos = end + 1;
    }
    return any;
}

bool SeparateBodyOp::rehydrateFromReload(const ReloadState& state, Document&) {
    if (m_bodyId < 0) return false;
    m_previousShape.Nullify();
    for (const auto& [id, shp] : state.modifiedBefore)
        if (id == m_bodyId) { m_previousShape = shp; break; }
    if (m_newBodyIds.empty())
        for (int id : state.created) m_newBodyIds.push_back(id);
    return !m_previousShape.IsNull();
}
