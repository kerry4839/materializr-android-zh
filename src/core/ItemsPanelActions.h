#pragma once

// Mutation logic behind ItemsPanel's body/folder actions, pulled out of the
// ImGui render code so it can be exercised by a unit test as well as by the
// real panel - see tests/test_body_changes.cpp's PanelDirtyContract fixture.

#include "Document.h"
#include "History.h"
#include "BodyChanges.h"
#include "../modeling/SeparateBodyOp.h"

#include <functional>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace materializr {

inline void setBodyVisibleAndMark(Document& doc, int bodyId, bool visible,
                                   const std::function<void(int)>& markBodyDirty) {
    doc.setBodyVisible(bodyId, visible);
    if (markBodyDirty) markBodyDirty(bodyId);
}

// Cascades to every member whose visibility actually changes - a folder can
// hold bodies already at the target state, and those must NOT be marked.
inline void setFolderVisibleAndMark(Document& doc, int folderId, bool visible,
                                     const std::function<void(int)>& markBodyDirty) {
    std::vector<int> changed;
    for (int id : doc.getBodiesInFolder(folderId))
        if (doc.isBodyVisible(id) != visible) changed.push_back(id);
    doc.setFolderVisible(folderId, visible);
    for (int id : changed)
        if (markBodyDirty) markBodyDirty(id);
}

inline void setBodyColorAndMark(Document& doc, int bodyId, const glm::vec3& color,
                                 const std::function<void(int)>& markBodyDirty) {
    doc.setBodyColor(bodyId, color);
    if (markBodyDirty) markBodyDirty(bodyId);
}

inline void setFolderColorAndMark(Document& doc, int folderId, const glm::vec3& color,
                                   const std::function<void(int)>& markBodyDirty) {
    std::vector<int> changed;
    for (int id : doc.getBodiesInFolder(folderId))
        if (doc.getBodyColor(id) != color) changed.push_back(id);
    doc.setFolderColor(folderId, color);
    for (int id : changed)
        if (markBodyDirty) markBodyDirty(id);
}

// Hides every other body, cascading only the ones that actually flip.
inline void isolateBody(Document& doc, int bodyId,
                         const std::function<void(int)>& markBodyDirty) {
    for (int otherId : doc.getAllBodyIds()) {
        bool target = (otherId == bodyId);
        if (doc.isBodyVisible(otherId) != target) {
            doc.setBodyVisible(otherId, target);
            if (markBodyDirty) markBodyDirty(otherId);
        }
    }
}

inline void showAllBodies(Document& doc, const std::function<void(int)>& markBodyDirty) {
    for (int otherId : doc.getAllBodyIds()) {
        if (!doc.isBodyVisible(otherId)) {
            doc.setBodyVisible(otherId, true);
            if (markBodyDirty) markBodyDirty(otherId);
        }
    }
}

// Splits a body's disconnected solids into separate bodies. The resized
// original plus every split-off body need marking - an unknown-ahead-of-time
// set, so this wraps the push in a BodyChangeScope rather than marking a
// fixed id. Returns pushOperation's result.
inline bool separateBody(Document& doc, History& history, int bodyId,
                          const std::function<void(int)>& markBodyDirty) {
    auto op = std::make_unique<SeparateBodyOp>();
    op->setBody(bodyId);
    BodyChangeScope scope(doc, markBodyDirty);
    return history.pushOperation(std::move(op), doc);
}

} // namespace materializr
