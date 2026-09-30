#pragma once
#include "LlmClient.h"
#include "../io/Settings.h"

#include <future>

namespace materializr { namespace ai {

// Fires one minimal, tool-free request against the given settings to verify
// connectivity/credentials, independent of any live chat session. Owns the
// client-construction decision (which provider class to build) so callers -
// including core app code, e.g. the Settings dialog's "Test Connection"
// button - never need to know about AnthropicClient/OpenAiCompatibleClient
// directly. See CONTRIBUTING.md's plugin-ownership rule.
std::future<LlmTurnResult> testConnection(const AppSettings::AiSettings& settings);

} } // namespace materializr::ai
