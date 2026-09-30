#include "VersionManager.h"
#include "Document.h"
#include "../io/ProjectIO.h"
#include "../i18n.h"

#include <fstream>
#include <sstream>
#include <ctime>
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>   // _mkdir (Windows-only)
#endif

namespace materializr {

VersionManager::VersionManager() {
    m_lastAutoSave = std::time(nullptr);
}

void VersionManager::setProjectDir(const std::string& dir) {
    m_projectDir = dir;
    m_versionsDir = dir + "/.versions";
    ensureVersionsDir();
    loadVersionList();
}

void VersionManager::ensureVersionsDir() {
    if (m_versionsDir.empty()) return;

    struct stat st;
    if (stat(m_versionsDir.c_str(), &st) != 0) {
        // Directory does not exist, create it
#ifdef _WIN32
        _mkdir(m_versionsDir.c_str());
#else
        mkdir(m_versionsDir.c_str(), 0755);
#endif
    }
}

void VersionManager::loadVersionList() {
    m_versions.clear();
    m_nextId = 1;

    std::string listPath = m_versionsDir + "/versions.txt";
    std::ifstream ifs(listPath);
    if (!ifs.is_open()) return;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        // Format: id|label|filename|timestamp|bodyCount
        std::istringstream iss(line);
        std::string idStr, label, filename, timestampStr, bodyCountStr;

        if (!std::getline(iss, idStr, '|')) continue;
        if (!std::getline(iss, label, '|')) continue;
        if (!std::getline(iss, filename, '|')) continue;
        if (!std::getline(iss, timestampStr, '|')) continue;
        if (!std::getline(iss, bodyCountStr, '|')) continue;

        VersionEntry entry;
        try {
            entry.id = std::stoi(idStr);
            entry.label = label;
            entry.filePath = m_versionsDir + "/" + filename;
            entry.timestamp = static_cast<std::time_t>(std::stoll(timestampStr));
            entry.bodyCount = std::stoi(bodyCountStr);
        } catch (...) {
            continue; // Skip malformed lines
        }

        m_versions.push_back(entry);

        if (entry.id >= m_nextId) {
            m_nextId = entry.id + 1;
        }
    }
}

void VersionManager::saveVersionList() {
    std::string listPath = m_versionsDir + "/versions.txt";
    std::ofstream ofs(listPath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) return;

    for (const auto& entry : m_versions) {
        // Extract just the filename from the full path
        std::string filename;
        auto pos = entry.filePath.rfind('/');
        if (pos != std::string::npos) {
            filename = entry.filePath.substr(pos + 1);
        } else {
            filename = entry.filePath;
        }

        ofs << entry.id << "|"
            << entry.label << "|"
            << filename << "|"
            << static_cast<long long>(entry.timestamp) << "|"
            << entry.bodyCount << "|\n";
    }
}

bool VersionManager::saveVersion(const Document& doc, const std::string& label) {
    if (m_versionsDir.empty()) return false;

    ensureVersionsDir();

    VersionEntry entry;
    entry.id = m_nextId++;
    entry.label = label.empty()
                     ? (materializr::tr("Version ") + std::to_string(entry.id))
                     : label;
    entry.timestamp = std::time(nullptr);
    entry.bodyCount = doc.bodyCount();

    // Build the version file path
    std::string filename = "version_" + std::to_string(entry.id) + ".materializr";
    entry.filePath = m_versionsDir + "/" + filename;

    // Save using ProjectIO
    ProjectSaveResult result = ProjectIO::save(entry.filePath, doc);
    if (!result.success) return false;

    m_versions.push_back(entry);
    saveVersionList();

    return true;
}

bool VersionManager::autoSave(const Document& doc) {
    if (!isAutoSaveDue()) return false;

    // Build auto-save label with timestamp
    std::time_t now = std::time(nullptr);
    struct tm* tm_info = std::localtime(&now);
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", tm_info);

    std::string label = std::string(materializr::tr("Auto-save ")) + timeBuf;
    bool ok = saveVersion(doc, label);

    if (ok) {
        m_lastAutoSave = now;
    }
    return ok;
}

const std::vector<VersionEntry>& VersionManager::getVersions() const {
    return m_versions;
}

bool VersionManager::restoreVersion(int versionId, Document& doc) {
    for (const auto& entry : m_versions) {
        if (entry.id == versionId) {
            ProjectLoadResult result = ProjectIO::load(entry.filePath, doc);
            return result.success;
        }
    }
    return false;
}

void VersionManager::setAutoSaveInterval(int seconds) {
    if (seconds > 0) {
        m_autoSaveInterval = seconds;
    }
}

int VersionManager::getAutoSaveInterval() const {
    return m_autoSaveInterval;
}

bool VersionManager::isAutoSaveDue() const {
    return std::time(nullptr) - m_lastAutoSave >= m_autoSaveInterval;
}

} // namespace materializr
