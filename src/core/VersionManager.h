#pragma once
#include <string>
#include <vector>
#include <ctime>

class Document;

namespace materializr {

struct VersionEntry {
    int id;
    std::string label;
    std::string filePath;
    std::time_t timestamp;
    int bodyCount;
};

class VersionManager {
public:
    VersionManager();

    void setProjectDir(const std::string& dir);

    // Save a version snapshot
    bool saveVersion(const Document& doc, const std::string& label = "");

    // Auto-save (called periodically)
    bool autoSave(const Document& doc);

    // Get all versions
    const std::vector<VersionEntry>& getVersions() const;

    // Restore a version
    bool restoreVersion(int versionId, Document& doc);

    // Settings
    void setAutoSaveInterval(int seconds);
    int getAutoSaveInterval() const;

    // Check if auto-save is due
    bool isAutoSaveDue() const;

private:
    std::string m_projectDir;
    std::string m_versionsDir;
    std::vector<VersionEntry> m_versions;
    int m_nextId = 1;
    int m_autoSaveInterval = 300; // 5 minutes
    std::time_t m_lastAutoSave = 0;

    void ensureVersionsDir();
    void loadVersionList();
    void saveVersionList();
};

} // namespace materializr
