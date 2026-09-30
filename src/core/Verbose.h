#pragma once

namespace materializr {

// Verbose logging gate. Off by default; main() flips it on when the user
// passes `--verbose` / `--log <path>`. Modeling ops use this to decide
// whether to spit per-operation diagnostics to stderr (or to the redirected
// log file). Without it, normal launches stay quiet.
bool isVerbose();
void setVerbose(bool enabled);

} // namespace materializr
