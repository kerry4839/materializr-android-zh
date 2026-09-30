#include "ui_layout_bridge.h"
#include "../i18n.h"

namespace materializr {

namespace {
std::function<int()>     g_get;
std::function<void(int)> g_set;
std::function<int()>     g_langGet;
std::function<void(int)> g_langSet;
} // namespace

int currentUiLayoutIndex() { return g_get ? g_get() : 0; }

void requestUiLayout(int index) {
    if (g_set && index >= 0 && index <= 2) g_set(index);
}

void bindUiLayoutBridge(std::function<int()> get, std::function<void(int)> set) {
    g_get = std::move(get);
    g_set = std::move(set);
}

int currentLanguageIndex() { return g_langGet ? g_langGet() : -1; }

void requestLanguage(int index) {
    if (index < 0 || index >= languageCount()) return;
    // Switch the live UI first so the caller's own frame is already translated,
    // then persist. setLanguage is cheap (one hash-map rebuild) and needs no
    // font atlas work, so this is safe to call mid-frame.
    setLanguage(static_cast<Lang>(index));
    if (g_langSet) g_langSet(index);
}

void bindLanguageBridge(std::function<int()> get, std::function<void(int)> set) {
    g_langGet = std::move(get);
    g_langSet = std::move(set);
}




} // namespace materializr
