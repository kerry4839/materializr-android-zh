#pragma once

namespace materializr {

class VersionManager;

class VersionPanel {
public:
    VersionPanel();

    void setVersionManager(VersionManager* mgr);

    // Returns version ID to restore, or -1 if none
    int render();

private:
    VersionManager* m_manager = nullptr;
    char m_labelBuffer[128] = {};
};

} // namespace materializr
