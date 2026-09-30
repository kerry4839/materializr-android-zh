#include "AxisTransformOp.h"
#include "../core/Document.h"
#include <imgui.h>
#include <cstdio>
#include <cstdlib>
#include "../i18n.h"
#include "../i18n.h"

bool AxisTransformOp::execute(Document& doc) {
    for (const auto& e : m_entries) {
        doc.setAxis(e.axisId, e.afterOrigin, e.afterDir);
    }
    return true;
}

bool AxisTransformOp::undo(Document& doc) {
    for (const auto& e : m_entries) {
        doc.setAxis(e.axisId, e.beforeOrigin, e.beforeDir);
    }
    return true;
}

std::string AxisTransformOp::description() const {
    if (m_entries.size() == 1) {
        return m_label + materializr::trFormat(" (axis %d)",
                                               m_entries.front().axisId);
    }
    return m_label + materializr::trFormat(" (%d axes)",
                                           static_cast<int>(m_entries.size()));
}

void AxisTransformOp::renderProperties() {
    ImGui::TextUnformatted(m_label.c_str());
    ImGui::Text(materializr::tr("Axes affected: %d"), static_cast<int>(m_entries.size()));
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", materializr::tr("Construction-axis transform (undo/redo only)."));
}

std::string AxisTransformOp::serializeParams() const {
    std::string blob = "n=" + std::to_string(m_entries.size());
    char buf[320];
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const Entry& e = m_entries[i];
        std::snprintf(buf, sizeof(buf),
                      ";e%zu=%d %.9g %.9g %.9g %.9g %.9g %.9g "
                      "%.9g %.9g %.9g %.9g %.9g %.9g",
                      i, e.axisId,
                      e.beforeOrigin.X(), e.beforeOrigin.Y(), e.beforeOrigin.Z(),
                      e.beforeDir.X(), e.beforeDir.Y(), e.beforeDir.Z(),
                      e.afterOrigin.X(), e.afterOrigin.Y(), e.afterOrigin.Z(),
                      e.afterDir.X(), e.afterDir.Y(), e.afterDir.Z());
        blob += buf;
    }
    blob += ";label=" + m_label;   // last: read to end-of-string
    return blob;
}

bool AxisTransformOp::deserializeParams(const std::string& blob) {
    m_entries.clear();
    size_t pos = 0;
    bool any = false;
    while (pos < blob.size()) {
        size_t eq = blob.find('=', pos);
        if (eq == std::string::npos) break;
        std::string key = blob.substr(pos, eq - pos);
        if (key == "label") {
            m_label = blob.substr(eq + 1);
            any = true;
            break;
        }
        size_t end = blob.find(';', eq);
        if (end == std::string::npos) end = blob.size();
        std::string val = blob.substr(eq + 1, end - eq - 1);
        if (!key.empty() && key[0] == 'e') {
            double d[13];
            int got = 0;
            const char* c = val.c_str();
            char* ce = nullptr;
            for (; got < 13; ++got) {
                d[got] = std::strtod(c, &ce);
                if (ce == c) break;
                c = ce;
            }
            if (got == 13) {
                Entry e;
                e.axisId       = static_cast<int>(d[0]);
                e.beforeOrigin = gp_Pnt(d[1], d[2], d[3]);
                e.beforeDir    = gp_Dir(d[4], d[5], d[6]);
                e.afterOrigin  = gp_Pnt(d[7], d[8], d[9]);
                e.afterDir     = gp_Dir(d[10], d[11], d[12]);
                m_entries.push_back(e);
                any = true;
            }
        } else if (key == "n") {
            any = true;
        }
        pos = end + 1;
    }
    if (m_label.empty()) m_label = "Move Axis";
    return any && !m_entries.empty();
}

bool AxisTransformOp::rehydrateFromReload(const ReloadState&, Document&) {
    return !m_entries.empty();
}
