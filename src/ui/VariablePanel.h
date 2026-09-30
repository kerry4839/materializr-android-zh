#pragma once
#include <string>

namespace materializr {

class VariableManager;

class VariablePanel {
public:
    VariablePanel();

    void setVariableManager(VariableManager* mgr);

    // Returns true if any variable changed (needs history replay)
    bool render();

private:
    VariableManager* m_manager = nullptr;
    char m_newName[64] = {};
    char m_newExpr[128] = {};
    char m_editBuffers[32][128] = {}; // edit buffers for existing variables
};

} // namespace materializr
