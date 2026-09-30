// changedBodies() names exactly the bodies an edit touched, so a preview
// re-tessellates those and not the rest of the document.
#include "core/BodyChanges.h"
#include "core/Document.h"
#include "core/EventBus.h"
#include "core/Events.h"
#include "core/History.h"
#include "core/HistoryPanelActions.h"
#include "core/ItemsPanelActions.h"
#include "modeling/DeleteOp.h"
#include "modeling/ExtrudeOp.h"
#include "modeling/SeparateBodyOp.h"
#include "modeling/SketchEditOp.h"

#include <gtest/gtest.h>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <fstream>
#include <sstream>
#include <set>
#include <vector>

using materializr::BodyChangeScope;
using materializr::BodySnapshot;
using materializr::changedBodies;
using materializr::snapshotBodies;
using materializr::applyStepEdit;
using materializr::deleteStep;
using materializr::isolateBody;
using materializr::redoStep;
using materializr::separateBody;
using materializr::setBodyColorAndMark;
using materializr::setBodyVisibleAndMark;
using materializr::setFolderColorAndMark;
using materializr::setFolderVisibleAndMark;
using materializr::showAllBodies;
using materializr::toggleStepEnabled;
using materializr::undoStep;

namespace {

TopoDS_Shape box(double s) { return BRepPrimAPI_MakeBox(s, s, s).Shape(); }

// Two air-gapped boxes fused into one body's shape, mimicking the
// boolean-remnant case Separate exists for (see tests/test_separate_body.cpp).
TopoDS_Shape twoLumpBody() {
    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);
    builder.Add(comp, BRepPrimAPI_MakeBox(gp_Pnt(0, 0, 0), 10, 10, 10).Shape());
    builder.Add(comp, BRepPrimAPI_MakeBox(gp_Pnt(50, 0, 0), 2, 2, 2).Shape());
    return comp;
}

struct Doc {
    Document doc;
    int a, b, c;
    Doc() : a(doc.addBody(box(10.0), "a")), b(doc.addBody(box(20.0), "b")), c(doc.addBody(box(30.0), "c")) {}
};

} // namespace

TEST(BodyChanges, NothingChangedNamesNothing) {
    Doc d;
    const BodySnapshot before = snapshotBodies(d.doc);
    EXPECT_TRUE(changedBodies(before, d.doc).empty());
}

TEST(BodyChanges, AReplacedShapeNamesThatBodyOnly) {
    Doc d;
    const BodySnapshot before = snapshotBodies(d.doc);
    d.doc.updateBody(d.b, box(25.0));
    EXPECT_EQ(changedBodies(before, d.doc), std::vector<int>{d.b});
}

TEST(BodyChanges, AMovedBodyCounts) {
    // Same TShape, new Location: the renderer bakes the location into the
    // vertices, so a move is a change. A TShape-pointer compare would miss it.
    Doc d;
    const BodySnapshot before = snapshotBodies(d.doc);
    gp_Trsf t;
    t.SetTranslation(gp_Vec(5.0, 0.0, 0.0));
    d.doc.updateBody(d.a, d.doc.getBody(d.a).Moved(TopLoc_Location(t)));
    EXPECT_EQ(changedBodies(before, d.doc), std::vector<int>{d.a});
}

TEST(BodyChanges, HidingCounts) {
    // The partial rebuild removes a hidden body only when it is marked.
    Doc d;
    const BodySnapshot before = snapshotBodies(d.doc);
    d.doc.setBodyVisible(d.c, false);
    EXPECT_EQ(changedBodies(before, d.doc), std::vector<int>{d.c});
}

TEST(BodyChanges, RemovedAndAddedBodiesCount) {
    Doc d;
    const BodySnapshot before = snapshotBodies(d.doc);
    d.doc.removeBody(d.a);
    const int n = d.doc.addBody(box(40.0), "n");
    std::vector<int> want{d.a, n};
    std::sort(want.begin(), want.end());
    EXPECT_EQ(changedBodies(before, d.doc), want);
}

TEST(BodyChanges, ScopeMarksOnExitAndFallsBackWithoutAPerBodyMark) {
    Doc d;
    std::set<int> marked;
    {
        BodyChangeScope scope(d.doc, [&](int id) { marked.insert(id); });
        d.doc.updateBody(d.b, box(21.0));
        EXPECT_TRUE(marked.empty()); // nothing until the scope closes
    }
    EXPECT_EQ(marked, std::set<int>{d.b});
    bool all = false;
    {
        BodyChangeScope scope(d.doc, nullptr, [&] { all = true; });
        d.doc.updateBody(d.a, box(11.0));
    }
    EXPECT_TRUE(all);
}


// The blind spot that makes Application keep removed ids dirty.
//
// A history replay removes a body and re-adds it with the IDENTICAL shape
// (ReplayOp::applyDelta does exactly this for steps it is not changing). A
// before/after diff cannot see that: the body is present with the same shape
// at both ends, so the scope marks NOTHING. Meanwhile BodyRemovedEvent has
// already told the renderer to drop the slot, and there is no added-body
// event to put it back - that event is the only body lifecycle event there
// is. Application therefore marks the id on removal instead of clearing it,
// and the partial rebuild re-adopts the body that came back.
//
// If this test ever starts failing because the scope DOES mark the body, the
// marking in the BodyRemovedEvent handler becomes belt-and-braces rather than
// load-bearing - worth knowing before removing it.
TEST(BodyChanges, ARemoveAndIdenticalRestoreIsInvisibleToTheDiff) {
    Doc d;
    const TopoDS_Shape original = d.doc.getBody(d.b);
    std::set<int> marked;
    {
        BodyChangeScope scope(d.doc, [&](int id) { marked.insert(id); });
        d.doc.removeBody(d.b);
        d.doc.putBody(d.b, original);   // same id, same shape
    }
    EXPECT_TRUE(marked.empty())
        << "the diff noticed a remove-and-restore; if that is now true, "
           "Application's removal marking is no longer load-bearing";

    // And the contrast: restoring something DIFFERENT is seen.
    std::set<int> marked2;
    {
        BodyChangeScope scope(d.doc, [&](int id) { marked2.insert(id); });
        d.doc.removeBody(d.b);
        d.doc.putBody(d.b, box(33.0));
    }
    EXPECT_EQ(marked2, std::set<int>{d.b});
}

namespace {

// A GL-free renderer-slot model exercises the removal event independently
// of the net-change diff, including identical-shape rollback.
struct PanelDirtyContract : testing::Test {
    materializr::EventBus bus;
    Document doc;
    History history;
    std::set<int> dirty;
    std::map<int, TopoDS_Shape> slots;
    std::function<void(int)> mark = [this](int id) { dirty.insert(id); };
    int a, b, sibling;

    void SetUp() override {
        doc.setEventBus(&bus);
        a = doc.addBody(box(10), "a");
        b = doc.addBody(box(20), "b");
        sibling = doc.addBody(box(30), "sibling");
        for (int i = 0; i < 20; ++i) doc.addBody(box(1), "unrelated");
        for (int id : doc.getAllBodyIds()) slots[id] = doc.getBody(id);
        bus.subscribe<materializr::BodyRemovedEvent>([this](const auto& e) {
            slots.erase(e.bodyId);
            mark(e.bodyId);
        });
    }

    void rebuild() {
        for (int id : dirty) {
            if (doc.isBodyVisible(id)) slots[id] = doc.getBody(id);
            else slots.erase(id);
        }
        dirty.clear();
    }

    void visibility(int id, bool visible) {
        setBodyVisibleAndMark(doc, id, visible, mark);
    }

    void allVisible(int isolated = -1) {
        if (isolated < 0) showAllBodies(doc, mark);
        else isolateBody(doc, isolated, mark);
    }

    void folderVisible(int folder, bool target) {
        setFolderVisibleAndMark(doc, folder, target, mark);
    }

    void folderColor(int folder, glm::vec3 target) {
        setFolderColorAndMark(doc, folder, target, mark);
    }
};

class PanelEditOp : public Operation {
public:
    int id;
    TopoDS_Shape before, after = box(12);
    bool fail = false;
    explicit PanelEditOp(int body) : id(body) {}
    bool execute(Document& doc) override {
        if (fail) { doc.removeBody(id); return false; }
        before = doc.getBody(id);
        doc.updateBody(id, after);
        return true;
    }
    bool undo(Document& doc) override { doc.updateBody(id, before); return true; }
    std::string name() const override { return "Panel edit"; }
    std::string description() const override { return name(); }
    std::string typeId() const override { return "panel_edit_test"; }
    void renderProperties() override {}
};

} // namespace

TEST_F(PanelDirtyContract, BodyAppearanceMarksOnlyItsId) {
    visibility(a, false);
    EXPECT_EQ(dirty, std::set<int>{a});
    rebuild();
    EXPECT_EQ(slots.count(a), 0u);
    EXPECT_EQ(slots.count(b), 1u);
    visibility(a, true);
    rebuild();
    EXPECT_TRUE(slots.at(a).IsEqual(doc.getBody(a)));
    setBodyColorAndMark(doc, b, glm::vec3(1, 0, 0), mark);
    EXPECT_EQ(dirty, std::set<int>{b});
    EXPECT_EQ(doc.getBodyColor(b), glm::vec3(1, 0, 0));
}

TEST_F(PanelDirtyContract, FolderFanOutMarksOnlyChangedMembers) {
    int folder = doc.addFolder("target");
    int other = doc.addFolder("sibling");
    doc.setBodyFolder(a, folder);
    doc.setBodyFolder(b, folder);
    doc.setBodyFolder(sibling, other);
    doc.setBodyVisible(a, false);
    folderVisible(folder, false);
    EXPECT_EQ(dirty, std::set<int>{b});
    EXPECT_TRUE(doc.isBodyVisible(sibling));
    dirty.clear();
    folderVisible(folder, false);
    EXPECT_TRUE(dirty.empty());
    glm::vec3 red(1, 0, 0);
    auto siblingColor = doc.getBodyColor(sibling);
    doc.setBodyColor(a, red);
    auto before = snapshotBodies(doc);
    folderColor(folder, red);
    EXPECT_EQ(dirty, std::set<int>{b});
    EXPECT_TRUE(changedBodies(before, doc).empty());
    EXPECT_EQ(doc.getBodyColor(b), red);
    EXPECT_EQ(doc.getBodyColor(sibling), siblingColor);
    dirty.clear();
    folderColor(folder, red);
    EXPECT_TRUE(dirty.empty());
}

TEST_F(PanelDirtyContract, IsolateAndShowAllHaveExactPartialAndZeroDirtySets) {
    allVisible();
    EXPECT_TRUE(dirty.empty());
    doc.setBodyVisible(b, false);
    doc.setBodyVisible(sibling, false);
    allVisible();
    EXPECT_EQ(dirty, (std::set<int>{b, sibling}));
    dirty.clear();
    for (int id : doc.getAllBodyIds()) doc.setBodyVisible(id, id == a);
    allVisible(a);
    EXPECT_TRUE(dirty.empty());
    doc.setBodyVisible(b, true);
    doc.setBodyVisible(a, false);
    allVisible(a);
    EXPECT_EQ(dirty, (std::set<int>{a, b}));
    for (int id : doc.getAllBodyIds()) EXPECT_EQ(doc.isBodyVisible(id), id == a);
}

TEST_F(PanelDirtyContract, FolderDeletionPreservesVisibleAndHiddenMembers) {
    for (bool visible : {true, false}) {
        int folder = doc.addFolder("group");
        doc.setBodyFolder(a, folder);
        doc.setBodyFolder(b, folder);
        doc.setFolderVisible(folder, visible);
        doc.setBodyColor(a, glm::vec3(1, 0, 0));
        auto before = snapshotBodies(doc);
        auto color = doc.getBodyColor(a);
        doc.removeFolder(folder);
        EXPECT_TRUE(dirty.empty());
        EXPECT_TRUE(changedBodies(before, doc).empty());
        EXPECT_EQ(doc.getBodyFolder(a), -1);
        EXPECT_EQ(doc.getBodyFolder(b), -1);
        EXPECT_EQ(doc.isBodyVisible(a), visible);
        EXPECT_EQ(doc.isBodyVisible(b), visible);
        EXPECT_EQ(doc.getBodyColor(a), color);
    }
}

TEST_F(PanelDirtyContract, DeleteUsesRemovalEventAndUndoRestoresOriginalSlot) {
    auto original = doc.getBody(a);
    auto op = std::make_unique<DeleteOp>();
    op->setBodyId(a);
    ASSERT_TRUE(history.pushOperation(std::move(op), doc));
    EXPECT_EQ(dirty, std::set<int>{a});
    EXPECT_EQ(slots.count(a), 0u);
    rebuild();
    ASSERT_TRUE(undoStep(history, doc, mark));
    EXPECT_EQ(dirty, std::set<int>{a});
    rebuild();
    EXPECT_TRUE(slots.at(a).IsEqual(original));
    doc.removeBody(b);
    EXPECT_EQ(dirty, std::set<int>{b});
    EXPECT_EQ(slots.count(b), 0u);
}

// Regression test: ItemsPanel's Separate menu item pushed the op through
// History but never marked anything dirty at all (not even the old blanket
// flag) - the resized original and every split-off body sat stale until an
// unrelated action happened to force a rebuild. Fixed by wrapping the push
// in a BodyChangeScope inside materializr::separateBody (core/ItemsPanelActions.h),
// which ItemsPanel.cpp calls directly - this test exercises that same
// function, not a reimplementation of it.
TEST_F(PanelDirtyContract, SeparateMarksTheResizedBodyAndEverySplitOffPiece) {
    doc.updateBody(a, twoLumpBody());
    dirty.clear();
    ASSERT_TRUE(separateBody(doc, history, a, mark));
    const auto* separate =
        dynamic_cast<const SeparateBodyOp*>(history.getStep(history.currentStep()));
    ASSERT_NE(separate, nullptr);
    ASSERT_FALSE(separate->getNewBodyIds().empty());
    std::set<int> expected{a};
    for (int id : separate->getNewBodyIds()) expected.insert(id);
    EXPECT_EQ(dirty, expected);
    EXPECT_TRUE(doc.isBodyVisible(b)); // unrelated body, untouched
}

TEST_F(PanelDirtyContract, HistoryMutationsLeaveUnrelatedBodiesClean) {
    auto op = std::make_unique<PanelEditOp>(a);
    auto* edit = op.get();
    ASSERT_TRUE(history.pushOperation(std::move(op), doc));
    auto check = [&](const std::function<bool()>& mutation) {
        dirty.clear();
        EXPECT_TRUE(mutation());
        EXPECT_EQ(dirty, std::set<int>{a});
    };
    check([&] { return undoStep(history, doc, mark); });
    check([&] { return redoStep(history, doc, mark); });
    edit->after = box(14);
    check([&] { return applyStepEdit(history, doc, 0, mark); });
    check([&] { return toggleStepEnabled(history, doc, 0, false, mark); });
    check([&] { return toggleStepEnabled(history, doc, 0, true, mark); });
    check([&] { return deleteStep(history, doc, 0, mark); });
}

TEST_F(PanelDirtyContract, FailedReplayRestoresSlotsEvenWhenTheDiffIsEmpty) {
    auto op = std::make_unique<PanelEditOp>(a);
    auto* edit = op.get();
    ASSERT_TRUE(history.pushOperation(std::move(op), doc));
    for (bool visible : {true, false}) {
        doc.setBodyVisible(a, visible);
        mark(a);
        rebuild();
        auto original = doc.getBody(a);
        std::set<int> diffDirty;
        edit->fail = true;
        EXPECT_FALSE(applyStepEdit(history, doc, 0, [&](int id) { diffDirty.insert(id); }));
        EXPECT_TRUE(diffDirty.empty());
        EXPECT_EQ(dirty, std::set<int>{a});
        EXPECT_EQ(slots.count(a), 0u);
        EXPECT_TRUE(doc.getBody(a).IsEqual(original));
        EXPECT_EQ(doc.isBodyVisible(a), visible);
        rebuild();
        EXPECT_EQ(slots.count(a), visible ? 1u : 0u);
        if (visible) EXPECT_TRUE(slots.at(a).IsEqual(original));
        EXPECT_EQ(slots.count(sibling), 1u);
    }
}

TEST_F(PanelDirtyContract, SketchEditEventMarksTheDrivenBodyWithoutPanelInvalidation) {
    auto sketch = std::make_shared<materializr::Sketch>();
    int p0 = sketch->addPoint({0, 0});
    int p1 = sketch->addPoint({10, 0});
    int p2 = sketch->addPoint({10, 10});
    int p3 = sketch->addPoint({0, 10});
    sketch->addLine(p0, p1);
    sketch->addLine(p1, p2);
    sketch->addLine(p2, p3);
    sketch->addLine(p3, p0);
    int sid = doc.addSketch(sketch);
    auto op = std::make_unique<ExtrudeOp>();
    auto* extrude = op.get();
    extrude->setSketchSource(sid);
    extrude->setDistance(5);
    ASSERT_TRUE(extrude->rebuildProfileFromSketch(doc));
    ASSERT_TRUE(history.pushOperation(std::move(op), doc));
    int driven = doc.getAllBodyIds().back();
    auto original = doc.getBody(driven);
    bus.subscribe<materializr::SketchEditedEvent>([&](const auto& e) {
        ASSERT_EQ(e.sketchId, sid);
        ASSERT_TRUE(extrude->rebuildProfileFromSketch(doc));
        auto before = snapshotBodies(doc);
        doc.setCascadeSketchOverride(sid, std::make_shared<materializr::Sketch>(*sketch));
        ASSERT_TRUE(history.editStep(0, doc, true));
        doc.clearCascadeSketchOverrides();
        for (int id : changedBodies(before, doc)) mark(id);
    });
    auto before = std::make_shared<materializr::Sketch>(*sketch);
    sketch->movePoint(p1, {15, 0});
    sketch->movePoint(p2, {15, 10});
    auto after = std::make_shared<materializr::Sketch>(*sketch);
    history.pushExecuted(std::make_unique<materializr::SketchEditOp>(sketch, before, after));
    bus.publish(materializr::SketchEditedEvent{sid});
    EXPECT_EQ(dirty, std::set<int>{driven});
    EXPECT_FALSE(doc.getBody(driven).IsEqual(original));
    rebuild();
    EXPECT_TRUE(slots.at(driven).IsEqual(doc.getBody(driven)));
}

// The Document tests cannot catch a layout consuming the old bool contract.
TEST(PanelDirtyWiring, BothLayoutsUseTheSharedCallbacks) {
    auto read = [](const char* path) {
        std::ifstream file(std::string(MZR_SOURCE_DIR) + path);
        std::ostringstream text;
        text << file.rdbuf();
        return text.str();
    };
    auto app = read("/src/app/Application.cpp");
    auto modern = read("/src/app/layout/modern/ModernLayout.cpp");
    ASSERT_FALSE(app.empty());
    ASSERT_FALSE(modern.empty());
    for (const std::string panel : {"items", "history", "properties"}) {
        std::string setter = "m_" + panel + "Panel->setBodyDirtyCallback([this](int id) { markBodyDirty(id); });";
        auto pos = app.find(setter);
        ASSERT_NE(pos, std::string::npos) << panel;
        EXPECT_EQ(app.find(setter, pos + 1), std::string::npos) << panel;
    }
    for (const std::string panel : {"history", "properties"}) {
        EXPECT_NE(app.find("m_" + panel + "Panel->render();"), std::string::npos);
        EXPECT_NE(modern.find("m_" + panel + "Panel->renderContent();"), std::string::npos);
        EXPECT_EQ(app.find("m_" + panel + "Panel->render())"), std::string::npos);
        EXPECT_EQ(modern.find("m_" + panel + "Panel->renderContent())"), std::string::npos);
    }
    for (auto pair : {std::make_pair(app, "render"), std::make_pair(modern, "renderContent")}) {
        auto pos = pair.first.find(std::string("m_itemsPanel->") + pair.second + "()) {");
        ASSERT_NE(pos, std::string::npos);
        auto body = pair.first.substr(pos, pair.first.find('}', pos) - pos);
        EXPECT_NE(body.find("m_hoveredBodyId = -1;"), std::string::npos);
        EXPECT_EQ(body.find("m_meshesDirty"), std::string::npos);
    }
    auto items = read("/src/ui/ItemsPanel.cpp");
    EXPECT_NE(items.find("return m_bodyDeleted;"), std::string::npos);
    EXPECT_EQ(items.find("colorChanged"), std::string::npos);
    {
        // Scoped to the Separate menu item's own block (not just "found
        // somewhere in the file"): guards against a future edit reintroducing
        // inline Separate logic (the original bug) without any test noticing.
        auto sepPos = items.find("tr(\"Separate\")");
        ASSERT_NE(sepPos, std::string::npos);
        auto sepBody = items.substr(sepPos, items.find('}', sepPos) - sepPos);
        EXPECT_NE(sepBody.find("separateBody("), std::string::npos);
    }
}
