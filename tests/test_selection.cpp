#include <gtest/gtest.h>
#include "core/SelectionManager.h"

// Helper: create a body selection entry
static SelectionEntry makeBodyEntry(int bodyId) {
    SelectionEntry entry;
    entry.type = SelectionType::Body;
    entry.bodyId = bodyId;
    entry.subShapeIndex = -1;
    return entry;
}

// Helper: create a face selection entry
static SelectionEntry makeFaceEntry(int bodyId, int faceIndex) {
    SelectionEntry entry;
    entry.type = SelectionType::Face;
    entry.bodyId = bodyId;
    entry.subShapeIndex = faceIndex;
    return entry;
}

// Helper: create an edge selection entry
static SelectionEntry makeEdgeEntry(int bodyId, int edgeIndex) {
    SelectionEntry entry;
    entry.type = SelectionType::Edge;
    entry.bodyId = bodyId;
    entry.subShapeIndex = edgeIndex;
    return entry;
}

TEST(SelectionManagerTest, InitialStateNoSelection) {
    SelectionManager mgr;
    EXPECT_FALSE(mgr.hasSelection());
    EXPECT_EQ(mgr.primaryType(), SelectionType::None);
    EXPECT_TRUE(mgr.getSelection().empty());
}

TEST(SelectionManagerTest, SelectSetsSelection) {
    SelectionManager mgr;

    SelectionEntry entry = makeBodyEntry(1);
    mgr.select(entry);

    EXPECT_TRUE(mgr.hasSelection());
    EXPECT_EQ(mgr.getSelection().size(), 1u);
    EXPECT_EQ(mgr.getSelection()[0].bodyId, 1);
}

TEST(SelectionManagerTest, SelectReplacesExisting) {
    SelectionManager mgr;

    mgr.select(makeBodyEntry(1));
    mgr.select(makeBodyEntry(2));

    // select() should replace, not add
    EXPECT_EQ(mgr.getSelection().size(), 1u);
    EXPECT_EQ(mgr.getSelection()[0].bodyId, 2);
}

TEST(SelectionManagerTest, ClearRemovesSelection) {
    SelectionManager mgr;

    mgr.select(makeBodyEntry(1));
    EXPECT_TRUE(mgr.hasSelection());

    mgr.clear();
    EXPECT_FALSE(mgr.hasSelection());
    EXPECT_TRUE(mgr.getSelection().empty());
}

TEST(SelectionManagerTest, AddToSelectionAccumulatesEntries) {
    SelectionManager mgr;

    mgr.addToSelection(makeBodyEntry(1));
    mgr.addToSelection(makeBodyEntry(2));
    mgr.addToSelection(makeBodyEntry(3));

    EXPECT_EQ(mgr.getSelection().size(), 3u);
}

TEST(SelectionManagerTest, AddToSelectionNoDuplicates) {
    SelectionManager mgr;

    mgr.addToSelection(makeBodyEntry(1));
    mgr.addToSelection(makeBodyEntry(1)); // duplicate

    EXPECT_EQ(mgr.getSelection().size(), 1u);
}

TEST(SelectionManagerTest, PrimaryTypeReflectsFirstItem) {
    SelectionManager mgr;

    mgr.addToSelection(makeFaceEntry(1, 0));
    mgr.addToSelection(makeEdgeEntry(1, 1));

    EXPECT_EQ(mgr.primaryType(), SelectionType::Face);
}

TEST(SelectionManagerTest, HasSelectedBodies) {
    SelectionManager mgr;

    EXPECT_FALSE(mgr.hasSelectedBodies());

    mgr.addToSelection(makeBodyEntry(1));
    EXPECT_TRUE(mgr.hasSelectedBodies());
    EXPECT_FALSE(mgr.hasSelectedFaces());
    EXPECT_FALSE(mgr.hasSelectedEdges());
}

TEST(SelectionManagerTest, HasSelectedFaces) {
    SelectionManager mgr;

    EXPECT_FALSE(mgr.hasSelectedFaces());

    mgr.addToSelection(makeFaceEntry(1, 0));
    EXPECT_TRUE(mgr.hasSelectedFaces());
    EXPECT_FALSE(mgr.hasSelectedBodies());
    EXPECT_FALSE(mgr.hasSelectedEdges());
}

TEST(SelectionManagerTest, HasSelectedEdges) {
    SelectionManager mgr;

    EXPECT_FALSE(mgr.hasSelectedEdges());

    mgr.addToSelection(makeEdgeEntry(1, 0));
    EXPECT_TRUE(mgr.hasSelectedEdges());
    EXPECT_FALSE(mgr.hasSelectedBodies());
    EXPECT_FALSE(mgr.hasSelectedFaces());
}

TEST(SelectionManagerTest, MixedSelectionCounts) {
    SelectionManager mgr;

    mgr.addToSelection(makeBodyEntry(1));
    mgr.addToSelection(makeBodyEntry(2));
    mgr.addToSelection(makeFaceEntry(1, 0));
    mgr.addToSelection(makeEdgeEntry(1, 0));
    mgr.addToSelection(makeEdgeEntry(1, 1));

    EXPECT_EQ(mgr.selectedBodyCount(), 2);
    EXPECT_EQ(mgr.selectedFaceCount(), 1);
    EXPECT_EQ(mgr.selectedEdgeCount(), 2);
}

TEST(SelectionManagerTest, ToggleSelectionAddsIfNotPresent) {
    SelectionManager mgr;

    mgr.toggleSelection(makeBodyEntry(1));
    EXPECT_TRUE(mgr.hasSelection());
    EXPECT_EQ(mgr.getSelection().size(), 1u);
}

TEST(SelectionManagerTest, ToggleSelectionRemovesIfPresent) {
    SelectionManager mgr;

    SelectionEntry entry = makeBodyEntry(1);
    mgr.select(entry);
    EXPECT_TRUE(mgr.hasSelection());

    mgr.toggleSelection(entry);
    EXPECT_FALSE(mgr.hasSelection());
}

TEST(SelectionManagerTest, ToggleSelectionDoubleToggle) {
    SelectionManager mgr;

    SelectionEntry entry = makeBodyEntry(1);
    mgr.toggleSelection(entry); // add
    mgr.toggleSelection(entry); // remove

    EXPECT_FALSE(mgr.hasSelection());
}

TEST(SelectionManagerTest, RemoveFromSelection) {
    SelectionManager mgr;

    mgr.addToSelection(makeBodyEntry(1));
    mgr.addToSelection(makeBodyEntry(2));
    mgr.addToSelection(makeBodyEntry(3));

    mgr.removeFromSelection(makeBodyEntry(2));
    EXPECT_EQ(mgr.getSelection().size(), 2u);
    EXPECT_EQ(mgr.selectedBodyCount(), 2);
}

TEST(SelectionManagerTest, RemoveNonexistentDoesNothing) {
    SelectionManager mgr;

    mgr.addToSelection(makeBodyEntry(1));
    mgr.removeFromSelection(makeBodyEntry(999));

    EXPECT_EQ(mgr.getSelection().size(), 1u);
}
