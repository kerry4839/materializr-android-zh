#pragma once

namespace materializr {

// Android-only startup hook (no-op on desktop). Run ONCE before constructing
// Application. It:
//   * points $HOME at app-private internal storage so SettingsIO::defaultPath()
//     writes a readable/writable settings file (no code change there);
//   * chdir()s there and extracts the bundled TTFs so resolveBundledFont()'s
//     cwd-relative "assets/fonts/" lookup succeeds (no code change there);
//   * extracts the bundled OpenCASCADE resource tree and sets the CSF_* env vars
//     OCCT consults for unit/message/STEP/IGES data.
void androidInitRuntime();

} // namespace materializr
