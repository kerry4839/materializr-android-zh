#pragma once
#include <gp_Ax2.hxx>

class Document;
class SelectionManager;

namespace materializr {

// What the cylindrical-face detector found: a closed cylindrical face (or one
// circular edge of one) that Edit Diameter and Thread can both work from.
//
// This used to have no type. detectCylindricalResizeCandidate() returned bool
// and left its answer in fifteen m_resizeCyl* members on Application, and TWO
// unrelated features read them back out: Edit Diameter (its own fields) and
// Thread, whose begin() copied nine of them into m_threadAxis[]. So the resize
// state was secretly also the thread's input, and "extract resize into a
// controller" would have silently broken threading.
//
// Returning a value instead means the detector has one caller-visible output,
// nobody reads anybody else's members, and the resize state is free to move
// into a controller (the point of the exercise).
struct CylindricalPick {
    bool ok = false;        // false: selection isn't a resizable cylinder

    int  bodyId = -1;
    bool isHole = true;     // normal toward the axis = material outside = hole

    // Anchored at the V_min end of the affected region; +Z runs to the far end.
    gp_Ax2 axis;
    double height  = 0.0;
    double bottomR = 0.0;   // radius at the axis location
    double topR    = 0.0;   // radius at the +height end (differs on a cone)

    // Which ends this edit moves. A face pick edits both (stays cylindrical);
    // an edge pick edits only its own end (turning the cylinder into a cone).
    bool editBottom = true;
    bool editTop    = true;
};

// Free function, not an Application method: it only ever needed the document
// and the selection, and a controller gets both from its IopContext. That's
// what lets ResizeCylindricalController resolve its own target instead of
// being handed one by the god-class.
CylindricalPick detectCylindricalPick(const Document& doc,
                                      const SelectionManager& selection);

} // namespace materializr
