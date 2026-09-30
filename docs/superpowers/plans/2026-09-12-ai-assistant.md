# AI Assistant Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let a user type a natural-language prompt ("make a 20mm cube with a 5mm hole") that an LLM turns into real modeling operations in the app.

**Architecture:** Testable logic (tool schema, tool dispatch, both LLM clients' request/response handling, the turn-loop state machine) lives under a new `src/ai/` directory so it can be added to `materializr_core`'s test-lib source list, exactly the way `MateSolver.cpp`/`Mate.cpp` (testable) are split from `MatePlugin.cpp` (untested UI glue). Only the actual plugin registration, toolbar button, and chat overlay UI live in `src/plugins/AiAssistantPlugin.cpp`. Every AI-generated document change goes through the existing `Operation`/`History::pushOperation()` path - no new mutation mechanism.

**Tech Stack:** C++17, OCCT, ImGui, libcurl (already linked), nlohmann/json (new), GoogleTest.

**Spec:** `docs/superpowers/specs/2026-09-12-ai-assistant-design.md` (as amended by its own later "Platform Scope: Desktop Only" section, both in the same file, committed on `land/assembly-mates`).

## Global Constraints

- Desktop only (Windows/Linux/macOS). Excluded from the Android build (Task 10). See spec's "Platform Scope" section.
- v1 tool set is exactly: `add_box`, `add_cylinder`, `add_sphere`, `add_cone`, `add_torus`, `move_body`, `rotate_body`, `scale_body` (uniform only), `boolean_op`. No sketching, extrude, mates, or patterns.
- Every AI action is a normal undoable `Operation` via `history().pushOperation()`. No preview/confirm step.
- Agentic loop, max 8 tool-call steps per user prompt.
- API keys stored in the existing plain-text `settings.cfg`, excluded from Settings export/import (same as `lastProjectPath`).
- `nlohmann/json` pinned to an exact release tag via CMake `FetchContent` (this repo requires pinned tags, never branches, for every dependency - see the `imgui` `FetchContent_Declare` comment in `CMakeLists.txt:62-92` for why).
- No GUI/overlay unit tests - matches this repo's established convention (confirmed zero existing tests touch `PluginContext`/`REGISTER_PLUGIN`/ImGui rendering anywhere in the codebase).
- `far`/`near` are reserved words on MSVC (`<windows.h>` macros) - grep any new code for `\bfar\b|\bnear\b` before considering a task done.
- No em dashes (`tools/no_em_dashes.py` gate) in any new source comment or string.

---

### Task 1: Add the nlohmann/json dependency

**Files:**
- Modify: `CMakeLists.txt` (add `FetchContent_Declare`/`FetchContent_MakeAvailable` block, link into the `materializr` target)
- Modify: `tests/CMakeLists.txt` (link the same target into `materializr_core`)
- Modify: `android/app/jni/src/CMakeLists.txt` (nlohmann/json is header-only and has no network/curl dependency, so it IS needed on Android too once `AiToolSchema`/`AiToolDispatcher` end up there indirectly through `materializr_core`-shaped sources - but since Task 10 excludes the whole `src/ai/` directory from Android, this file needs NO change. Note this explicitly so a future worker doesn't add one by mistake.)
- Test: none (infra-only; verified by the next task's build)

**Interfaces:**
- Produces: the `nlohmann_json::nlohmann_json` CMake target, usable via `#include <nlohmann/json.hpp>` and `nlohmann::json` from any `.cpp` that links `materializr` or `materializr_core`.

- [ ] **Step 1: Add the FetchContent block**

In `CMakeLists.txt`, immediately after the `glm` block (after line 67's `FetchContent_MakeAvailable(glm)`), add:

```cmake
# nlohmann/json - header-only, pinned to an exact release tag (see the imgui
# block above for why this repo never pins a branch). Needed by the AI
# Assistant feature's two LLM clients to build/parse request and response
# bodies; UpdateChecker's hand-rolled findJsonString stays as-is for the one
# field it reads.
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)
```

- [ ] **Step 2: Link it into the `materializr` target**

In `CMakeLists.txt`, find the `find_package(CURL REQUIRED)` / `target_link_libraries(materializr PRIVATE CURL::libcurl)` pair (around line 546) and add right after it:

```cmake
target_link_libraries(materializr PRIVATE nlohmann_json::nlohmann_json)
```

- [ ] **Step 3: Link it into the test library**

In `tests/CMakeLists.txt`, find the `target_link_libraries(materializr_core PUBLIC ...)` block (the one ending in `ZLIB::ZLIB`) and add one more line inside it:

```cmake
    nlohmann_json::nlohmann_json
```

- [ ] **Step 4: Verify the build picks it up**

```bash
cmake --build build --target materializr_core -j8
```

Expected: configures and builds clean (nlohmann/json is fetched on first configure - if CMake wasn't re-run since editing `CMakeLists.txt`, run `cmake -S . -B build` first). No new warnings.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt
git commit -m "Add nlohmann/json dependency for the AI Assistant feature

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 2: `AppSettings::AiSettings` + persistence

**Files:**
- Modify: `src/io/Settings.h` (add `AiProvider` enum, `AiSettings` struct, `AiSettings ai;` member on `AppSettings`)
- Modify: `src/io/Settings.cpp` (`applyKv`, `save`, `exportJson`, `importJson`)
- Test: Create `tests/test_ai_settings.cpp`
- Modify: `tests/CMakeLists.txt` (register the new test executable)

**Interfaces:**
- Produces: `materializr::AiProvider` (enum: `Anthropic`, `OpenAiCompatible`), `materializr::AppSettings::AiSettings` (fields: `provider`, `anthropicApiKey`, `anthropicModel`, `openAiApiKey`, `openAiBaseUrl`, `openAiModel`), reachable as `AppSettings::ai`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_ai_settings.cpp`:

```cpp
// AI Assistant settings round-trip through the plain-text .cfg file and are
// deliberately excluded from the portable JSON export/import path (same
// treatment as lastProjectPath) so a shared settings backup can't leak an
// API key.
#include "io/Settings.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using materializr::AppSettings;
using materializr::AiProvider;
namespace SettingsIO = materializr::SettingsIO;

namespace {
std::string tmpCfg(const char* tag) {
    static int n = 0;
    return (fs::temp_directory_path() /
            ("mzr_ai_settings_" + std::string(tag) + "_" +
             std::to_string(++n) + ".cfg")).string();
}
} // namespace

TEST(AiSettings, RoundTripsThroughTheCfgFile) {
    const std::string p = tmpCfg("roundtrip");
    AppSettings s;
    s.ai.provider = AiProvider::OpenAiCompatible;
    s.ai.anthropicApiKey = "sk-ant-test123";
    s.ai.anthropicModel = "claude-sonnet-4-5";
    s.ai.openAiApiKey = "sk-openai-test456";
    s.ai.openAiBaseUrl = "http://localhost:11434/v1";
    s.ai.openAiModel = "llama3.1";
    ASSERT_TRUE(SettingsIO::save(p, s));

    AppSettings loaded = SettingsIO::load(p);
    EXPECT_EQ(loaded.ai.provider, AiProvider::OpenAiCompatible);
    EXPECT_EQ(loaded.ai.anthropicApiKey, "sk-ant-test123");
    EXPECT_EQ(loaded.ai.anthropicModel, "claude-sonnet-4-5");
    EXPECT_EQ(loaded.ai.openAiApiKey, "sk-openai-test456");
    EXPECT_EQ(loaded.ai.openAiBaseUrl, "http://localhost:11434/v1");
    EXPECT_EQ(loaded.ai.openAiModel, "llama3.1");
    fs::remove(p);
}

TEST(AiSettings, DefaultsToAnthropicWithNoKeys) {
    AppSettings s;
    EXPECT_EQ(s.ai.provider, AiProvider::Anthropic);
    EXPECT_TRUE(s.ai.anthropicApiKey.empty());
    EXPECT_TRUE(s.ai.openAiApiKey.empty());
    EXPECT_EQ(s.ai.openAiBaseUrl, "https://api.openai.com/v1");
}

TEST(AiSettings, ExcludedFromJsonExportAndImport) {
    const std::string exportPath = tmpCfg("export") + ".json";
    AppSettings s;
    s.ai.anthropicApiKey = "sk-ant-should-not-leak";
    ASSERT_TRUE(SettingsIO::exportJson(exportPath, s));

    std::ifstream f(exportPath);
    std::string body((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
    EXPECT_EQ(body.find("sk-ant-should-not-leak"), std::string::npos)
        << "an API key must never appear in an exported settings file";

    // A malicious/foreign import file setting an AI key must not be applied.
    const std::string importPath = tmpCfg("import") + ".json";
    {
        std::ofstream f2(importPath);
        f2 << "{\n  \"aiAnthropicApiKey\": \"sk-ant-injected\"\n}\n";
    }
    bool ok = false;
    AppSettings imported = SettingsIO::importJson(importPath, &ok);
    EXPECT_TRUE(imported.ai.anthropicApiKey.empty())
        << "an imported settings file must not be able to inject an API key";
    fs::remove(exportPath);
    fs::remove(importPath);
}
```

Register it in `tests/CMakeLists.txt`, right after the `test_settings_sessions` block:

```cmake
add_executable(test_ai_settings test_ai_settings.cpp
    TestSignalInit.cpp)
target_link_libraries(test_ai_settings PRIVATE materializr_core gtest gtest_main)
add_test(NAME test_ai_settings COMMAND test_ai_settings)
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target test_ai_settings -j8
```

Expected: FAIL to compile - `AppSettings` has no member `ai`, `materializr::AiProvider` does not exist.

- [ ] **Step 3: Add the struct and enum**

In `src/io/Settings.h`, right before `struct AppSettings {` (the enum/comment block already there for `UiLayout`), add:

```cpp
// The AI Assistant's backend choice. OpenAiCompatible covers OpenAI itself
// and any server that mimics its Chat Completions API (Ollama, LM Studio),
// distinguished only by openAiBaseUrl.
enum class AiProvider { Anthropic, OpenAiCompatible };
```

Then inside `struct AppSettings { ... }`, add one member (anywhere in the struct body; the end is fine):

```cpp
    // AI Assistant configuration. Kept as one sub-struct rather than six
    // scattered top-level fields since nothing outside this feature reads
    // an individual field - PluginContext::aiSettings() hands the whole
    // thing to the plugin. Excluded from JSON export/import (see
    // SettingsIO::exportJson/importJson) so a shared settings backup can't
    // leak an API key, same treatment as lastProjectPath.
    struct AiSettings {
        AiProvider provider = AiProvider::Anthropic;
        std::string anthropicApiKey;
        std::string anthropicModel = "claude-sonnet-4-5";
        std::string openAiApiKey;
        std::string openAiBaseUrl = "https://api.openai.com/v1";
        std::string openAiModel = "gpt-4o";
    };
    AiSettings ai;
```

- [ ] **Step 4: Wire the .cfg load/save path**

In `src/io/Settings.cpp`, add a name mapping next to `uiLayoutName` (same file, right after it):

```cpp
const char* aiProviderName(AiProvider p) {
    return p == AiProvider::OpenAiCompatible ? "openai_compatible" : "anthropic";
}
```

In `applyKv(...)`, add (anywhere after the existing reads; right after the `snapToGrid` line is fine):

```cpp
    {
        std::string v;
        readString(kv, "aiProvider", v);
        if (v == "openai_compatible") s.ai.provider = AiProvider::OpenAiCompatible;
        else if (v == "anthropic")    s.ai.provider = AiProvider::Anthropic;
        // unknown/missing value: keep the default (Anthropic)
    }
    readString(kv, "aiAnthropicApiKey", s.ai.anthropicApiKey);
    readString(kv, "aiAnthropicModel",  s.ai.anthropicModel);
    readString(kv, "aiOpenAiApiKey",    s.ai.openAiApiKey);
    readString(kv, "aiOpenAiBaseUrl",   s.ai.openAiBaseUrl);
    readString(kv, "aiOpenAiModel",     s.ai.openAiModel);
```

In `SettingsIO::save(...)` (the function around line 445 that writes `lastProjectPath`), add, anywhere after the `snapToGrid` write:

```cpp
    ofs << "aiProvider = "         << aiProviderName(s.ai.provider)      << "\n";
    ofs << "aiAnthropicApiKey = "  << sanitizeValue(s.ai.anthropicApiKey) << "\n";
    ofs << "aiAnthropicModel = "   << sanitizeValue(s.ai.anthropicModel)  << "\n";
    ofs << "aiOpenAiApiKey = "     << sanitizeValue(s.ai.openAiApiKey)    << "\n";
    ofs << "aiOpenAiBaseUrl = "    << sanitizeValue(s.ai.openAiBaseUrl)   << "\n";
    ofs << "aiOpenAiModel = "      << sanitizeValue(s.ai.openAiModel)     << "\n";
```

- [ ] **Step 5: Exclude from JSON export/import**

`SettingsIO::exportJson` already only writes the fields it explicitly lists - do NOT add any `ai*` lines to it. That is the whole exclusion; nothing else to change there.

In `SettingsIO::importJson(...)`, right after the existing `kv.erase("lastProjectPath");` block, add:

```cpp
    kv.erase("aiProvider");
    kv.erase("aiAnthropicApiKey");
    kv.erase("aiAnthropicModel");
    kv.erase("aiOpenAiApiKey");
    kv.erase("aiOpenAiBaseUrl");
    kv.erase("aiOpenAiModel");
```

(`parseFlatJson`, used just above this by `importJson`, already turns the imported JSON into the same `kv` map `applyKv` consumes - erasing the keys here before `applyKv(kv, s)` runs is what step 3's test asserts against.)

- [ ] **Step 6: Run test to verify it passes**

```bash
cmake --build build --target test_ai_settings -j8 && ./build/tests/test_ai_settings
```

Expected: `[ PASSED ] 3 tests.`

- [ ] **Step 7: Commit**

```bash
git add src/io/Settings.h src/io/Settings.cpp tests/test_ai_settings.cpp tests/CMakeLists.txt
git commit -m "Add AI Assistant settings (provider, keys, model, base URL)

Excluded from JSON export/import so a shared settings backup can't
leak an API key, same treatment lastProjectPath already gets.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 3: `PluginContext::aiSettings()` + Application wiring

**Files:**
- Modify: `src/plugin/PluginContext.h` (declare `aiSettings()`, add a member + `_bind()` parameter)
- Modify: `src/plugin/PluginContext.cpp` (define `aiSettings()`, extend `_bind()`)
- Modify: `src/app/Application.h` (mirror member `AppSettings::AiSettings m_aiSettings;`)
- Modify: `src/app/Application.cpp` (`applyAppSettings`, `currentSettings`, the `_bind()` call site)
- Test: none (thin plumbing; verified by build + Task 9's manual smoke test)

**Interfaces:**
- Consumes: `AppSettings::AiSettings` from Task 2.
- Produces: `PluginContext::aiSettings() const -> const AppSettings::AiSettings&`, usable by `AiAssistantPlugin` (Task 9).

- [ ] **Step 1: Declare the accessor and extend `_bind()`**

In `src/plugin/PluginContext.h`, add near the other accessors (after `const Camera& camera() const;`):

```cpp
    // The AI Assistant's provider/key/model config. A pointer, not a value,
    // because it must reflect the LIVE settings if the user edits them in
    // the Settings dialog mid-session - the plugin re-reads it on every
    // send, it doesn't cache it.
    const AppSettings::AiSettings& aiSettings() const;
```

Add the forward declaration/include it needs - `#include "../io/Settings.h"` at the top of the file, next to the existing includes.

Extend `_bind()`'s signature (both declaration here and definition in the `.cpp`):

```cpp
    void _bind(Document* doc, History* hist, SelectionManager* sel,
               EventBus* bus, Camera* cam, bool* meshesDirtyFlag,
               const bool* sketchModeFlag,
               const AppSettings::AiSettings* aiSettings,
               std::function<void()> markDirtyFn = {});
```

Add the storage member in the `private:` section, next to `m_sketchModeFlag`:

```cpp
    const AppSettings::AiSettings* m_aiSettings = nullptr;
```

- [ ] **Step 2: Define the accessor and update `_bind()`'s body**

In `src/plugin/PluginContext.cpp`, add:

```cpp
const AppSettings::AiSettings& PluginContext::aiSettings() const {
    static const AppSettings::AiSettings kEmpty;
    return m_aiSettings ? *m_aiSettings : kEmpty;
}
```

Update `PluginContext::_bind(...)`'s parameter list to match the header, and inside its body add:

```cpp
    m_aiSettings = aiSettings;
```

- [ ] **Step 3: Mirror the settings on `Application`**

In `src/app/Application.h`, add a member near the other settings mirrors (search for `m_checkForUpdatesOnLaunch` and add right after it):

```cpp
    AppSettings::AiSettings m_aiSettings;
```

In `src/app/Application.cpp`'s `applyAppSettings(const AppSettings& s)`, add:

```cpp
    m_aiSettings = s.ai;
```

In `currentSettings() const`'s builder (the function starting `AppSettings s;`), add:

```cpp
    s.ai = m_aiSettings;
```

- [ ] **Step 4: Pass it through the `_bind()` call site**

In `src/app/Application.cpp`, find the `_bind(...)` call (around line 480) and add the new argument in the matching position:

```cpp
        m_pluginContext->_bind(m_document, m_history, m_selection,
                               m_eventBus.get(), &m_viewport->getCamera(),
                               &m_meshesDirty, &m_inSketchMode,
                               &m_aiSettings,
                               [this]{ markDirty(); });
```

- [ ] **Step 5: Verify it builds**

```bash
cmake --build build --target materializr -j8
```

Expected: builds clean. (No test here - this is pure plumbing with no independently-testable behavior; Task 9's manual smoke test is the first point it's actually exercised end to end.)

- [ ] **Step 6: Commit**

```bash
git add src/plugin/PluginContext.h src/plugin/PluginContext.cpp src/app/Application.h src/app/Application.cpp
git commit -m "Wire AiSettings through PluginContext

Mirrors PluginContext::markDocumentDirty()'s precedent: a narrow,
justified extension of the plugin/host contract for one plugin's need.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 4: Shared AI types + `AiToolSchema`

**Files:**
- Create: `src/ai/AiTypes.h`
- Create: `src/ai/AiToolSchema.h`
- Create: `src/ai/AiToolSchema.cpp`
- Test: Create `tests/test_ai_tool_schema.cpp`
- Modify: `tests/CMakeLists.txt` (add `src/ai/AiToolSchema.cpp` to `materializr_core`'s source list; register the new test executable)

**Interfaces:**
- Produces:
  - `materializr::ai::ChatRole` (enum: `User`, `Assistant`, `ToolResult`)
  - `materializr::ai::ChatMessage` (fields: `role`, `text`, `toolCallId` [only meaningful for `ToolResult`])
  - `materializr::ai::ToolCall` (fields: `id`, `name`, `nlohmann::json args`)
  - `materializr::ai::LlmTurnResult` (fields: `bool ok`, `std::string error`, `std::string finalText`, `std::vector<ToolCall> toolCalls`)
  - `materializr::ai::ToolParamType` (enum: `Number`, `String`)
  - `materializr::ai::ToolParam` (fields: `name`, `type`, `required`, `description`)
  - `materializr::ai::ToolDef` (fields: `name`, `description`, `std::vector<ToolParam> params`)
  - `materializr::ai::allTools() -> const std::vector<ToolDef>&` (the fixed v1 tool list)
  - `materializr::ai::toolsToAnthropicJson(const std::vector<ToolDef>&) -> nlohmann::json`
  - `materializr::ai::toolsToOpenAiJson(const std::vector<ToolDef>&) -> nlohmann::json`

- [ ] **Step 1: Write the failing test**

Create `tests/test_ai_tool_schema.cpp`:

```cpp
#include "ai/AiToolSchema.h"

#include <gtest/gtest.h>

using namespace materializr::ai;

TEST(AiToolSchema, AllToolsContainsExactlyTheNineV1Tools) {
    const auto& tools = allTools();
    std::vector<std::string> names;
    for (const auto& t : tools) names.push_back(t.name);
    std::vector<std::string> expected = {
        "add_box", "add_cylinder", "add_sphere", "add_cone", "add_torus",
        "move_body", "rotate_body", "scale_body", "boolean_op"};
    EXPECT_EQ(names, expected);
}

TEST(AiToolSchema, EveryToolHasANonEmptyDescription) {
    for (const auto& t : allTools())
        EXPECT_FALSE(t.description.empty()) << "tool: " << t.name;
}

TEST(AiToolSchema, AnthropicJsonHasOneEntryPerToolWithInputSchema) {
    nlohmann::json j = toolsToAnthropicJson(allTools());
    ASSERT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), allTools().size());
    for (const auto& entry : j) {
        EXPECT_TRUE(entry.contains("name"));
        EXPECT_TRUE(entry.contains("input_schema"));
        EXPECT_EQ(entry["input_schema"]["type"], "object");
    }
}

TEST(AiToolSchema, OpenAiJsonWrapsEachToolInAFunctionEnvelope) {
    nlohmann::json j = toolsToOpenAiJson(allTools());
    ASSERT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), allTools().size());
    for (const auto& entry : j) {
        EXPECT_EQ(entry["type"], "function");
        EXPECT_TRUE(entry["function"].contains("name"));
        EXPECT_TRUE(entry["function"].contains("parameters"));
        EXPECT_EQ(entry["function"]["parameters"]["type"], "object");
    }
}

TEST(AiToolSchema, BooleanOpModeParamIsRequired) {
    for (const auto& t : allTools()) {
        if (t.name != "boolean_op") continue;
        bool found = false;
        for (const auto& p : t.params)
            if (p.name == "mode") { found = true; EXPECT_TRUE(p.required); }
        EXPECT_TRUE(found);
        return;
    }
    FAIL() << "boolean_op tool not found";
}
```

Register in `tests/CMakeLists.txt`, right after `test_ai_settings`:

```cmake
add_executable(test_ai_tool_schema test_ai_tool_schema.cpp
    TestSignalInit.cpp)
target_link_libraries(test_ai_tool_schema PRIVATE materializr_core gtest gtest_main)
add_test(NAME test_ai_tool_schema COMMAND test_ai_tool_schema)
```

Also add `${CMAKE_SOURCE_DIR}/src/ai/AiToolSchema.cpp` to the `materializr_core` source list in `tests/CMakeLists.txt` (right after the `Mate.cpp`/`MateSolver.cpp` lines is a reasonable spot).

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target test_ai_tool_schema -j8
```

Expected: FAIL - `ai/AiToolSchema.h` does not exist.

- [ ] **Step 3: Write the shared types header**

Create `src/ai/AiTypes.h`:

```cpp
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace materializr { namespace ai {

enum class ChatRole { User, Assistant, ToolResult };

struct ChatMessage {
    ChatRole role;
    std::string text;       // user/assistant text, or the tool result string
    std::string toolCallId; // only meaningful when role == ToolResult -
                             // must match the ToolCall::id it answers
};

struct ToolCall {
    std::string id;
    std::string name;
    nlohmann::json args;
};

struct LlmTurnResult {
    bool ok = false;
    std::string error;           // set iff !ok
    std::string finalText;       // set iff ok and toolCalls is empty
    std::vector<ToolCall> toolCalls;
};

enum class ToolParamType { Number, String };

struct ToolParam {
    std::string name;
    ToolParamType type;
    bool required = true;
    std::string description;
};

struct ToolDef {
    std::string name;
    std::string description;
    std::vector<ToolParam> params;
};

} } // namespace materializr::ai
```

- [ ] **Step 4: Write the tool schema header and implementation**

Create `src/ai/AiToolSchema.h`:

```cpp
#pragma once
#include "AiTypes.h"

namespace materializr { namespace ai {

// The fixed v1 tool list: primitives plus basic transforms and booleans.
// One definition, shared by both LLM clients - each formats it into its own
// provider's wire shape (toolsToAnthropicJson / toolsToOpenAiJson below).
const std::vector<ToolDef>& allTools();

nlohmann::json toolsToAnthropicJson(const std::vector<ToolDef>& tools);
nlohmann::json toolsToOpenAiJson(const std::vector<ToolDef>& tools);

} } // namespace materializr::ai
```

Create `src/ai/AiToolSchema.cpp`:

```cpp
#include "AiToolSchema.h"

namespace materializr { namespace ai {

namespace {
ToolParam num(const char* name, const char* desc, bool required = true) {
    return {name, ToolParamType::Number, required, desc};
}
ToolParam str(const char* name, const char* desc, bool required = true) {
    return {name, ToolParamType::String, required, desc};
}
// x, y, z default to the world origin - see AiToolDispatcher (Task 5) for
// where the default is actually applied when the model omits them.
std::vector<ToolParam> withOrigin(std::vector<ToolParam> params) {
    params.push_back(num("x", "World X position in mm (default 0).", false));
    params.push_back(num("y", "World Y position in mm (default 0).", false));
    params.push_back(num("z", "World Z position in mm (default 0).", false));
    return params;
}
} // namespace

const std::vector<ToolDef>& allTools() {
    static const std::vector<ToolDef> kTools = {
        {"add_box", "Create a rectangular box body.",
         withOrigin({num("width", "Size along X in mm."),
                     num("height", "Size along Y in mm."),
                     num("depth", "Size along Z in mm.")})},
        {"add_cylinder", "Create a cylindrical body.",
         withOrigin({num("radius", "Radius in mm."),
                     num("height", "Height in mm.")})},
        {"add_sphere", "Create a spherical body.",
         withOrigin({num("radius", "Radius in mm.")})},
        {"add_cone", "Create a conical body (top_radius 0 for a sharp point).",
         withOrigin({num("bottom_radius", "Base radius in mm."),
                     num("top_radius", "Top radius in mm; 0 for a point."),
                     num("height", "Height in mm.")})},
        {"add_torus", "Create a torus (ring) body.",
         withOrigin({num("major_radius", "Distance from centre to tube centre, in mm."),
                     num("minor_radius", "Tube radius in mm.")})},
        {"move_body", "Translate an existing body.",
         {num("body_id", "The id of the body to move."),
          num("dx", "Move along X in mm."),
          num("dy", "Move along Y in mm."),
          num("dz", "Move along Z in mm.")}},
        {"rotate_body", "Rotate an existing body about an axis through the world origin.",
         {num("body_id", "The id of the body to rotate."),
          num("axis_x", "Rotation axis X component."),
          num("axis_y", "Rotation axis Y component."),
          num("axis_z", "Rotation axis Z component."),
          num("angle_degrees", "Rotation angle in degrees.")}},
        {"scale_body", "Uniformly scale an existing body.",
         {num("body_id", "The id of the body to scale."),
          num("factor", "Scale factor, e.g. 2.0 doubles the size.")}},
        {"boolean_op", "Combine two bodies with a boolean operation.",
         {num("target_body_id", "The body kept after the operation."),
          num("tool_body_id", "The body combined into the target."),
          str("mode", "One of: union, subtract, intersect.")}},
    };
    return kTools;
}

namespace {
const char* typeName(ToolParamType t) {
    return t == ToolParamType::String ? "string" : "number";
}
// Shared by both formatters: the JSON Schema "object" body every provider
// wraps identically (properties + required list), only the outer envelope
// differs (Anthropic's input_schema vs OpenAI's function.parameters).
nlohmann::json paramsToJsonSchema(const std::vector<ToolParam>& params) {
    nlohmann::json properties = nlohmann::json::object();
    nlohmann::json required = nlohmann::json::array();
    for (const auto& p : params) {
        properties[p.name] = {{"type", typeName(p.type)}, {"description", p.description}};
        if (p.required) required.push_back(p.name);
    }
    return {{"type", "object"}, {"properties", properties}, {"required", required}};
}
} // namespace

nlohmann::json toolsToAnthropicJson(const std::vector<ToolDef>& tools) {
    nlohmann::json out = nlohmann::json::array();
    for (const auto& t : tools)
        out.push_back({{"name", t.name},
                       {"description", t.description},
                       {"input_schema", paramsToJsonSchema(t.params)}});
    return out;
}

nlohmann::json toolsToOpenAiJson(const std::vector<ToolDef>& tools) {
    nlohmann::json out = nlohmann::json::array();
    for (const auto& t : tools)
        out.push_back({{"type", "function"},
                       {"function", {{"name", t.name},
                                     {"description", t.description},
                                     {"parameters", paramsToJsonSchema(t.params)}}}});
    return out;
}

} } // namespace materializr::ai
```

- [ ] **Step 5: Run test to verify it passes**

```bash
cmake --build build --target test_ai_tool_schema -j8 && ./build/tests/test_ai_tool_schema
```

Expected: `[ PASSED ] 5 tests.`

- [ ] **Step 6: Commit**

```bash
git add src/ai/AiTypes.h src/ai/AiToolSchema.h src/ai/AiToolSchema.cpp tests/test_ai_tool_schema.cpp tests/CMakeLists.txt
git commit -m "Add the AI Assistant's fixed v1 tool schema

One provider-agnostic definition; each LLM client formats it into its
own wire shape.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 5: `AiToolDispatcher`

**Files:**
- Create: `src/ai/AiToolDispatcher.h`
- Create: `src/ai/AiToolDispatcher.cpp`
- Test: Create `tests/test_ai_tool_dispatcher.cpp`
- Modify: `tests/CMakeLists.txt` (add source + register test executable)

**Interfaces:**
- Consumes: `materializr::ai::ToolCall` (Task 4), `materializr::PluginContext` (existing - `document()`, `history()`).
- Produces: `materializr::ai::ToolResult` (fields: `bool ok`, `std::string message`), `materializr::ai::executeTool(materializr::PluginContext& ctx, const std::string& toolName, const nlohmann::json& args) -> ToolResult`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_ai_tool_dispatcher.cpp`. This tests `executeTool` directly against a `Document`/`History` pair - no `PluginContext`/`Application` needed, since `PluginContext` only wraps references to exactly those two objects plus a few others `executeTool` never touches. Build a minimal-bound context by hand:

```cpp
#include "ai/AiToolDispatcher.h"
#include "core/Document.h"
#include "core/History.h"
#include "plugin/PluginContext.h"

#include <gtest/gtest.h>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

using namespace materializr::ai;
using materializr::PluginContext;

namespace {
// A PluginContext with just enough bound to run executeTool: Document +
// History. The other _bind() parameters aren't touched by any tool.
PluginContext makeCtx(Document& doc, History& hist) {
    PluginContext ctx;
    ctx._bind(&doc, &hist, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    return ctx;
}
double bboxSizeX(Document& doc, int bodyId) {
    Bnd_Box box;
    BRepBndLib::Add(doc.getBody(bodyId), box);
    double x0, y0, z0, x1, y1, z1;
    box.Get(x0, y0, z0, x1, y1, z1);
    return x1 - x0;
}
} // namespace

TEST(AiToolDispatcher, AddBoxCreatesABodyWithTheGivenDimensions) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);

    nlohmann::json args = {{"width", 20.0}, {"height", 15.0}, {"depth", 10.0}};
    ToolResult r = executeTool(ctx, "add_box", args);

    ASSERT_TRUE(r.ok) << r.message;
    ASSERT_EQ(doc.getAllBodyIds().size(), 1u);
    int id = doc.getAllBodyIds().front();
    EXPECT_NEAR(bboxSizeX(doc, id), 20.0, 1e-6);
}

TEST(AiToolDispatcher, AddBoxDefaultsPositionToTheOrigin) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);

    nlohmann::json args = {{"width", 10.0}, {"height", 10.0}, {"depth", 10.0}};
    ToolResult r = executeTool(ctx, "add_box", args);
    ASSERT_TRUE(r.ok) << r.message;

    Bnd_Box box;
    BRepBndLib::Add(doc.getBody(doc.getAllBodyIds().front()), box);
    double x0, y0, z0, x1, y1, z1;
    box.Get(x0, y0, z0, x1, y1, z1);
    EXPECT_NEAR(x0, 0.0, 1e-6);
    EXPECT_NEAR(y0, 0.0, 1e-6);
    EXPECT_NEAR(z0, 0.0, 1e-6);
}

TEST(AiToolDispatcher, RejectsANonPositiveBoxDimensionWithoutTouchingTheDocument) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);

    nlohmann::json args = {{"width", -5.0}, {"height", 10.0}, {"depth", 10.0}};
    ToolResult r = executeTool(ctx, "add_box", args);

    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.message.empty());
    EXPECT_TRUE(doc.getAllBodyIds().empty())
        << "a rejected tool call must not create a body";
}

TEST(AiToolDispatcher, MoveBodyRejectsAnUnknownBodyId) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);

    nlohmann::json args = {{"body_id", 999}, {"dx", 1.0}, {"dy", 0.0}, {"dz", 0.0}};
    ToolResult r = executeTool(ctx, "move_body", args);
    EXPECT_FALSE(r.ok);
}

TEST(AiToolDispatcher, BooleanOpRejectsAnUnknownMode) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);

    ToolResult a = executeTool(ctx, "add_box", {{"width", 10.0}, {"height", 10.0}, {"depth", 10.0}});
    ToolResult b = executeTool(ctx, "add_box", {{"width", 5.0}, {"height", 5.0}, {"depth", 5.0}});
    ASSERT_TRUE(a.ok);
    ASSERT_TRUE(b.ok);
    auto ids = doc.getAllBodyIds();
    ASSERT_EQ(ids.size(), 2u);

    nlohmann::json args = {{"target_body_id", ids[0]}, {"tool_body_id", ids[1]}, {"mode", "explode"}};
    ToolResult r = executeTool(ctx, "boolean_op", args);
    EXPECT_FALSE(r.ok);
}

TEST(AiToolDispatcher, BooleanOpUnionMergesTwoBodiesIntoOne) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);

    ToolResult a = executeTool(ctx, "add_box", {{"width", 10.0}, {"height", 10.0}, {"depth", 10.0}});
    ToolResult b = executeTool(ctx, "add_box",
        {{"width", 10.0}, {"height", 10.0}, {"depth", 10.0}, {"x", 5.0}});
    ASSERT_TRUE(a.ok);
    ASSERT_TRUE(b.ok);
    auto ids = doc.getAllBodyIds();
    ASSERT_EQ(ids.size(), 2u);

    ToolResult r = executeTool(ctx, "boolean_op",
        {{"target_body_id", ids[0]}, {"tool_body_id", ids[1]}, {"mode", "union"}});
    EXPECT_TRUE(r.ok) << r.message;
    EXPECT_EQ(doc.getAllBodyIds().size(), 1u)
        << "the tool body must be consumed by a union";
}

TEST(AiToolDispatcher, UnknownToolNameIsRejected) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);
    ToolResult r = executeTool(ctx, "delete_universe", {});
    EXPECT_FALSE(r.ok);
}
```

Register in `tests/CMakeLists.txt`:

```cmake
add_executable(test_ai_tool_dispatcher test_ai_tool_dispatcher.cpp
    TestSignalInit.cpp)
target_link_libraries(test_ai_tool_dispatcher PRIVATE materializr_core gtest gtest_main)
add_test(NAME test_ai_tool_dispatcher COMMAND test_ai_tool_dispatcher)
```

Add `${CMAKE_SOURCE_DIR}/src/ai/AiToolDispatcher.cpp` to `materializr_core`'s source list.

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target test_ai_tool_dispatcher -j8
```

Expected: FAIL - `ai/AiToolDispatcher.h` does not exist. (If `PluginContext::_bind` rejects an 8-argument call because Task 3 hasn't landed yet in this exact worktree, land Task 3 first - this task depends on it.)

- [ ] **Step 3: Write the dispatcher**

Create `src/ai/AiToolDispatcher.h`:

```cpp
#pragma once
#include "AiTypes.h"

namespace materializr { class PluginContext; }

namespace materializr { namespace ai {

struct ToolResult {
    bool ok = false;
    // Doubles as the human-readable scrollback line AND the text fed back
    // to the LLM as this tool's result - see AiSessionController (Task 8).
    std::string message;
};

// Validates args first (no document mutation on any validation failure),
// then builds the matching concrete Operation and pushes it through
// ctx.history().pushOperation() - the exact same imperative shape every
// existing interactive op-commit already uses.
ToolResult executeTool(materializr::PluginContext& ctx, const std::string& toolName,
                       const nlohmann::json& args);

} } // namespace materializr::ai
```

Create `src/ai/AiToolDispatcher.cpp`:

```cpp
#include "AiToolDispatcher.h"
#include "../plugin/PluginContext.h"
#include "../core/Document.h"
#include "../core/History.h"
#include "../modeling/PrimitiveOp.h"
#include "../modeling/TransformOp.h"
#include "../modeling/BooleanOp.h"

namespace materializr { namespace ai {

namespace {

// nlohmann::json::value(key, default) already does exactly what an optional
// numeric arg needs; this wraps the REQUIRED case so a missing key is a
// deliberate error rather than silently defaulting to 0.
bool requireNumber(const nlohmann::json& args, const char* key, double& out,
                   std::string& err) {
    if (!args.contains(key) || !args[key].is_number()) {
        err = std::string("missing or non-numeric required argument '") + key + "'";
        return false;
    }
    out = args[key].get<double>();
    return true;
}
bool requirePositive(const nlohmann::json& args, const char* key, double& out,
                     std::string& err) {
    if (!requireNumber(args, key, out, err)) return false;
    if (out <= 0.0) {
        err = std::string("'") + key + "' must be positive, got " + std::to_string(out);
        return false;
    }
    return true;
}
bool requireBodyId(Document& doc, const nlohmann::json& args, const char* key,
                   int& out, std::string& err) {
    double raw;
    if (!requireNumber(args, key, raw, err)) return false;
    out = static_cast<int>(raw);
    for (int id : doc.getAllBodyIds()) if (id == out) return true;
    err = std::string("no body with id ") + std::to_string(out);
    return false;
}
double optNumber(const nlohmann::json& args, const char* key, double fallback) {
    if (args.contains(key) && args[key].is_number()) return args[key].get<double>();
    return fallback;
}

ToolResult addPrimitive(PluginContext& ctx, PrimitiveOp::Kind kind,
                        const nlohmann::json& args) {
    std::string err;
    auto op = std::make_unique<PrimitiveOp>();
    op->setKind(kind);
    switch (kind) {
        case PrimitiveOp::Kind::Box: {
            double w, h, d;
            if (!requirePositive(args, "width", w, err) ||
                !requirePositive(args, "height", h, err) ||
                !requirePositive(args, "depth", d, err))
                return {false, err};
            op->setBoxExtents(w, h, d);
            break;
        }
        case PrimitiveOp::Kind::Cylinder: {
            double r, h;
            if (!requirePositive(args, "radius", r, err) ||
                !requirePositive(args, "height", h, err))
                return {false, err};
            op->setRadius(r);
            op->setHeight(h);
            break;
        }
        case PrimitiveOp::Kind::Sphere: {
            double r;
            if (!requirePositive(args, "radius", r, err)) return {false, err};
            op->setRadius(r);
            break;
        }
        case PrimitiveOp::Kind::Cone: {
            double br, tr, h;
            if (!requirePositive(args, "bottom_radius", br, err) ||
                !requireNumber(args, "top_radius", tr, err) ||
                !requirePositive(args, "height", h, err))
                return {false, err};
            if (tr < 0.0) return {false, "'top_radius' must not be negative"};
            op->setRadius(br);
            op->setTopRadius(tr);
            op->setHeight(h);
            break;
        }
        case PrimitiveOp::Kind::Torus: {
            double major, minor;
            if (!requirePositive(args, "major_radius", major, err) ||
                !requirePositive(args, "minor_radius", minor, err))
                return {false, err};
            op->setRadius(major);
            op->setMinorRadius(minor);
            break;
        }
    }
    op->setOrigin(optNumber(args, "x", 0.0), optNumber(args, "y", 0.0),
                 optNumber(args, "z", 0.0));
    if (!ctx.history().pushOperation(std::move(op), ctx.document()))
        return {false, "the operation failed to execute"};
    int newId = ctx.document().getAllBodyIds().back();
    return {true, "Created body " + std::to_string(newId)};
}

ToolResult moveBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    double dx, dy, dz;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err) ||
        !requireNumber(args, "dx", dx, err) ||
        !requireNumber(args, "dy", dy, err) ||
        !requireNumber(args, "dz", dz, err))
        return {false, err};
    auto op = std::make_unique<TransformOp>();
    op->setBodyId(bodyId);
    op->setType(TransformType::Translate);
    op->setTranslation(dx, dy, dz);
    if (!ctx.history().pushOperation(std::move(op), ctx.document()))
        return {false, "the operation failed to execute"};
    return {true, "Moved body " + std::to_string(bodyId)};
}

ToolResult rotateBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    double ax, ay, az, angle;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err) ||
        !requireNumber(args, "axis_x", ax, err) ||
        !requireNumber(args, "axis_y", ay, err) ||
        !requireNumber(args, "axis_z", az, err) ||
        !requireNumber(args, "angle_degrees", angle, err))
        return {false, err};
    if (ax == 0.0 && ay == 0.0 && az == 0.0)
        return {false, "the rotation axis must not be the zero vector"};
    auto op = std::make_unique<TransformOp>();
    op->setBodyId(bodyId);
    op->setType(TransformType::Rotate);
    op->setRotation(ax, ay, az, angle);
    if (!ctx.history().pushOperation(std::move(op), ctx.document()))
        return {false, "the operation failed to execute"};
    return {true, "Rotated body " + std::to_string(bodyId)};
}

ToolResult scaleBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    double factor;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err) ||
        !requirePositive(args, "factor", factor, err))
        return {false, err};
    auto op = std::make_unique<TransformOp>();
    op->setBodyId(bodyId);
    op->setType(TransformType::Scale);
    op->setScale(factor);
    if (!ctx.history().pushOperation(std::move(op), ctx.document()))
        return {false, "the operation failed to execute"};
    return {true, "Scaled body " + std::to_string(bodyId)};
}

ToolResult booleanOp(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int targetId, toolId;
    if (!requireBodyId(ctx.document(), args, "target_body_id", targetId, err) ||
        !requireBodyId(ctx.document(), args, "tool_body_id", toolId, err))
        return {false, err};
    if (targetId == toolId)
        return {false, "target_body_id and tool_body_id must be different bodies"};
    if (!args.contains("mode") || !args["mode"].is_string())
        return {false, "missing required argument 'mode'"};
    std::string modeStr = args["mode"].get<std::string>();
    BooleanMode mode;
    if (modeStr == "union") mode = BooleanMode::Union;
    else if (modeStr == "subtract") mode = BooleanMode::Subtract;
    else if (modeStr == "intersect") mode = BooleanMode::Intersect;
    else return {false, "'mode' must be one of: union, subtract, intersect"};

    auto op = std::make_unique<BooleanOp>();
    op->setTargetBodyId(targetId);
    op->setToolBodyId(toolId);
    op->setMode(mode);
    if (!ctx.history().pushOperation(std::move(op), ctx.document()))
        return {false, "the operation failed to execute"};
    return {true, "Combined bodies " + std::to_string(targetId) + " and " +
                  std::to_string(toolId) + " (" + modeStr + ")"};
}

} // namespace

ToolResult executeTool(PluginContext& ctx, const std::string& toolName,
                       const nlohmann::json& args) {
    if (toolName == "add_box") return addPrimitive(ctx, PrimitiveOp::Kind::Box, args);
    if (toolName == "add_cylinder") return addPrimitive(ctx, PrimitiveOp::Kind::Cylinder, args);
    if (toolName == "add_sphere") return addPrimitive(ctx, PrimitiveOp::Kind::Sphere, args);
    if (toolName == "add_cone") return addPrimitive(ctx, PrimitiveOp::Kind::Cone, args);
    if (toolName == "add_torus") return addPrimitive(ctx, PrimitiveOp::Kind::Torus, args);
    if (toolName == "move_body") return moveBody(ctx, args);
    if (toolName == "rotate_body") return rotateBody(ctx, args);
    if (toolName == "scale_body") return scaleBody(ctx, args);
    if (toolName == "boolean_op") return booleanOp(ctx, args);
    return {false, "unknown tool '" + toolName + "'"};
}

} } // namespace materializr::ai
```

- [ ] **Step 4: Run test to verify it passes**

```bash
cmake --build build --target test_ai_tool_dispatcher -j8 && ./build/tests/test_ai_tool_dispatcher
```

Expected: `[ PASSED ] 7 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/ai/AiToolDispatcher.h src/ai/AiToolDispatcher.cpp tests/test_ai_tool_dispatcher.cpp tests/CMakeLists.txt
git commit -m "Add AiToolDispatcher: tool call -> Operation -> pushOperation

Pure logic, no network - validates args before touching the document,
so a rejected call never creates a stray body.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 6: `AnthropicClient`

**Files:**
- Create: `src/ai/LlmClient.h`
- Create: `src/ai/AnthropicClient.h`
- Create: `src/ai/AnthropicClient.cpp`
- Test: Create `tests/test_ai_anthropic_client.cpp`
- Modify: `tests/CMakeLists.txt` (add source + register test executable)

**Interfaces:**
- Consumes: `materializr::ai::ChatMessage`, `materializr::ai::ToolDef`, `materializr::ai::LlmTurnResult` (Task 4).
- Produces: `materializr::ai::LlmClient` (abstract; one virtual `sendTurn`), `materializr::ai::AnthropicClient` (constructed with `apiKey`, `model`), plus two functions later tasks and tests call directly: `AnthropicClient::buildRequestBody(...)` and `AnthropicClient::parseResponse(...)` (both `static`, pure).

- [ ] **Step 1: Write the failing test**

Create `tests/test_ai_anthropic_client.cpp`. These tests call the two pure static functions directly with hand-built JSON - no network, no real API key.

```cpp
#include "ai/AnthropicClient.h"

#include <gtest/gtest.h>

using namespace materializr::ai;

TEST(AnthropicClient, BuildRequestBodyIncludesModelMessagesAndTools) {
    std::vector<ChatMessage> messages = {{ChatRole::User, "make a box", ""}};
    nlohmann::json body = AnthropicClient::buildRequestBody(
        messages, allTools(), "claude-sonnet-4-5");

    EXPECT_EQ(body["model"], "claude-sonnet-4-5");
    ASSERT_TRUE(body["messages"].is_array());
    ASSERT_EQ(body["messages"].size(), 1u);
    EXPECT_EQ(body["messages"][0]["role"], "user");
    ASSERT_TRUE(body["tools"].is_array());
    EXPECT_EQ(body["tools"].size(), allTools().size());
}

TEST(AnthropicClient, BuildRequestBodyMapsToolResultMessagesToUserToolResultBlocks) {
    // Anthropic has no separate "tool" role - a tool result rides inside a
    // user-role message as a tool_result content block.
    std::vector<ChatMessage> messages = {
        {ChatRole::User, "make a box", ""},
        {ChatRole::Assistant, "", ""}, // the tool_use turn itself isn't replayed here
        {ChatRole::ToolResult, "Created body 1", "call_abc"},
    };
    nlohmann::json body = AnthropicClient::buildRequestBody(messages, {}, "claude-sonnet-4-5");
    const auto& last = body["messages"].back();
    EXPECT_EQ(last["role"], "user");
    ASSERT_EQ(last["content"].size(), 1u);
    EXPECT_EQ(last["content"][0]["type"], "tool_result");
    EXPECT_EQ(last["content"][0]["tool_use_id"], "call_abc");
    EXPECT_EQ(last["content"][0]["content"], "Created body 1");
}

TEST(AnthropicClient, ParseResponseExtractsFinalTextWhenNoToolUse) {
    nlohmann::json response = {
        {"content", {{{"type", "text"}, {"text", "Done!"}}}},
    };
    LlmTurnResult r = AnthropicClient::parseResponse(response, 200);
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.finalText, "Done!");
    EXPECT_TRUE(r.toolCalls.empty());
}

TEST(AnthropicClient, ParseResponseExtractsToolUseBlocks) {
    nlohmann::json response = {
        {"content", {
            {{"type", "text"}, {"text", "Sure, one moment."}},
            {{"type", "tool_use"}, {"id", "call_1"}, {"name", "add_box"},
             {"input", {{"width", 10}, {"height", 10}, {"depth", 10}}}},
        }},
    };
    LlmTurnResult r = AnthropicClient::parseResponse(response, 200);
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.toolCalls.size(), 1u);
    EXPECT_EQ(r.toolCalls[0].id, "call_1");
    EXPECT_EQ(r.toolCalls[0].name, "add_box");
    EXPECT_EQ(r.toolCalls[0].args["width"], 10);
}

TEST(AnthropicClient, ParseResponseSurfacesAnHttpErrorStatus) {
    nlohmann::json response = {{"error", {{"message", "invalid x-api-key"}}}};
    LlmTurnResult r = AnthropicClient::parseResponse(response, 401);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.error.find("invalid x-api-key"), std::string::npos);
}

TEST(AnthropicClient, ParseResponseHandlesUnparseableJsonGracefully) {
    LlmTurnResult r = AnthropicClient::parseResponseFromRawBody("not json at all", 200);
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}
```

Register in `tests/CMakeLists.txt`:

```cmake
add_executable(test_ai_anthropic_client test_ai_anthropic_client.cpp
    TestSignalInit.cpp)
target_link_libraries(test_ai_anthropic_client PRIVATE materializr_core gtest gtest_main)
add_test(NAME test_ai_anthropic_client COMMAND test_ai_anthropic_client)
```

Add `${CMAKE_SOURCE_DIR}/src/ai/AnthropicClient.cpp` to `materializr_core`'s source list. `materializr_core` must also link `CURL::libcurl` (it currently does not need to, and does not) - add:

```cmake
find_package(CURL REQUIRED)
target_link_libraries(materializr_core PUBLIC CURL::libcurl)
```

right after the existing `target_link_libraries(materializr_core PUBLIC ...)` block in `tests/CMakeLists.txt`.

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target test_ai_anthropic_client -j8
```

Expected: FAIL - `ai/AnthropicClient.h` does not exist.

- [ ] **Step 3: Write the abstract client interface**

Create `src/ai/LlmClient.h`:

```cpp
#pragma once
#include "AiTypes.h"

namespace materializr { namespace ai {

class LlmClient {
public:
    virtual ~LlmClient() = default;
    // Blocking. Called ONLY from a background worker thread (see
    // AiSessionController, Task 8) - never from the main/render thread.
    virtual LlmTurnResult sendTurn(const std::vector<ChatMessage>& messages,
                                   const std::vector<ToolDef>& tools) = 0;
};

} } // namespace materializr::ai
```

- [ ] **Step 4: Write `AnthropicClient`**

Create `src/ai/AnthropicClient.h`:

```cpp
#pragma once
#include "LlmClient.h"
#include "AiToolSchema.h"

namespace materializr { namespace ai {

class AnthropicClient : public LlmClient {
public:
    AnthropicClient(std::string apiKey, std::string model)
        : m_apiKey(std::move(apiKey)), m_model(std::move(model)) {}

    LlmTurnResult sendTurn(const std::vector<ChatMessage>& messages,
                          const std::vector<ToolDef>& tools) override;

    // Pure, network-free - directly unit-testable.
    static nlohmann::json buildRequestBody(const std::vector<ChatMessage>& messages,
                                           const std::vector<ToolDef>& tools,
                                           const std::string& model);
    static LlmTurnResult parseResponse(const nlohmann::json& body, long httpStatus);
    // Convenience for a raw response string that might not even be valid
    // JSON (a proxy error page, a truncated connection, ...).
    static LlmTurnResult parseResponseFromRawBody(const std::string& rawBody,
                                                  long httpStatus);

private:
    std::string m_apiKey;
    std::string m_model;
};

} } // namespace materializr::ai
```

Create `src/ai/AnthropicClient.cpp`:

```cpp
#include "AnthropicClient.h"

#include <curl/curl.h>

namespace materializr { namespace ai {

namespace {
const char* roleName(ChatRole r) {
    // Anthropic has exactly two roles on the wire; a ToolResult message
    // rides inside a "user" message as a tool_result content block (see
    // buildRequestBody below), and an Assistant message with no text (the
    // tool_use turn itself) is skipped entirely - the tool_use block that
    // produced it lived only in the RESPONSE this client already returned
    // to the caller, so there is nothing to replay here for v1's stateless
    // (each turn rebuilds the whole request) client.
    return r == ChatRole::User ? "user" : "assistant";
}
size_t writeToString(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    std::string* out = static_cast<std::string*>(userp);
    const size_t kMaxResponse = 4u * 1024 * 1024;
    if (out->size() + total > kMaxResponse) return 0;
    out->append(static_cast<char*>(contents), total);
    return total;
}
} // namespace

nlohmann::json AnthropicClient::buildRequestBody(const std::vector<ChatMessage>& messages,
                                                 const std::vector<ToolDef>& tools,
                                                 const std::string& model) {
    nlohmann::json out;
    out["model"] = model;
    out["max_tokens"] = 4096;
    nlohmann::json msgs = nlohmann::json::array();
    for (const auto& m : messages) {
        if (m.role == ChatRole::Assistant && m.text.empty()) continue; // see roleName
        if (m.role == ChatRole::ToolResult) {
            msgs.push_back({{"role", "user"},
                            {"content", {{{"type", "tool_result"},
                                          {"tool_use_id", m.toolCallId},
                                          {"content", m.text}}}}});
            continue;
        }
        msgs.push_back({{"role", roleName(m.role)}, {"content", m.text}});
    }
    out["messages"] = msgs;
    if (!tools.empty()) out["tools"] = toolsToAnthropicJson(tools);
    return out;
}

LlmTurnResult AnthropicClient::parseResponse(const nlohmann::json& body, long httpStatus) {
    LlmTurnResult r;
    if (httpStatus < 200 || httpStatus >= 300) {
        r.ok = false;
        r.error = body.contains("error") && body["error"].contains("message")
                      ? body["error"]["message"].get<std::string>()
                      : ("Anthropic API returned HTTP " + std::to_string(httpStatus));
        return r;
    }
    if (!body.contains("content") || !body["content"].is_array()) {
        r.ok = false;
        r.error = "Anthropic response had no 'content' array";
        return r;
    }
    std::string text;
    for (const auto& block : body["content"]) {
        if (!block.contains("type")) continue;
        if (block["type"] == "text" && block.contains("text"))
            text += block["text"].get<std::string>();
        else if (block["type"] == "tool_use") {
            ToolCall call;
            call.id = block.value("id", "");
            call.name = block.value("name", "");
            call.args = block.value("input", nlohmann::json::object());
            r.toolCalls.push_back(std::move(call));
        }
    }
    r.ok = true;
    if (r.toolCalls.empty()) r.finalText = text;
    return r;
}

LlmTurnResult AnthropicClient::parseResponseFromRawBody(const std::string& rawBody,
                                                        long httpStatus) {
    nlohmann::json parsed;
    try {
        parsed = nlohmann::json::parse(rawBody);
    } catch (const nlohmann::json::parse_error&) {
        LlmTurnResult r;
        r.ok = false;
        r.error = "Anthropic returned a response that wasn't valid JSON "
                  "(HTTP " + std::to_string(httpStatus) + ")";
        return r;
    }
    return parseResponse(parsed, httpStatus);
}

LlmTurnResult AnthropicClient::sendTurn(const std::vector<ChatMessage>& messages,
                                       const std::vector<ToolDef>& tools) {
    nlohmann::json requestBody = buildRequestBody(messages, tools, m_model);
    std::string requestStr = requestBody.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        LlmTurnResult r;
        r.ok = false;
        r.error = "Failed to initialise libcurl.";
        return r;
    }

    std::string responseBody;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("x-api-key: " + m_apiKey).c_str());
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestStr.c_str());
#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
#endif
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode code = curl_easy_perform(curl);
    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        LlmTurnResult r;
        r.ok = false;
        r.error = curl_easy_strerror(code);
        return r;
    }
    return parseResponseFromRawBody(responseBody, httpStatus);
}

} } // namespace materializr::ai
```

- [ ] **Step 5: Run test to verify it passes**

```bash
cmake --build build --target test_ai_anthropic_client -j8 && ./build/tests/test_ai_anthropic_client
```

Expected: `[ PASSED ] 6 tests.`

- [ ] **Step 6: Commit**

```bash
git add src/ai/LlmClient.h src/ai/AnthropicClient.h src/ai/AnthropicClient.cpp tests/test_ai_anthropic_client.cpp tests/CMakeLists.txt
git commit -m "Add AnthropicClient (Messages API with tool use)

Request-building and response-parsing are pure functions, tested
against hand-built JSON fixtures - no test hits the real API.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 7: `OpenAiCompatibleClient`

**Files:**
- Create: `src/ai/OpenAiCompatibleClient.h`
- Create: `src/ai/OpenAiCompatibleClient.cpp`
- Test: Create `tests/test_ai_openai_client.cpp`
- Modify: `tests/CMakeLists.txt` (add source + register test executable)

**Interfaces:**
- Consumes: same as Task 6.
- Produces: `materializr::ai::OpenAiCompatibleClient` (constructed with `apiKey`, `baseUrl`, `model`), `OpenAiCompatibleClient::buildRequestBody(...)`, `OpenAiCompatibleClient::parseResponse(...)` (both `static`, pure) - mirrors `AnthropicClient`'s shape exactly, different wire format.

- [ ] **Step 1: Write the failing test**

Create `tests/test_ai_openai_client.cpp`:

```cpp
#include "ai/OpenAiCompatibleClient.h"

#include <gtest/gtest.h>

using namespace materializr::ai;

TEST(OpenAiCompatibleClient, BuildRequestBodyIncludesModelMessagesAndTools) {
    std::vector<ChatMessage> messages = {{ChatRole::User, "make a box", ""}};
    nlohmann::json body = OpenAiCompatibleClient::buildRequestBody(
        messages, allTools(), "gpt-4o");

    EXPECT_EQ(body["model"], "gpt-4o");
    ASSERT_EQ(body["messages"].size(), 1u);
    EXPECT_EQ(body["messages"][0]["role"], "user");
    ASSERT_TRUE(body["tools"].is_array());
    EXPECT_EQ(body["tools"].size(), allTools().size());
}

TEST(OpenAiCompatibleClient, BuildRequestBodyMapsToolResultToARealToolRole) {
    // Unlike Anthropic, OpenAI's shape HAS a dedicated "tool" role.
    std::vector<ChatMessage> messages = {
        {ChatRole::ToolResult, "Created body 1", "call_abc"},
    };
    nlohmann::json body = OpenAiCompatibleClient::buildRequestBody(messages, {}, "gpt-4o");
    const auto& last = body["messages"].back();
    EXPECT_EQ(last["role"], "tool");
    EXPECT_EQ(last["tool_call_id"], "call_abc");
    EXPECT_EQ(last["content"], "Created body 1");
}

TEST(OpenAiCompatibleClient, ParseResponseExtractsFinalTextWhenNoToolCalls) {
    nlohmann::json response = {
        {"choices", {{{"message", {{"role", "assistant"}, {"content", "Done!"}}}}}},
    };
    LlmTurnResult r = OpenAiCompatibleClient::parseResponse(response, 200);
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.finalText, "Done!");
    EXPECT_TRUE(r.toolCalls.empty());
}

TEST(OpenAiCompatibleClient, ParseResponseExtractsFunctionToolCalls) {
    nlohmann::json response = {{"choices", {{{"message", {
        {"role", "assistant"},
        {"content", nullptr},
        {"tool_calls", {{
            {"id", "call_1"},
            {"type", "function"},
            {"function", {{"name", "add_box"},
                         {"arguments", "{\"width\":10,\"height\":10,\"depth\":10}"}}},
        }}},
    }}}}}};
    LlmTurnResult r = OpenAiCompatibleClient::parseResponse(response, 200);
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.toolCalls.size(), 1u);
    EXPECT_EQ(r.toolCalls[0].id, "call_1");
    EXPECT_EQ(r.toolCalls[0].name, "add_box");
    EXPECT_EQ(r.toolCalls[0].args["width"], 10);
}

TEST(OpenAiCompatibleClient, ParseResponseSurfacesAnHttpErrorStatus) {
    nlohmann::json response = {{"error", {{"message", "model not found"}}}};
    LlmTurnResult r = OpenAiCompatibleClient::parseResponse(response, 404);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.error.find("model not found"), std::string::npos);
}

TEST(OpenAiCompatibleClient, ParseResponseHandlesMalformedFunctionArgumentsGracefully) {
    // A local model (Ollama/LM Studio) can emit non-JSON "arguments" - must
    // not crash, must surface a clear tool-level error instead.
    nlohmann::json response = {{"choices", {{{"message", {
        {"role", "assistant"},
        {"tool_calls", {{
            {"id", "call_1"},
            {"function", {{"name", "add_box"}, {"arguments", "not json"}}},
        }}},
    }}}}}};
    LlmTurnResult r = OpenAiCompatibleClient::parseResponse(response, 200);
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.toolCalls.size(), 1u);
    EXPECT_TRUE(r.toolCalls[0].args.is_object());
    EXPECT_TRUE(r.toolCalls[0].args.empty())
        << "unparseable arguments must fall back to an empty object, not crash";
}
```

Register in `tests/CMakeLists.txt`:

```cmake
add_executable(test_ai_openai_client test_ai_openai_client.cpp
    TestSignalInit.cpp)
target_link_libraries(test_ai_openai_client PRIVATE materializr_core gtest gtest_main)
add_test(NAME test_ai_openai_client COMMAND test_ai_openai_client)
```

Add `${CMAKE_SOURCE_DIR}/src/ai/OpenAiCompatibleClient.cpp` to `materializr_core`'s source list.

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target test_ai_openai_client -j8
```

Expected: FAIL - `ai/OpenAiCompatibleClient.h` does not exist.

- [ ] **Step 3: Write `OpenAiCompatibleClient`**

Create `src/ai/OpenAiCompatibleClient.h`:

```cpp
#pragma once
#include "LlmClient.h"
#include "AiToolSchema.h"

namespace materializr { namespace ai {

class OpenAiCompatibleClient : public LlmClient {
public:
    OpenAiCompatibleClient(std::string apiKey, std::string baseUrl, std::string model)
        : m_apiKey(std::move(apiKey)), m_baseUrl(std::move(baseUrl)),
          m_model(std::move(model)) {}

    LlmTurnResult sendTurn(const std::vector<ChatMessage>& messages,
                          const std::vector<ToolDef>& tools) override;

    static nlohmann::json buildRequestBody(const std::vector<ChatMessage>& messages,
                                           const std::vector<ToolDef>& tools,
                                           const std::string& model);
    static LlmTurnResult parseResponse(const nlohmann::json& body, long httpStatus);
    static LlmTurnResult parseResponseFromRawBody(const std::string& rawBody,
                                                  long httpStatus);

private:
    std::string m_apiKey;
    std::string m_baseUrl;
    std::string m_model;
};

} } // namespace materializr::ai
```

Create `src/ai/OpenAiCompatibleClient.cpp`:

```cpp
#include "OpenAiCompatibleClient.h"

#include <curl/curl.h>

namespace materializr { namespace ai {

namespace {
size_t writeToString(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    std::string* out = static_cast<std::string*>(userp);
    const size_t kMaxResponse = 4u * 1024 * 1024;
    if (out->size() + total > kMaxResponse) return 0;
    out->append(static_cast<char*>(contents), total);
    return total;
}
} // namespace

nlohmann::json OpenAiCompatibleClient::buildRequestBody(
        const std::vector<ChatMessage>& messages, const std::vector<ToolDef>& tools,
        const std::string& model) {
    nlohmann::json out;
    out["model"] = model;
    nlohmann::json msgs = nlohmann::json::array();
    for (const auto& m : messages) {
        if (m.role == ChatRole::ToolResult) {
            msgs.push_back({{"role", "tool"}, {"tool_call_id", m.toolCallId},
                            {"content", m.text}});
        } else {
            msgs.push_back({{"role", m.role == ChatRole::User ? "user" : "assistant"},
                            {"content", m.text}});
        }
    }
    out["messages"] = msgs;
    if (!tools.empty()) out["tools"] = toolsToOpenAiJson(tools);
    return out;
}

LlmTurnResult OpenAiCompatibleClient::parseResponse(const nlohmann::json& body,
                                                    long httpStatus) {
    LlmTurnResult r;
    if (httpStatus < 200 || httpStatus >= 300) {
        r.ok = false;
        r.error = body.contains("error") && body["error"].contains("message")
                      ? body["error"]["message"].get<std::string>()
                      : ("Request returned HTTP " + std::to_string(httpStatus));
        return r;
    }
    if (!body.contains("choices") || body["choices"].empty()) {
        r.ok = false;
        r.error = "Response had no 'choices'";
        return r;
    }
    const auto& message = body["choices"][0]["message"];
    r.ok = true;
    if (message.contains("tool_calls") && message["tool_calls"].is_array()) {
        for (const auto& tc : message["tool_calls"]) {
            ToolCall call;
            call.id = tc.value("id", "");
            const auto& fn = tc["function"];
            call.name = fn.value("name", "");
            std::string argsStr = fn.value("arguments", "{}");
            try {
                call.args = nlohmann::json::parse(argsStr);
            } catch (const nlohmann::json::parse_error&) {
                // A local model emitted non-JSON arguments - fall back to an
                // empty object so AiToolDispatcher's own arg validation
                // reports a clean "missing required argument" instead of
                // this layer crashing on it.
                call.args = nlohmann::json::object();
            }
            r.toolCalls.push_back(std::move(call));
        }
    } else if (message.contains("content") && message["content"].is_string()) {
        r.finalText = message["content"].get<std::string>();
    }
    return r;
}

LlmTurnResult OpenAiCompatibleClient::parseResponseFromRawBody(const std::string& rawBody,
                                                               long httpStatus) {
    nlohmann::json parsed;
    try {
        parsed = nlohmann::json::parse(rawBody);
    } catch (const nlohmann::json::parse_error&) {
        LlmTurnResult r;
        r.ok = false;
        r.error = "The server returned a response that wasn't valid JSON "
                  "(HTTP " + std::to_string(httpStatus) + ")";
        return r;
    }
    return parseResponse(parsed, httpStatus);
}

LlmTurnResult OpenAiCompatibleClient::sendTurn(const std::vector<ChatMessage>& messages,
                                              const std::vector<ToolDef>& tools) {
    nlohmann::json requestBody = buildRequestBody(messages, tools, m_model);
    std::string requestStr = requestBody.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        LlmTurnResult r;
        r.ok = false;
        r.error = "Failed to initialise libcurl.";
        return r;
    }

    std::string responseBody;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    // A dummy/empty key is fine for a local server (Ollama/LM Studio) that
    // doesn't check it - the header is just always sent for consistency.
    headers = curl_slist_append(headers, ("Authorization: Bearer " + m_apiKey).c_str());

    std::string url = m_baseUrl + "/chat/completions";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestStr.c_str());
    // NOT pinned to HTTPS: a local Ollama/LM Studio endpoint is plain HTTP
    // by default (http://localhost:11434), unlike the two fixed cloud
    // endpoints AnthropicClient and the default OpenAI base URL use.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode code = curl_easy_perform(curl);
    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        LlmTurnResult r;
        r.ok = false;
        r.error = curl_easy_strerror(code);
        return r;
    }
    return parseResponseFromRawBody(responseBody, httpStatus);
}

} } // namespace materializr::ai
```

- [ ] **Step 4: Run test to verify it passes**

```bash
cmake --build build --target test_ai_openai_client -j8 && ./build/tests/test_ai_openai_client
```

Expected: `[ PASSED ] 6 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/ai/OpenAiCompatibleClient.h src/ai/OpenAiCompatibleClient.cpp tests/test_ai_openai_client.cpp tests/CMakeLists.txt
git commit -m "Add OpenAiCompatibleClient (covers OpenAI, Ollama, LM Studio)

Same request/response pure-function split as AnthropicClient. Not
HTTPS-pinned, unlike Anthropic's client - a local server's default
endpoint is plain HTTP.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 8: `AiSessionController`

**Files:**
- Create: `src/ai/AiSessionController.h`
- Create: `src/ai/AiSessionController.cpp`
- Test: Create `tests/test_ai_session_controller.cpp`
- Modify: `tests/CMakeLists.txt` (add source + register test executable)

**Interfaces:**
- Consumes: `materializr::ai::LlmClient` (Task 6/7, but tested here against a fake), `materializr::ai::executeTool` (Task 5), `materializr::PluginContext`.
- Produces: `materializr::ai::AiSessionController` with:
  - `void submitPrompt(const std::string& userText)`
  - `void poll(materializr::PluginContext& ctx)` - call once per frame; drives the async turn and any due tool calls.
  - `bool isBusy() const`
  - `struct ScrollbackLine { enum class Kind { User, Assistant, ToolSummary, Error } kind; std::string text; }`
  - `const std::vector<ScrollbackLine>& scrollback() const`

This is the one piece that genuinely needs the `std::async` + `wait_for(0)` polling shape from `UpdateChecker`'s established pattern, but structured so `poll()` (the only frame-driven entry point) is unit-testable by injecting a fake `LlmClient` that returns pre-scripted results synchronously (no real thread, no real network) - the test drives `poll()` in a loop exactly like the real overlay would, but the "async" work completes instantly.

- [ ] **Step 1: Write the failing test**

Create `tests/test_ai_session_controller.cpp`:

```cpp
#include "ai/AiSessionController.h"
#include "core/Document.h"
#include "core/History.h"
#include "plugin/PluginContext.h"

#include <gtest/gtest.h>

using namespace materializr::ai;
using materializr::PluginContext;

namespace {
// A scripted LlmClient: each call to sendTurn returns the next entry in a
// pre-built queue, so the test controls exactly what the "model" does on
// each turn without any real network or thread.
class ScriptedClient : public LlmClient {
public:
    explicit ScriptedClient(std::vector<LlmTurnResult> turns)
        : m_turns(std::move(turns)) {}
    LlmTurnResult sendTurn(const std::vector<ChatMessage>&,
                          const std::vector<ToolDef>&) override {
        if (m_next >= m_turns.size()) {
            LlmTurnResult r;
            r.ok = false;
            r.error = "ScriptedClient ran out of scripted turns";
            return r;
        }
        return m_turns[m_next++];
    }
private:
    std::vector<LlmTurnResult> m_turns;
    size_t m_next = 0;
};

PluginContext makeCtx(Document& doc, History& hist) {
    PluginContext ctx;
    ctx._bind(&doc, &hist, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    return ctx;
}

// Drive poll() until the controller stops being busy or a safety cap of
// iterations is hit (a real hang here is a bug the test must fail on, not
// spin forever).
void pumpUntilIdle(AiSessionController& sess, PluginContext& ctx) {
    for (int i = 0; i < 100 && sess.isBusy(); ++i) sess.poll(ctx);
}

LlmTurnResult finalText(const std::string& text) {
    LlmTurnResult r;
    r.ok = true;
    r.finalText = text;
    return r;
}
LlmTurnResult toolCall(const std::string& id, const std::string& name,
                      nlohmann::json args) {
    LlmTurnResult r;
    r.ok = true;
    ToolCall c{id, name, std::move(args)};
    r.toolCalls.push_back(std::move(c));
    return r;
}
} // namespace

TEST(AiSessionController, AFinalTextTurnEndsTheSessionImmediately) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);
    AiSessionController sess(std::make_unique<ScriptedClient>(
        std::vector<LlmTurnResult>{finalText("All done.")}));

    sess.submitPrompt("say hi");
    pumpUntilIdle(sess, ctx);

    EXPECT_FALSE(sess.isBusy());
    ASSERT_FALSE(sess.scrollback().empty());
    EXPECT_EQ(sess.scrollback().back().text, "All done.");
}

TEST(AiSessionController, AToolCallTurnExecutesItAndContinuesTheLoop) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);
    AiSessionController sess(std::make_unique<ScriptedClient>(
        std::vector<LlmTurnResult>{
            toolCall("call_1", "add_box",
                     {{"width", 10.0}, {"height", 10.0}, {"depth", 10.0}}),
            finalText("Made your box."),
        }));

    sess.submitPrompt("make a box");
    pumpUntilIdle(sess, ctx);

    EXPECT_FALSE(sess.isBusy());
    EXPECT_EQ(doc.getAllBodyIds().size(), 1u)
        << "the tool call must actually have executed against the document";
    EXPECT_EQ(sess.scrollback().back().text, "Made your box.");
}

TEST(AiSessionController, StopsAfterTheStepCapInsteadOfLoopingForever) {
    // Script far more tool-call turns than the cap - the controller must
    // stop on its own rather than exhausting the script or spinning.
    std::vector<LlmTurnResult> turns;
    for (int i = 0; i < 20; ++i)
        turns.push_back(toolCall("call_" + std::to_string(i), "add_box",
                                 {{"width", 1.0}, {"height", 1.0}, {"depth", 1.0}}));
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);
    AiSessionController sess(std::make_unique<ScriptedClient>(turns));

    sess.submitPrompt("keep making boxes");
    pumpUntilIdle(sess, ctx);

    EXPECT_FALSE(sess.isBusy());
    EXPECT_LE(doc.getAllBodyIds().size(), 8u)
        << "the 8-step cap must actually bound how many tool calls run";
}

TEST(AiSessionController, AnInvalidToolCallFeedsTheErrorBackRatherThanStopping) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);
    AiSessionController sess(std::make_unique<ScriptedClient>(
        std::vector<LlmTurnResult>{
            toolCall("call_1", "add_box", {{"width", -5.0}}), // invalid
            finalText("Fixed it."), // the model gets the error and recovers
        }));

    sess.submitPrompt("make a box");
    pumpUntilIdle(sess, ctx);

    EXPECT_FALSE(sess.isBusy());
    EXPECT_TRUE(doc.getAllBodyIds().empty())
        << "the invalid call must not have created a body";
    EXPECT_EQ(sess.scrollback().back().text, "Fixed it.");
}

TEST(AiSessionController, ANetworkFailureEndsTheSessionWithAnErrorLine) {
    Document doc;
    History hist;
    PluginContext ctx = makeCtx(doc, hist);
    LlmTurnResult failure;
    failure.ok = false;
    failure.error = "Connection refused";
    AiSessionController sess(std::make_unique<ScriptedClient>(
        std::vector<LlmTurnResult>{failure}));

    sess.submitPrompt("make a box");
    pumpUntilIdle(sess, ctx);

    EXPECT_FALSE(sess.isBusy());
    bool sawError = false;
    for (const auto& line : sess.scrollback())
        if (line.kind == AiSessionController::ScrollbackLine::Kind::Error &&
            line.text.find("Connection refused") != std::string::npos)
            sawError = true;
    EXPECT_TRUE(sawError);
}
```

Register in `tests/CMakeLists.txt`:

```cmake
add_executable(test_ai_session_controller test_ai_session_controller.cpp
    TestSignalInit.cpp)
target_link_libraries(test_ai_session_controller PRIVATE materializr_core gtest gtest_main)
add_test(NAME test_ai_session_controller COMMAND test_ai_session_controller)
```

Add `${CMAKE_SOURCE_DIR}/src/ai/AiSessionController.cpp` to `materializr_core`'s source list.

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target test_ai_session_controller -j8
```

Expected: FAIL - `ai/AiSessionController.h` does not exist.

- [ ] **Step 3: Write `AiSessionController`**

Create `src/ai/AiSessionController.h`:

```cpp
#pragma once
#include "AiTypes.h"
#include "LlmClient.h"

#include <future>
#include <memory>

namespace materializr { class PluginContext; }

namespace materializr { namespace ai {

// Drives one AI conversation: submitPrompt() starts a turn, poll() (called
// once per frame from the chat overlay's OverlayContribution::render, the
// same way UpdateChecker's result is polled from Application's main loop)
// advances it. Owns the std::future for the in-flight network call - all
// Document/History mutation happens inside poll() on the caller's thread
// (the main thread, in the real app), never inside the async lambda itself.
class AiSessionController {
public:
    struct ScrollbackLine {
        enum class Kind { User, Assistant, ToolSummary, Error };
        Kind kind;
        std::string text;
    };

    explicit AiSessionController(std::unique_ptr<LlmClient> client);

    void submitPrompt(const std::string& userText);
    // Call once per frame. No-op if nothing is in flight.
    void poll(materializr::PluginContext& ctx);
    bool isBusy() const { return m_future.valid(); }
    const std::vector<ScrollbackLine>& scrollback() const { return m_scrollback; }

private:
    void startTurn();

    std::unique_ptr<LlmClient> m_client;
    std::vector<ChatMessage> m_messages;
    std::vector<ScrollbackLine> m_scrollback;
    std::future<LlmTurnResult> m_future;
    int m_stepCount = 0;
    static constexpr int kMaxStepsPerPrompt = 8;
};

} } // namespace materializr::ai
```

Create `src/ai/AiSessionController.cpp`:

```cpp
#include "AiSessionController.h"
#include "AiToolDispatcher.h"
#include "AiToolSchema.h"
#include "../plugin/PluginContext.h"

namespace materializr { namespace ai {

AiSessionController::AiSessionController(std::unique_ptr<LlmClient> client)
    : m_client(std::move(client)) {}

void AiSessionController::submitPrompt(const std::string& userText) {
    if (isBusy()) return; // one turn in flight at a time
    m_messages.push_back({ChatRole::User, userText, ""});
    m_scrollback.push_back({ScrollbackLine::Kind::User, userText});
    m_stepCount = 0;
    startTurn();
}

void AiSessionController::startTurn() {
    // Captured by value: m_client is a pointer the lambda doesn't own the
    // lifetime of, but AiSessionController outlives every turn it starts
    // (poll() always completes a turn before the next submitPrompt can
    // start another - see the isBusy() guard above).
    LlmClient* client = m_client.get();
    std::vector<ChatMessage> messagesCopy = m_messages;
    m_future = std::async(std::launch::async, [client, messagesCopy]() {
        return client->sendTurn(messagesCopy, allTools());
    });
}

void AiSessionController::poll(materializr::PluginContext& ctx) {
    if (!m_future.valid()) return;
    if (m_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;

    LlmTurnResult result = m_future.get(); // future becomes invalid; isBusy() -> false
                                            // again unless we start a new one below

    if (!result.ok) {
        m_scrollback.push_back({ScrollbackLine::Kind::Error,
                                "AI request failed: " + result.error});
        return;
    }
    if (result.toolCalls.empty()) {
        m_scrollback.push_back({ScrollbackLine::Kind::Assistant, result.finalText});
        return;
    }

    for (const auto& call : result.toolCalls) {
        ToolResult toolResult = executeTool(ctx, call.name, call.args);
        m_scrollback.push_back({toolResult.ok ? ScrollbackLine::Kind::ToolSummary
                                              : ScrollbackLine::Kind::Error,
                                "-> " + toolResult.message});
        m_messages.push_back({ChatRole::ToolResult, toolResult.message, call.id});
    }
    ++m_stepCount;
    if (m_stepCount >= kMaxStepsPerPrompt) {
        m_scrollback.push_back({ScrollbackLine::Kind::Error,
                                "Stopped after " + std::to_string(kMaxStepsPerPrompt) +
                                " steps."});
        return;
    }
    startTurn();
}

} } // namespace materializr::ai
```

- [ ] **Step 4: Run test to verify it passes**

```bash
cmake --build build --target test_ai_session_controller -j8 && ./build/tests/test_ai_session_controller
```

Expected: `[ PASSED ] 5 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/ai/AiSessionController.h src/ai/AiSessionController.cpp tests/test_ai_session_controller.cpp tests/CMakeLists.txt
git commit -m "Add AiSessionController: the agentic turn-loop state machine

Tested against a scripted fake LlmClient - no real thread, no real
network, but pumps the same poll() a real per-frame overlay would.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 9: `AiAssistantPlugin` (UI) + Settings dialog tab

**Files:**
- Create: `src/plugins/AiAssistantPlugin.cpp`
- Modify: `src/plugins/ForceLink.cpp` (declare + call `forceLink_AiAssistant()`)
- Modify: `CMakeLists.txt` (add the plugin source to the `materializr` target's source list)
- Modify: `src/app/Application_Dialogs.cpp` (`renderSettings()`: new "AI Assistant" tab)
- Test: none (matches this repo's established convention of not unit-testing plugin ImGui glue - confirmed zero existing tests touch `PluginContext`/`REGISTER_PLUGIN`/ImGui rendering anywhere in this codebase)

**Interfaces:**
- Consumes: `materializr::ai::AiSessionController` (Task 8), `materializr::PluginContext::aiSettings()` (Task 3), `materializr::ai::AnthropicClient` / `OpenAiCompatibleClient` (Tasks 6/7).
- Produces: nothing further downstream - this is the leaf.

- [ ] **Step 1: Write the plugin**

Create `src/plugins/AiAssistantPlugin.cpp`. This owns the overlay's `AiSessionController` in a file-local `static`, matching the "plugins own their render caches in file-local statics" convention `Application.cpp`'s `_bind` comment already documents for exactly this reason.

```cpp
#include "../plugin/PluginMacro.h"
#include "../plugin/PluginContext.h"
#include "../ai/AiSessionController.h"
#include "../ai/AnthropicClient.h"
#include "../ai/OpenAiCompatibleClient.h"
#include "../io/Settings.h"

#include <imgui.h>
#include <memory>

namespace {

using materializr::ai::AiSessionController;
using materializr::ai::AnthropicClient;
using materializr::ai::OpenAiCompatibleClient;
using materializr::ai::LlmClient;

// One conversation for the app's lifetime, matching the design's explicit
// choice that AI chat history is in-memory only and does not persist across
// a restart. Rebuilt whenever the provider/model/key changes so a mid-
// session Settings edit takes effect on the NEXT prompt rather than needing
// a relaunch.
std::unique_ptr<AiSessionController> g_session;
materializr::AiProvider g_sessionProvider;
std::string g_sessionKeyOrUrlFingerprint;

std::string fingerprint(const materializr::AppSettings::AiSettings& s) {
    return s.provider == materializr::AiProvider::Anthropic
               ? s.anthropicApiKey + "|" + s.anthropicModel
               : s.openAiApiKey + "|" + s.openAiBaseUrl + "|" + s.openAiModel;
}

AiSessionController& sessionFor(const materializr::AppSettings::AiSettings& ai) {
    const std::string fp = fingerprint(ai);
    if (!g_session || g_sessionProvider != ai.provider ||
        g_sessionKeyOrUrlFingerprint != fp) {
        std::unique_ptr<LlmClient> client;
        if (ai.provider == materializr::AiProvider::Anthropic)
            client = std::make_unique<AnthropicClient>(ai.anthropicApiKey, ai.anthropicModel);
        else
            client = std::make_unique<OpenAiCompatibleClient>(
                ai.openAiApiKey, ai.openAiBaseUrl, ai.openAiModel);
        g_session = std::make_unique<AiSessionController>(std::move(client));
        g_sessionProvider = ai.provider;
        g_sessionKeyOrUrlFingerprint = fp;
    }
    return *g_session;
}

bool hasApiKeyConfigured(const materializr::AppSettings::AiSettings& ai) {
    return ai.provider == materializr::AiProvider::Anthropic
               ? !ai.anthropicApiKey.empty()
               : true; // a local server (Ollama/LM Studio) often needs no key at all
}

void renderOverlay(materializr::PluginContext& ctx) {
    static bool open = false;
    static char inputBuf[2048] = {};

    if (ImGui::Begin("AI Assistant", &open ? nullptr : nullptr)) {} // placeholder guard below
    ImGui::End();

    if (!ImGui::Begin("AI Assistant")) { ImGui::End(); return; }

    const auto& ai = ctx.aiSettings();
    AiSessionController& session = sessionFor(ai);
    session.poll(ctx);

    ImGui::BeginChild("AiScrollback", ImVec2(0, -60), true);
    for (const auto& line : session.scrollback()) {
        using Kind = AiSessionController::ScrollbackLine::Kind;
        switch (line.kind) {
            case Kind::User:        ImGui::TextWrapped("You: %s", line.text.c_str()); break;
            case Kind::Assistant:   ImGui::TextWrapped("AI: %s", line.text.c_str()); break;
            case Kind::ToolSummary: ImGui::TextWrapped("%s", line.text.c_str()); break;
            case Kind::Error:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::TextWrapped("%s", line.text.c_str());
                ImGui::PopStyleColor();
                break;
        }
    }
    if (session.isBusy()) ImGui::TextDisabled("Thinking...");
    ImGui::EndChild();

    if (!hasApiKeyConfigured(ai)) {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                           "Set up your API key in Settings -> AI Assistant.");
    } else {
        const bool busy = session.isBusy();
        ImGui::BeginDisabled(busy);
        ImGui::InputText("##AiPrompt", inputBuf, sizeof(inputBuf));
        ImGui::SameLine();
        if (ImGui::Button("Send") && inputBuf[0] != '\0') {
            session.submitPrompt(inputBuf);
            inputBuf[0] = '\0';
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
}

} // namespace

REGISTER_PLUGIN(AiAssistant, [](materializr::PluginContext& ctx) {
    ctx.registerCommand({"AI Assistant", "", [](materializr::PluginContext&) {
        // The overlay renders unconditionally below; this command exists so
        // the feature is reachable from the menu/command surface even
        // though it needs no per-click action - matches every other
        // free-floating OverlayContribution's registerCommand-just-for-
        // discoverability pattern.
    }});
    ctx.registerOverlay({"AI Assistant", 100, renderOverlay});
});
```

Note the `ImGui::Begin`/`End` placeholder pair at the top of `renderOverlay` is dead and must be deleted before this step is considered done - see Step 1a below.

- [ ] **Step 1a: Delete the placeholder Begin/End pair**

Remove these two lines from `renderOverlay` (an artifact of drafting, not a real open/close toggle - the real, single `ImGui::Begin("AI Assistant")` a few lines below is the only one needed; a plain `OverlayContribution` window has no built-in close button anyway, matching how `TutorialPlugin`'s existing overlay works):

```cpp
    if (ImGui::Begin("AI Assistant", &open ? nullptr : nullptr)) {} // placeholder guard below
    ImGui::End();
```

Also delete the now-unused `static bool open = false;` line above it.

- [ ] **Step 2: Register the plugin's force-link entry**

In `src/plugins/ForceLink.cpp`, add the declaration in the appropriate phase-numbered block (append a new one, e.g. "Phase 6 plugins" if that's the latest, or add to whatever the last block is):

```cpp
void forceLink_AiAssistant();
```

Find where these declared functions are actually CALLED (the function body further down in the same file) and add:

```cpp
forceLink_AiAssistant();
```

- [ ] **Step 3: Add the plugin source to the build**

In `CMakeLists.txt`, add to the plugin source list (right after the last `src/plugins/*.cpp` entry):

```cmake
    src/plugins/AiAssistantPlugin.cpp
```

Also add the four new `src/ai/*.cpp` files (from Tasks 4-8) to the SAME target's source list, since `materializr` (the app) needs them too, not just `materializr_core` (the test lib does, from earlier tasks - the app target is a separate list in the SAME `CMakeLists.txt`):

```cmake
    src/ai/AiToolSchema.cpp
    src/ai/AiToolDispatcher.cpp
    src/ai/AnthropicClient.cpp
    src/ai/OpenAiCompatibleClient.cpp
    src/ai/AiSessionController.cpp
```

- [ ] **Step 4: Add the Settings dialog tab**

In `src/app/Application_Dialogs.cpp`'s `renderSettings()`, find the `ImGui::BeginTabBar("SettingsTabs")` block and add one more `ImGui::BeginTabItem(...)` / `ImGui::EndTabItem()` pair (following the exact shape every existing tab already uses), anywhere among the existing tabs:

```cpp
if (ImGui::BeginTabItem("AI Assistant")) {
    static char anthropicKeyBuf[256] = {};
    static char anthropicModelBuf[128] = {};
    static char openAiKeyBuf[256] = {};
    static char openAiUrlBuf[256] = {};
    static char openAiModelBuf[128] = {};
    static bool buffersLoaded = false;
    if (!buffersLoaded) {
        std::snprintf(anthropicKeyBuf, sizeof(anthropicKeyBuf), "%s",
                     m_aiSettings.anthropicApiKey.c_str());
        std::snprintf(anthropicModelBuf, sizeof(anthropicModelBuf), "%s",
                     m_aiSettings.anthropicModel.c_str());
        std::snprintf(openAiKeyBuf, sizeof(openAiKeyBuf), "%s",
                     m_aiSettings.openAiApiKey.c_str());
        std::snprintf(openAiUrlBuf, sizeof(openAiUrlBuf), "%s",
                     m_aiSettings.openAiBaseUrl.c_str());
        std::snprintf(openAiModelBuf, sizeof(openAiModelBuf), "%s",
                     m_aiSettings.openAiModel.c_str());
        buffersLoaded = true;
    }

    int providerIdx = m_aiSettings.provider == materializr::AiProvider::Anthropic ? 0 : 1;
    const char* providerNames[] = {"Anthropic", "OpenAI-compatible (OpenAI/Ollama/LM Studio)"};
    if (ImGui::Combo("Provider", &providerIdx, providerNames, 2)) {
        m_aiSettings.provider = providerIdx == 0 ? materializr::AiProvider::Anthropic
                                                  : materializr::AiProvider::OpenAiCompatible;
        changed = true;
    }

    if (m_aiSettings.provider == materializr::AiProvider::Anthropic) {
        if (ImGui::InputText("API Key", anthropicKeyBuf, sizeof(anthropicKeyBuf),
                             ImGuiInputTextFlags_Password)) {
            m_aiSettings.anthropicApiKey = anthropicKeyBuf;
            changed = true;
        }
        if (ImGui::InputText("Model", anthropicModelBuf, sizeof(anthropicModelBuf))) {
            m_aiSettings.anthropicModel = anthropicModelBuf;
            changed = true;
        }
    } else {
        if (ImGui::InputText("API Key", openAiKeyBuf, sizeof(openAiKeyBuf),
                             ImGuiInputTextFlags_Password)) {
            m_aiSettings.openAiApiKey = openAiKeyBuf;
            changed = true;
        }
        if (ImGui::InputText("Base URL", openAiUrlBuf, sizeof(openAiUrlBuf))) {
            m_aiSettings.openAiBaseUrl = openAiUrlBuf;
            changed = true;
        }
        if (ImGui::InputText("Model", openAiModelBuf, sizeof(openAiModelBuf))) {
            m_aiSettings.openAiModel = openAiModelBuf;
            changed = true;
        }
    }
    ImGui::TextWrapped(
        "Point the base URL at http://localhost:11434/v1 for Ollama, or "
        "LM Studio's local server address, to run without any cloud key.");

    // Test Connection: fires one minimal, tool-free request against
    // whatever is CURRENTLY TYPED (not yet saved) so the user gets a
    // yes/no on their setup before ever opening the chat overlay - the
    // whole point of "quick and simple" per the spec.
    static std::future<materializr::ai::LlmTurnResult> testFuture;
    static std::string testResultText;
    static bool testResultIsError = false;
    const bool testBusy = testFuture.valid() &&
        testFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready;
    if (testFuture.valid() &&
        testFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        materializr::ai::LlmTurnResult r = testFuture.get();
        testResultIsError = !r.ok;
        testResultText = r.ok ? "Connection OK." : ("Failed: " + r.error);
    }
    ImGui::BeginDisabled(testBusy);
    if (ImGui::Button("Test Connection")) {
        std::unique_ptr<materializr::ai::LlmClient> client;
        if (m_aiSettings.provider == materializr::AiProvider::Anthropic)
            client = std::make_unique<materializr::ai::AnthropicClient>(
                m_aiSettings.anthropicApiKey, m_aiSettings.anthropicModel);
        else
            client = std::make_unique<materializr::ai::OpenAiCompatibleClient>(
                m_aiSettings.openAiApiKey, m_aiSettings.openAiBaseUrl,
                m_aiSettings.openAiModel);
        testResultText.clear();
        testFuture = std::async(std::launch::async,
            [c = std::shared_ptr<materializr::ai::LlmClient>(std::move(client))]() {
                std::vector<materializr::ai::ChatMessage> msgs = {
                    {materializr::ai::ChatRole::User, "Reply with OK.", ""}};
                return c->sendTurn(msgs, {});
            });
    }
    ImGui::EndDisabled();
    if (testBusy) { ImGui::SameLine(); ImGui::TextDisabled("Testing..."); }
    if (!testResultText.empty()) {
        ImGui::TextColored(testResultIsError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                                             : ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                           "%s", testResultText.c_str());
    }
    ImGui::EndTabItem();
}
```

This needs three more includes at the top of `Application_Dialogs.cpp` (add them next to its existing includes): `#include "../ai/AnthropicClient.h"`, `#include "../ai/OpenAiCompatibleClient.h"`, and `<future>` (likely already included given the file's existing `std::async` usage patterns elsewhere in the app - check before adding a duplicate).

`changed` is the same bool the rest of `renderSettings()` already declares and checks at the end to call `saveAppSettings()` - no new plumbing needed there.

- [ ] **Step 5: Verify it builds**

```bash
cmake --build build --target materializr -j8
```

Expected: builds clean.

- [ ] **Step 6: Manual smoke test**

Launch the built app. Open Settings -> AI Assistant, paste a real Anthropic API key (or point the OpenAI-compatible base URL at a running local Ollama with a tool-calling-capable model, e.g. `llama3.1`). Open the AI Assistant overlay (via its command/menu entry). Type "make a 20mm cube" and Send. Confirm: a box appears in the viewport, Ctrl+Z removes it. Then try "make a 30mm cube with a 10mm hole through the center" and confirm the agentic loop runs more than one tool call (a box, a cylinder, a boolean subtract) before the final text reply.

- [ ] **Step 7: Commit**

```bash
git add src/plugins/AiAssistantPlugin.cpp src/plugins/ForceLink.cpp CMakeLists.txt src/app/Application_Dialogs.cpp
git commit -m "Add the AI Assistant plugin: chat overlay + Settings tab

Wires AiSessionController to a real toolbar/overlay entry point and
lets the user configure provider/key/model without touching a file.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 10: Exclude from the Android build

**Files:**
- Modify: `android/app/jni/src/CMakeLists.txt`
- Modify: `src/plugins/ForceLink.cpp` (guard the one call added in Task 9)
- Test: none (platform-exclusion; verified by reading the resulting source list, and by a real Android build if a worker has the toolchain available)

**Interfaces:** none new - this task only removes reachability, it adds nothing.

- [ ] **Step 1: Exclude the `src/ai/` directory and the plugin file from Android's source glob**

In `android/app/jni/src/CMakeLists.txt`, right after the existing:

```cmake
list(FILTER MZ_SOURCES EXCLUDE REGEX "/UpdateChecker\\.cpp$")   # libcurl (desktop-only)
```

add:

```cmake
# AI Assistant: every backend is a network call, and Android gates loopback
# traffic behind INTERNET the same as any other host - there is no way to
# reach even a local Ollama/LM Studio server without it. Desktop only; see
# docs/superpowers/specs/2026-09-12-ai-assistant-design.md's "Platform Scope"
# section.
list(FILTER MZ_SOURCES EXCLUDE REGEX "/src/ai/")
list(FILTER MZ_SOURCES EXCLUDE REGEX "/AiAssistantPlugin\\.cpp$")
```

- [ ] **Step 2: Guard the force-link call**

In `src/plugins/ForceLink.cpp`, wrap BOTH the declaration and the call added in Task 9:

```cpp
#if !defined(__ANDROID__)
void forceLink_AiAssistant();
#endif
```

and, at the call site:

```cpp
#if !defined(__ANDROID__)
forceLink_AiAssistant();
#endif
```

- [ ] **Step 3: Verify the desktop build still works**

```bash
cmake --build build --target materializr -j8
```

Expected: builds clean (the `#if !defined(__ANDROID__)` guards are no-ops on desktop, since `__ANDROID__` is never defined there).

- [ ] **Step 4: Verify the exclusion, statically**

```bash
cd android/app/jni/src
cmake -P - <<'EOF' 2>/dev/null || true
EOF
```

A full Android Studio/gradle cross-build is the real verification, but is not assumed available in every worker's environment. At minimum, confirm by inspection that the two new `list(FILTER ...)` lines' regexes actually match the files added in Tasks 4-9:

```bash
grep -rl "AiAssistantPlugin\|src/ai/" CMakeLists.txt | true
find src/ai -name '*.cpp'
```

Expected: `find src/ai -name '*.cpp'` lists exactly the five files from Tasks 4-8 (`AiToolSchema.cpp`, `AiToolDispatcher.cpp`, `AnthropicClient.cpp`, `OpenAiCompatibleClient.cpp`, `AiSessionController.cpp`) plus `AiAssistantPlugin.cpp` under `src/plugins/` - confirm each is matched by one of the two new regexes (`/src/ai/` matches the whole directory; `/AiAssistantPlugin\.cpp$` matches the plugin file specifically). If an Android toolchain (Android Studio + NDK, or a CI runner) IS available, additionally run:

```bash
cd android && ./gradlew assembleDebug
```

Expected: succeeds, and its build log contains no reference to any `src/ai/*.cpp` or `AiAssistantPlugin.cpp` object file.

- [ ] **Step 5: Commit**

```bash
git add android/app/jni/src/CMakeLists.txt src/plugins/ForceLink.cpp
git commit -m "Exclude the AI Assistant feature from the Android build

Every backend is a network call; Android gates even loopback traffic
behind INTERNET, which this app deliberately does not request.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 11: Final integration pass

**Files:** none new - this task only runs and verifies.

**Interfaces:** none.

- [ ] **Step 1: Full clean build**

```bash
cmake --build build -j8
```

Expected: `materializr` and every test target build with zero errors.

- [ ] **Step 2: Full test suite**

```bash
cd build && ctest --output-on-failure -j8
```

Expected: every existing suite still passes, plus the six new ones from this plan (`test_ai_settings`, `test_ai_tool_schema`, `test_ai_tool_dispatcher`, `test_ai_anthropic_client`, `test_ai_openai_client`, `test_ai_session_controller`) all green.

- [ ] **Step 3: Style gates**

```bash
cd .. && python3 tools/no_em_dashes.py
python3 tools/no_double_build.py
python3 tools/units_audit.py
grep -rnE '\bfar\b|\bnear\b' src/ai/ src/plugins/AiAssistantPlugin.cpp
```

Expected: all three Python gates report clean; the `grep` for `far`/`near` finds nothing (or only matches inside comments/strings, never an identifier - inspect any hit by hand before treating it as a pass).

- [ ] **Step 4: Re-run the manual smoke test from Task 9, Step 6**

If it wasn't already run at the end of Task 9, run it now: real (or local Ollama) key, "make a 20mm cube," confirm undoable; then the multi-step "cube with a hole" prompt to exercise the agentic loop and a boolean.

- [ ] **Step 5: Update the spec's status (optional, recommended)**

Add a short "Status: Implemented" note at the top of `docs/superpowers/specs/2026-09-12-ai-assistant-design.md`, then commit:

```bash
git add docs/superpowers/specs/2026-09-12-ai-assistant-design.md
git commit -m "Mark AI Assistant spec as implemented

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```
