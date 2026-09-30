#include "MirrorOp.h"
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <imgui.h>
#include "../i18n.h"
#include "../i18n.h"

MirrorOp::MirrorOp() = default;

void MirrorOp::setBody(int id) {
    m_bodyId = id;
}

void MirrorOp::setPlane(MirrorPlane p) {
    m_plane = p;
}

void MirrorOp::setCustomPlane(const gp_Ax2& ax) {
    m_customPlane = ax;
}

void MirrorOp::setKeepOriginal(bool keep) {
    m_keepOriginal = keep;
}

gp_Ax2 MirrorOp::getMirrorAxis() const {
    gp_Pnt origin(0, 0, 0);
    switch (m_plane) {
        case MirrorPlane::XY:
            return gp_Ax2(origin, gp_Dir(0, 0, 1)); // normal along Z
        case MirrorPlane::XZ:
            return gp_Ax2(origin, gp_Dir(0, 1, 0)); // normal along Y
        case MirrorPlane::YZ:
            return gp_Ax2(origin, gp_Dir(1, 0, 0)); // normal along X
        case MirrorPlane::Custom:
            return m_customPlane;
    }
    return gp_Ax2(origin, gp_Dir(1, 0, 0));
}

bool MirrorOp::execute(Document& doc) {
    if (m_bodyId < 0) {
        return false;
    }

    try {
        // Store previous shape for undo
        m_previousShape = doc.getBody(m_bodyId);

        // Create mirror transformation
        gp_Trsf trsf;
        trsf.SetMirror(getMirrorAxis());

        BRepBuilderAPI_Transform transform(m_previousShape, trsf, true);
        transform.Build();
        if (!transform.IsDone()) {
            return false;
        }

        TopoDS_Shape mirroredShape = transform.Shape();

        if (m_keepOriginal) {
            // Reuse prior id on redo to preserve folder/colour/etc. via the
            // Document tombstone restore.
            doc.addOrPutBody(m_mirroredBodyId, mirroredShape, "Mirror");
        } else {
            // Replace the original body with the mirrored version
            doc.updateBody(m_bodyId, mirroredShape);
            m_mirroredBodyId = -1;
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool MirrorOp::undo(Document& doc) {
    try {
        if (m_keepOriginal) {
            // Remove the mirrored body
            if (m_mirroredBodyId >= 0) {
                doc.removeBody(m_mirroredBodyId);
                // Keep m_mirroredBodyId so redo via addOrPutBody picks up the
                // tombstone-stashed folder/colour/etc.
            }
        } else {
            // Restore the original shape
            if (m_bodyId >= 0 && !m_previousShape.IsNull()) {
                doc.updateBody(m_bodyId, m_previousShape);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::string MirrorOp::description() const {
    const char* planeStr = "Custom";
    switch (m_plane) {
        case MirrorPlane::XY: planeStr = "XY"; break;
        case MirrorPlane::XZ: planeStr = "XZ"; break;
        case MirrorPlane::YZ: planeStr = "YZ"; break;
        case MirrorPlane::Custom: planeStr = materializr::tr("Custom"); break;
    }
    return materializr::trFormat("Mirror across %s%s", materializr::tr(planeStr),
                                 m_keepOriginal
                                     ? materializr::tr(" (keep original)") : "");
}

void MirrorOp::renderProperties() {
    ImGui::Text("%s", materializr::tr("Mirror"));
    ImGui::Separator();

    const char* planeItems[] = { "XY", "XZ", "YZ", "Custom" };
    int planeIndex = static_cast<int>(m_plane);
    if (ImGui::Combo(materializr::tr("Mirror Plane"), &planeIndex, planeItems, 4)) {
        m_plane = static_cast<MirrorPlane>(planeIndex);
    }

    ImGui::Checkbox(materializr::tr("Keep Original"), &m_keepOriginal);

    ImGui::Text(materializr::tr("Body ID: %d"), m_bodyId);

    if (m_mirroredBodyId >= 0) {
        ImGui::Text(materializr::tr("Mirrored body ID: %d"), m_mirroredBodyId);
    }
}

OperationDiff MirrorOp::captureDiff() const {
    OperationDiff d;
    if (m_keepOriginal) {
        if (m_mirroredBodyId >= 0) d.created.push_back(m_mirroredBodyId);
    } else if (m_bodyId >= 0 && !m_previousShape.IsNull()) {
        d.modifiedBefore.push_back({m_bodyId, m_previousShape});
    }
    return d;
}

std::string MirrorOp::serializeParams() const {
    const gp_Ax3 a(m_customPlane);
    char buf[380];
    std::snprintf(buf, sizeof(buf),
                  "body=%d;mirrored=%d;keep=%d;"
                  "ox=%.9g;oy=%.9g;oz=%.9g;nx=%.9g;ny=%.9g;nz=%.9g;"
                  "xx=%.9g;xy=%.9g;xz=%.9g",
                  m_bodyId, m_mirroredBodyId, m_keepOriginal ? 1 : 0,
                  a.Location().X(), a.Location().Y(), a.Location().Z(),
                  a.Direction().X(), a.Direction().Y(), a.Direction().Z(),
                  a.XDirection().X(), a.XDirection().Y(), a.XDirection().Z());
    return buf;
}

bool MirrorOp::deserializeParams(const std::string& blob) {
    double v[9] = {0, 0, 0, 0, 0, 1, 1, 0, 0};
    bool any = false;
    size_t pos = 0;
    while (pos < blob.size()) {
        size_t eq = blob.find('=', pos);
        if (eq == std::string::npos) break;
        size_t end = blob.find(';', eq);
        if (end == std::string::npos) end = blob.size();
        std::string key = blob.substr(pos, eq - pos);
        double d = std::atof(blob.substr(eq + 1, end - eq - 1).c_str());
        if      (key == "body")     { m_bodyId = static_cast<int>(d); any = true; }
        else if (key == "mirrored") { m_mirroredBodyId = static_cast<int>(d); any = true; }
        else if (key == "keep")     { m_keepOriginal = d != 0; any = true; }
        else if (key == "ox") v[0] = d; else if (key == "oy") v[1] = d;
        else if (key == "oz") v[2] = d; else if (key == "nx") v[3] = d;
        else if (key == "ny") v[4] = d; else if (key == "nz") v[5] = d;
        else if (key == "xx") v[6] = d; else if (key == "xy") v[7] = d;
        else if (key == "xz") v[8] = d;
        pos = end + 1;
    }
    try {
        m_customPlane = gp_Ax2(gp_Pnt(v[0], v[1], v[2]),
                               gp_Dir(v[3], v[4], v[5]),
                               gp_Dir(v[6], v[7], v[8]));
    } catch (...) {}
    return any;
}

bool MirrorOp::rehydrateFromReload(const ReloadState& state, Document&) {
    if (m_bodyId < 0) return false;
    m_previousShape.Nullify();
    for (const auto& [id, shp] : state.modifiedBefore)
        if (id == m_bodyId) { m_previousShape = shp; break; }
    if (m_keepOriginal) {
        if (m_mirroredBodyId < 0 && !state.created.empty())
            m_mirroredBodyId = state.created.front();
        return true;   // source body is live; execute() re-mirrors it
    }
    return !m_previousShape.IsNull();   // in-place mirror needs the before shape
}
