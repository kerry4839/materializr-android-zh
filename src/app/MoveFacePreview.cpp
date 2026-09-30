#include "MoveFacePreview.h"

#include "modeling/MoveFaceOp.h"

#include <BRepBuilderAPI_Copy.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>

#include <chrono>

namespace materializr {

MoveFacePreviewJob::MoveFacePreviewJob() = default;
MoveFacePreviewJob::~MoveFacePreviewJob() = default;

std::unique_ptr<MoveFacePreviewJob> MoveFacePreviewJob::prepare(
    const TopoDS_Shape& originalBody, const TopoDS_Face& face,
    const std::function<void(MoveFaceOp&)>& configure)
{
    if (originalBody.IsNull() || face.IsNull()) return nullptr;
    try {
        TopTools_IndexedMapOfShape faces;
        TopExp::MapShapes(originalBody, TopAbs_FACE, faces);
        if (!faces.Contains(face)) return nullptr; // stale handle - refuse, don't guess

        BRepBuilderAPI_Copy copier;
        copier.Perform(originalBody, Standard_True, Standard_False);
        // The copier maps by IsSame and hands back FORWARD copies; the op
        // was given the sub-shape as oriented in the body.
        const TopoDS_Shape scratchFace =
            copier.ModifiedShape(face).Oriented(face.Orientation()); // throws if not a sub-shape
        if (scratchFace.IsNull() || scratchFace.ShapeType() != TopAbs_FACE) return nullptr;

        std::unique_ptr<MoveFacePreviewJob> job(new MoveFacePreviewJob());
        job->m_scratch = std::make_unique<Document>();
        job->m_scratchBodyId = job->m_scratch->addBody(copier.Shape(), "preview");
        job->m_scratchBase = copier.Shape();
        job->m_op = std::make_unique<MoveFaceOp>();
        job->m_op->setBody(job->m_scratchBodyId);
        job->m_op->setFace(TopoDS::Face(scratchFace));
        configure(*job->m_op);
        return job;
    } catch (...) {
        return nullptr; // a refused copy or a stale sub-shape: not previewed off-thread
    }
}

MoveFacePreviewResult MoveFacePreviewJob::run()
{
    MoveFacePreviewResult r;
    if (!m_scratch || !m_op) return r;
    const auto t0 = std::chrono::steady_clock::now();
    try {
        r.ok = m_op->execute(*m_scratch);
    } catch (...) {
        r.ok = false;
    }
    r.millis = std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - t0).count();
    if (!r.ok) return r;
    try {
        r.shape = m_scratch->getBody(m_scratchBodyId);
    } catch (...) {
        r.ok = false;
        return r;
    }
    r.key = m_op->previewKey(m_scratchBase);
    return r;
}

} // namespace materializr
