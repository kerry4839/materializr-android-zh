#include <gtest/gtest.h>
#include "core/History.h"
#include "core/SketchCascadeTarget.h"
#include "modeling/ExtrudeOp.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <vector>

namespace {

class MockAddBodyOp : public Operation {
public:
    bool execute(Document& doc) override {
        m_createdId = doc.addBody(BRepPrimAPI_MakeBox(10, 10, 10).Shape(), "Box");
        return true;
    }
    bool undo(Document& doc) override {
        doc.removeBody(m_createdId);
        return true;
    }
    std::string name() const override { return "Add Box"; }
    std::string description() const override { return "Add test box"; }
    void renderProperties() override {}
    std::string typeId() const override { return "mock_add_body"; }

private:
    int m_createdId = -1;
};

} // namespace

TEST(SketchCascadeTargetTest, ResolvesSketchOperations) {
    Document doc;
    auto sketch = std::make_shared<materializr::Sketch>();
    const int sid = doc.addSketch(sketch);
    materializr::SketchEditOp edit(sketch, sketch, sketch);
    EXPECT_EQ(materializr::sketchIdForCascade(&edit, doc), sid);
    materializr::SketchTransformOp transform;
    transform.setSketch(sid);
    EXPECT_EQ(materializr::sketchIdForCascade(&transform, doc), sid);
    MockAddBodyOp body;
    EXPECT_EQ(materializr::sketchIdForCascade(&body, doc), -1);
    EXPECT_EQ(materializr::sketchIdForCascade(nullptr, doc), -1);
    materializr::SketchEditOp missing(nullptr, nullptr, nullptr);
    EXPECT_EQ(materializr::sketchIdForCascade(&missing, doc), -1);
    auto unregistered = std::make_shared<materializr::Sketch>();
    materializr::SketchEditOp unknown(unregistered, unregistered, unregistered);
    EXPECT_EQ(materializr::sketchIdForCascade(&unknown, doc), -1);
}

TEST(SketchCascadeTargetTest, DispatchesOnlyDistinctAppliedTargets) {
    Document doc;
    auto sketch = std::make_shared<materializr::Sketch>();
    const int sid = doc.addSketch(sketch);
    const int other = doc.addSketch(std::make_shared<materializr::Sketch>());
    materializr::SketchEditOp edit(sketch, sketch, sketch);
    MockAddBodyOp body;
    std::vector<int> fired;
    auto cascade = [&](int id) { fired.push_back(id); };

    materializr::dispatchUndoRedoCascade(-1, &edit, doc, sid, cascade);
    EXPECT_TRUE(fired.empty());
    materializr::dispatchUndoRedoCascade(0, &body, doc, sid, cascade);
    EXPECT_EQ(fired, (std::vector<int>{sid}));
    fired.clear();
    materializr::dispatchUndoRedoCascade(0, &edit, doc, -1, cascade);
    EXPECT_EQ(fired, (std::vector<int>{sid}));
    fired.clear();
    materializr::dispatchUndoRedoCascade(0, &edit, doc, sid, cascade);
    EXPECT_EQ(fired, (std::vector<int>{sid}));
    fired.clear();
    materializr::dispatchUndoRedoCascade(0, &edit, doc, other, cascade);
    EXPECT_EQ(fired, (std::vector<int>{other, sid}));
    fired.clear();
    materializr::dispatchUndoRedoCascade(0, nullptr, doc, -1, cascade);
    EXPECT_TRUE(fired.empty());
}

TEST(SketchCascadeTargetTest, DisabledTipDispatchesActuallyUndoneSketchEdit) {
    Document doc;
    History history;
    auto sketch = std::make_shared<materializr::Sketch>();
    int p0 = sketch->addPoint({0, 0});
    int p1 = sketch->addPoint({10, 0});
    int p2 = sketch->addPoint({10, 10});
    int p3 = sketch->addPoint({0, 10});
    sketch->addLine(p0, p1);
    sketch->addLine(p1, p2);
    sketch->addLine(p2, p3);
    sketch->addLine(p3, p0);
    const int sid = doc.addSketch(sketch);
    auto extrude = std::make_unique<ExtrudeOp>();
    extrude->setSketchSource(sid);
    extrude->setDistance(5);
    ASSERT_TRUE(extrude->rebuildProfileFromSketch(doc));
    ASSERT_TRUE(history.pushOperation(std::move(extrude), doc));
    auto before = std::make_shared<materializr::Sketch>(*sketch);
    sketch->movePoint(p1, {15, 0});
    sketch->movePoint(p2, {15, 10});
    auto after = std::make_shared<materializr::Sketch>(*sketch);
    history.pushExecuted(std::make_unique<materializr::SketchEditOp>(sketch, before, after));
    ASSERT_TRUE(history.pushOperation(std::make_unique<MockAddBodyOp>(), doc));
    ASSERT_TRUE(history.setStepEnabled(2, false, doc));
    ASSERT_EQ(history.currentStep(), 2);
    ASSERT_EQ(materializr::sketchIdForCascade(history.getStep(2), doc), -1);

    ASSERT_TRUE(history.undo(doc));
    EXPECT_EQ(history.lastUndoneStep(), 1);
    std::vector<int> fired;
    auto cascade = [&](int id) { fired.push_back(id); };
    materializr::dispatchUndoRedoCascade(history.lastUndoneStep(),
        history.getStep(history.lastUndoneStep()), doc, -1, cascade);
    EXPECT_EQ(fired, (std::vector<int>{sid}));
}
