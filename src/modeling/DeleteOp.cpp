#include "DeleteOp.h"
#include <cstdio>
#include <cstdlib>
#include <imgui.h>
#include "../i18n.h"

DeleteOp::DeleteOp() = default;

void DeleteOp::setBodyId(int id) {
    m_bodyId = id;
}

bool DeleteOp::execute(Document& doc) {
    if (m_bodyId < 0) {
        return false;
    }

    try {
        // Save shape, name, and visibility for undo
        m_deletedShape = doc.getBody(m_bodyId);
        m_deletedName = doc.getBodyName(m_bodyId);
        m_wasVisible = doc.isBodyVisible(m_bodyId);

        // Remove the body from the document
        doc.removeBody(m_bodyId);

        return true;
    } catch (...) {
        return false;
    }
}

bool DeleteOp::undo(Document& doc) {
    if (m_deletedShape.IsNull()) {
        return false;
    }

    try {
        // Restore the body under its ORIGINAL id (not a fresh addBody id) so an
        // editStep replay can roll this delete back and any step that references
        // the body by id still resolves it. putBody also pulls folder/colour
        // metadata back from the tombstone.
        doc.putBody(m_bodyId, m_deletedShape, m_deletedName);
        doc.setBodyVisible(m_bodyId, m_wasVisible);
        return true;
    } catch (...) {
        return false;
    }
}

std::string DeleteOp::description() const {
    if (m_deletedName.empty()) {
        return materializr::trFormat("Delete body %d", m_bodyId);
    }
    return materializr::trFormat("Delete \"%s\"", m_deletedName.c_str());
}

void DeleteOp::renderProperties() {
    ImGui::Text("%s", materializr::tr("Delete"));
    ImGui::Separator();

    ImGui::Text(materializr::tr("Body ID: %d"), m_bodyId);

    if (!m_deletedName.empty()) {
        ImGui::Text(materializr::tr("Name: %s"), m_deletedName.c_str());
    }
}

OperationDiff DeleteOp::captureDiff() const {
    OperationDiff d;
    if (m_bodyId >= 0 && !m_deletedShape.IsNull())
        d.deletedBefore.push_back({m_bodyId, m_deletedShape});
    return d;
}

std::string DeleteOp::serializeParams() const {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "body=%d", m_bodyId);
    return buf;
}

bool DeleteOp::deserializeParams(const std::string& blob) {
    bool any = false;
    size_t pos = 0;
    while (pos < blob.size()) {
        size_t eq = blob.find('=', pos);
        if (eq == std::string::npos) break;
        size_t end = blob.find(';', eq);
        if (end == std::string::npos) end = blob.size();
        std::string key = blob.substr(pos, eq - pos);
        std::string val = blob.substr(eq + 1, end - eq - 1);
        if (key == "body") { m_bodyId = std::atoi(val.c_str()); any = true; }
        pos = end + 1;
    }
    return any;
}

bool DeleteOp::rehydrateFromReload(const ReloadState& state, Document& /*doc*/) {
    if (m_bodyId < 0) return false;
    // Recover the deleted body's shape from the saved step diff so undo() can
    // put it back (under its original id) during an editStep replay.
    m_deletedShape.Nullify();
    for (const auto& [id, shp] : state.deletedBefore)
        if (id == m_bodyId) { m_deletedShape = shp; break; }
    return !m_deletedShape.IsNull();
}
