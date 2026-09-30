#include "Verbose.h"

namespace materializr {

namespace {
bool g_verbose = false;
}

bool isVerbose() { return g_verbose; }
void setVerbose(bool enabled) { g_verbose = enabled; }

} // namespace materializr
