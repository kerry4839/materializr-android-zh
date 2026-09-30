#include "ProjectSession.h"

#include "core/Document.h"
#include "core/History.h"
#include "core/SelectionManager.h"

namespace materializr {

ProjectSession::ProjectSession()
    : document(std::make_unique<Document>()),
      history(std::make_unique<History>()),
      selection(std::make_unique<SelectionManager>()) {}

// Out-of-line so the unique_ptrs' deleters see complete types.
ProjectSession::~ProjectSession() = default;

} // namespace materializr
